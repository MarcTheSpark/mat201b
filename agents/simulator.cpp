/*
Agent simulation implementing flocking behavior
Marc Evans
Mat201B Winter 2018
2/7/2018
marc.p.evans@gmail.com
Licensed under the CC Attribution-ShareAlike license, assuming I'm allowed to do that.
*/

#include "Cuttlebone/Cuttlebone.hpp"
#include "common.hpp"
#include "allocore/io/al_App.hpp"
#include "Gamma/Oscillator.h"
#include "Gamma/Noise.h"
#include "Gamma/Filter.h"
#include "Gamma/Analysis.h"
using namespace al;
using namespace std;
using namespace gam;

// general parameters
unsigned numBoids = 300;
float boidRadius = 0.1;
double initialRadius = 5;
float initialSpeed = 1.0;
float timeStep = 0.0625;
unsigned iterationsPerFrame = 1;

// flocking parameters
float neighborDist = 5;
float desiredSeparation = 1.5;

float separationWeight = 0.5;
float cohesionWeight = 0.1;
float alignmentWeight = 0.1;

// when Boids go beyond a certain radius from the origin, they start to turn back
// with an acceleration proportional to the distance they have gone beyond that radius
float originPullStartRadius = 10;
float originPullStrength = 0.5;

float followLerp = 0.07;
float lerpRampUpTime = 1.0;

Mesh boid;


// helper function: makes a random vector
Vec3f r() { return Vec3f(al::rnd::uniformS(), al::rnd::uniformS(), al::rnd::uniformS()); }

// added this to limit the length of a vector, since I didn't see one in the Vec3f class
// (I ran into problems using normalize initially, since it would also make the vector
// longer when I didn't want it to.)
Vec3f& limit(Vec3f& v, double maxMagnitude) {
  if (v.magSqr() > maxMagnitude*maxMagnitude) return v.normalize(maxMagnitude);
  else return v;
}

struct Boid {
  LFO<> lfo;
  Biquad<> lp, bp1, bp2;
  EnvFollow<> accelFollow, speedFollow;
  float pan = al::rnd::uniformS();
  float vmag = 0;

  float baseFreq = 110 + int(al::rnd::uniform()*2)*55;

  Pose p;
  Vec3f velocity, acceleration;
  float maxSpeed = 2.0;
  float maxAccel = 10.0;
  Color c;
  Boid() : 
  c(HSV(al::rnd::uniform(), 0.7, 1)), 
  lfo(baseFreq), 
  lp(600, 20, LOW_PASS),
  bp1(1200, 20, BAND_PASS),
  bp2(2000, 20, BAND_PASS) {
    p.pos(r() * initialRadius);
    velocity =
        // this will tend to spin stuff around the y axis
        Vec3f(0, 1, 0).cross(p.pos()).normalize(initialSpeed);
    p.faceToward(p.pos() + velocity);
  }
  void draw(Graphics& g) {
    g.pushMatrix();
    g.translate(p.pos());
    g.color(c);
    g.rotate(p);
    g.draw(boid);
    g.popMatrix();
  }
  void step(float dt, vector<Boid>& theFlock) {
     acceleration.zero();
     // find the acceleration from the flocking forces
     acceleration = getSeparationForce(theFlock) * separationWeight + 
                    getCohesionForce(theFlock) * cohesionWeight + 
                    getAlignmentForce(theFlock) * alignmentWeight;
     limit(acceleration, maxAccel);
     // we add in the origin pull force after normalizing so it can't be overcome by strong flockign forces
     acceleration += getOriginPullForce();
     limit(acceleration, maxAccel);
     // euler
     velocity += acceleration * dt;
     limit(velocity, maxSpeed);
     p.pos(p.pos() + velocity * dt);

     // face in the direction it's moving
     p.faceToward(p.pos() + velocity);
  }
  Vec3f getSeparationForce(vector<Boid>& theFlock) {
    // sum up all the separation forces from the various boids and limits to the max acceleration
    Vec3f sum(0, 0, 0);
    for (auto&b : theFlock) {
      Vec3f displacement = b.p.pos() - p.pos();
      float distance = displacement.mag();
      // check only other boids that are below the desired separation
      if(&b != this && distance < desiredSeparation) {
        // if the distance = the desired separation, the separation force is 0 (only just starting to push away)
        // as it approaches a distance of 0, it linearly ramps up to maxSpeed
        sum += -displacement.normalize() * (1 - distance / desiredSeparation) * maxSpeed;
      }
    }
    return limit(sum, maxAccel);
  }
  Vec3f getCohesionForce(vector<Boid>& theFlock) {
    // averages the location of all the neighboring boids and provides a force in that direction
    Vec3f sum(0, 0, 0);
    unsigned count = 0;
    for (auto&b : theFlock) {
      if(&b != this && (b.p.pos() - p.pos()).mag() < neighborDist) {
        sum += b.p.pos();
        count++;
      }
    }
    if (count > 0) {
      return seek(sum / count);
    } else {
      return sum;
    }
  }
  Vec3f seek(Vec3f target) {
    // provides a corrective steering force to head towards a given location
    Vec3f desiredVelocity = target - p.pos();
    limit(desiredVelocity, maxSpeed);
    desiredVelocity -= velocity;
    return limit(desiredVelocity, maxAccel);
  }
  Vec3f getAlignmentForce(vector<Boid>& theFlock) {
    // averages the velocity of neighboring boids and returns an acceleration that corrects towards that velocity
    Vec3f sum(0, 0, 0);
    for (auto&b : theFlock) {
      if(&b != this && (b.p.pos() - p.pos()).mag() < neighborDist) {
        sum += b.velocity;
      }
    }
    if (sum.magSqr() > 0) {
      // as long as there's some non-zero average velocity from the boid's neighbors
      // we'll normalize it to the max speed (no need to divide out by a count, 
      // since we're normalizing anyway)
      sum.normalize(maxSpeed);
      // get the difference between that direction at maxSpeed and our current velocity
      sum -= velocity;
      // and limit it to the max acceleration;
      return limit(sum, maxAccel);
    } else {
      return sum;
    }
  }
  Vec3f getOriginPullForce() {
    // returns a force pointing towards the origin if the boid has strayed beyond the originPullStartRadius
    // it grows in magnitude the further away from the origin the boid is
    float distFromOrigin = p.pos().mag();
    if(distFromOrigin > originPullStartRadius) {
      return -p.pos()/distFromOrigin * (distFromOrigin - originPullStartRadius) * originPullStrength;
    } else { return Vec3f(0, 0, 0); }
  }
  float prepareForBlock() {
    lfo.freq(accelFollow(baseFreq * (1+acceleration.mag()/maxAccel*3)));
    lp.freq(600 + velocity.x * 150);
    bp1.freq(1200 + velocity.y * 250);
    bp2.freq(2000 + velocity.x * 350);
    vmag = velocity.mag();
  }
  float getSample() {
    float triVal = lfo.tri();
    float total = (lp(triVal) + bp1(triVal) + bp2(triVal))/3;
    return total * speedFollow(vmag / maxSpeed * 0.2);
  }
};

