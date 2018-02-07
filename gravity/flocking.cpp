#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;

// general parameters
unsigned numBoids = 50;
float boidRadius = 1.0;
double initialRadius = 50;
float initialSpeed = 10;
float timeStep = 0.0625;
double scaleFactor = 0.1;
unsigned iterationsPerFrame = 1;

// flocking parameters
float neighborDist = 50;
float desiredSeparation = 4;

float separationWeight = 1.0;
float cohesionWeight = 0.1;
float alignmentWeight = 1.0;

// when Boids go beyond a certain radius from the origin, they start to turn back
// with an acceleration proportional to the distance they have gone beyond that radius
float originPullStartRadius = 150;
float originPullStrength = 1;

Mesh boid;  // global prototype; leave this alone

// helper function: makes a random vector
Vec3f r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

Vec3f& limit(Vec3f& v, double maxMagnitude) {
  if (v.magSqr() > maxMagnitude*maxMagnitude) return v.normalize(maxMagnitude);
  else return v;
}

struct Boid {
  Pose p; // Vec3f + Quatf position+orientation
  Vec3f position, velocity, acceleration;
  float maxSpeed = 20.0;
  float maxAccel = 100.0;
  Color c;
  Boid() {
    position = r() * initialRadius;
    velocity =
        // this will tend to spin stuff around the y axis
        Vec3f(0, 1, 0).cross(position).normalize(initialSpeed);
    p.faceToward(velocity);
    c = HSV(rnd::uniform(), 0.7, 1);
  }
  void draw(Graphics& g) {
    g.pushMatrix();
    g.translate(position);
    g.color(c);
    g.rotate(p);
    g.draw(boid);
    g.popMatrix();
  }
  void step(float dt, vector<Boid>& theFlock) {
     acceleration.zero();
     acceleration = getSeparationForce(theFlock) * separationWeight + 
                    getCohesionForce(theFlock) * cohesionWeight + 
                    getAlignmentForce(theFlock) * alignmentWeight;
     limit(acceleration, maxAccel);
     // we add in the origin pull after normalizing so it can't be overcome by the others
     acceleration += getOriginPullForce();
     limit(acceleration, maxAccel);
     velocity += acceleration * dt;
     limit(velocity, maxSpeed);
     position += velocity * dt; 

     p.faceToward(velocity);
  }
  Vec3f getSeparationForce(vector<Boid>& theFlock) {
    Vec3f sum(0, 0, 0);   // sum up all the separation forces
    unsigned count = 0;
    for (auto&b : theFlock) {
      Vec3f displacement = b.position - position;
      float distance = displacement.mag();
      if(&b != this && distance < desiredSeparation) {
        // if the distance = the desired separation, the separation force is 0 (only just starting to push away)
        // as it approaches a distance of 0, it linearly ramps up to maxSpeed
        sum += -displacement.normalize() * (1 - distance / desiredSeparation) * maxSpeed;
        count++;
      }
    }
    if (count > 0) {
      return sum / count;
    } else {
      return sum;
    }
  }
  Vec3f getCohesionForce(vector<Boid>& theFlock) {
    Vec3f sum(0, 0, 0);
    unsigned count = 0;
    for (auto&b : theFlock) {
      if(&b != this && (b.position - position).mag() < neighborDist) {
        sum += b.position;
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
    Vec3f desiredVelocity = target - position;
    limit(desiredVelocity, maxSpeed);
    desiredVelocity -= velocity;
    return limit(desiredVelocity, maxAccel);
  }
  Vec3f getAlignmentForce(vector<Boid>& theFlock) {
    return Vec3f(0, 0, 0);
  }
  Vec3f getOriginPullForce() {
    float distFromOrigin = position.mag();
    // cout << distFromOrigin << endl;
    if(distFromOrigin > originPullStartRadius) {
      return -position/distFromOrigin;
    } else { return Vec3f(0, 0, 0); }
  }
};

struct FlockingFaces : App {
  Material material;
  Light light;
  bool simulate = true, runOneFrame = false;

  vector<Boid> boids;

  FlockingFaces() {
    light.pos(5, 5, 5);              // place the light
    nav().pos(0, 0, 30*scaleFactor);             // place the viewer
    lens().far(400*scaleFactor);                 // set the far clipping plane
    background(Color(0.07));

    boids.resize(numBoids);
    initWindow();
    initAudio();
  }

  void onAnimate(double dt) {
    if (!simulate && !runOneFrame)
      // skip the rest of this function
      return;
    // I made pressing 5 when the simulation is stopped advance the simulation by a single frame
    // It was useful in debugging the application.
    runOneFrame = false;
    for (unsigned k = 0; k < iterationsPerFrame; ++k) {
      for (auto& b : boids)
        b.step(timeStep / iterationsPerFrame, boids);
    }
  }

  void onDraw(Graphics& g) {
    material();
    light();
    g.scale(scaleFactor);
    for (auto& b : boids)
      b.draw(g);
  }

  void onSound(AudioIO& io) {
    while (io()) {
      io.out(0) = 0;
      io.out(1) = 0;
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
    }
  }
};

void constructBoidMesh() {
  addSphere(boid, boidRadius);
  boid.translate(Vec3f(boidRadius*2.5, 0, 0));
  addSphere(boid, boidRadius);
  boid.translate(Vec3f(-boidRadius*1.25, 0, 0));
  boid.scale(0.4, 0.4, 0.4);
  boid.translate(Vec3f(0, boidRadius*0.4, -boidRadius*0.7));
  addSphere(boid, boidRadius);
  boid.translate(Vec3f(0, 0, 0.8));
  addCone(boid, boidRadius/4, Vec3f(0, 0, -1.0));
  boid.translate(Vec3f(0, 0, -0.8));
  boid.generateNormals();
}

int main() { constructBoidMesh(); FlockingFaces().start(); }