struct FlockingFaces : App {
  Material material;
  Light light;
  bool simulate = true, runOneFrame = false;
  int perspective = -1;            // -1 means free motion, otherwise it's the index of the boid to follow
  float lerpAmount = 0;

  vector<Boid> boids;

  State state;
  cuttlebone::Maker<State> maker;

  FlockingFaces() {
    light.pos(5, 5, 5);              // place the light
    nav().pos(0, 0, 0);             // place the viewer
    lens().far(400);                 // set the far clipping plane
    setInitialStateInfo();
    maker.set(state);
    background(Color(0.07));

    boids.resize(numBoids);
    initWindow();
    initAudio();
  }

  void setInitialStateInfo() {
    state.numBoids = numBoids;
  }

  void setVariableStateInfo() {
    for(unsigned i = 0; i < numBoids; i++) {
      state.boidPoses[i] = boids[i].p;
    }
  }

  void onAnimate(double dt) {
    if (!simulate && !runOneFrame)
      // skip the rest of this function
      return;
    // I made pressing 5 when the simulation is stopped advance the simulation by a single frame
    // It was useful in debugging the application.
    runOneFrame = false;
    // we can do multiple iterations per frame like in the gravity simulation
    // but because there were no sudden strong collision forces, it's not really necessary
    for (unsigned k = 0; k < iterationsPerFrame; ++k) {
      for (auto& b : boids)
        b.step(timeStep / iterationsPerFrame, boids);
    }
    if(perspective >= 0) { 
      nav().set(nav().lerp(boids[perspective].p, lerpAmount)); 
      if (lerpAmount < followLerp) {
        lerpAmount += dt / lerpRampUpTime * followLerp;
      } else { lerpAmount = followLerp; }
    }
    setVariableStateInfo();
    maker.set(state);
  }

  void onDraw(Graphics& g) {
    material();
    light();
    for (int i=0; i < boids.size(); ++i) {
      boids[i].draw(g);
    }
  }

  void onSound(AudioIOData& io) {
    gam::Sync::master().spu(audioIO().fps());
    for (auto& b : boids) { b.prepareForBlock(); }
    while (io()) {
      float l = 0, r = 0;
      // s += boids[0].getSample();
      for (auto& b : boids) { 
        float s = b.getSample() / sqrt(numBoids);  
        l += b.pan * s;
        r += (1-b.pan) * s;
      }
      io.out(0) = l;
      io.out(1) = r;
    }
  }

  void onKeyDown(const ViewpointWindow&, const Keyboard& k) {
    switch (k.key()) {
      default:
      case '1':
        // reverse time
        timeStep *= -1;
        break;
      case '2':
        // speed up time
        if (timeStep < 1) timeStep *= 2;
        break;
      case '3':
        // slow down time
        if (timeStep > 0.0005) timeStep /= 2;
        break;
      case '4':
        // pause the simulation
        simulate = !simulate;
        break;
      case '5':
        // advance the simulation by a single frame (when the simulation is paused)
        runOneFrame = true;
        break;
      case 'o':
        nav().home();
        perspective = -1;
        break;
      case 'p':
        lerpAmount = 0;
        perspective = rand() % boids.size();
        break;
    }
  }
};

void constructBoidMesh() {
  // constructs the face we know and love by successive additions to the mesh
  addSphere(boid, boidRadius);
  boid.translate(Vec3f(boidRadius*2.5, 0, 0));
  addSphere(boid, boidRadius);
  boid.translate(Vec3f(-boidRadius*1.25, 0, 0));
  boid.scale(0.4, 0.4, 0.4);
  boid.translate(Vec3f(0, boidRadius*0.4, -boidRadius*0.7));
  addSphere(boid, boidRadius);
  boid.translate(Vec3f(0, 0, boidRadius*0.8));
  addCone(boid, boidRadius/4, Vec3f(0, 0, -boidRadius));
  boid.translate(Vec3f(0, 0, -boidRadius*0.8));
  boid.generateNormals();
}

int main() {
  constructBoidMesh(); 
  FlockingFaces flockingApp;
  flockingApp.maker.start();
  flockingApp.start(); 
}