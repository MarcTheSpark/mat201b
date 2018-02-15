/*
Gravity simulation implementing spring collisions
Marc Evans
Mat201B Winter 2018
2/7/2018
marc.p.evans@gmail.com
Licensed under the CC Attribution-ShareAlike license, assuming I'm allowed to do that.
*/

// static wavetable initialization
// placing sources
// virtual

#define SAMPLE_RATE (44100)
#define BLOCK_SIZE (256)
#include "allocore/io/al_App.hpp"
#include "Cuttlebone/Cuttlebone.hpp"
#include "common.hpp"
using namespace al;
using namespace std;


static AudioScene scene(BLOCK_SIZE);
static StereoPanner *panner;
static SpeakerLayout speakerLayout;

unsigned particleCount = 50;     // try 2, 5, 50, and 5000
double maximumGravitationalAcceleration = 30;  // prevents explosion, loss of particles
double maximumSpringAcceleration = 400;  // much higher, since we need a strong reaction
double initialRadius = 50;        // initial condition
double initialSpeed = 0;         // initial condition
double gravityFactor = 1e5;       // Gravitational Constant (I reduced this by an order of magnitude)
double timeStep = 0.0625;         // keys change this value for effect
double scaleFactor = 0.03;         // resizes the entire scene
double sphereRadius = 3;  // increase this to make collisions more frequent

double collisionSpringConstant = -1000.0; // k
unsigned iterationsPerFrame = 10; // we run multiple iterations per frame, which turned out to be crucial
// I added visualization of velocity and acceleration for debugging. 
// 0=no arrow drawn, 1=velocity, 2=net acceleration, 3=gravitational accel, 4=spring accel
unsigned arrowToDraw = 0;

Mesh sphere;  // global prototype; leave this alone
Mesh arrow;  // to draw arrows for debugging purposes

// helper function: makes a random vector
Vec3f r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

// (0, 1)
struct Phasor {
  float phase, increment;

  void freq(float hz) { increment = float(hz) / SAMPLE_RATE; }
  float freq() { return increment * SAMPLE_RATE; }

  Phasor(float frequency=440, float startingPhase=0) {
    freq(frequency);
    phase = startingPhase;
  }

  virtual float getNextSample() {
    float returnValue = phase;
    phase += increment;
    if (phase >= 1) phase -= 1;
    return returnValue;
  }

  float operator()() { return getNextSample(); }
};

// (-1, 1)
struct Sawtooth : Phasor {
  using Phasor::Phasor;
  float getNextSample() { return 2 * Phasor::getNextSample() - 1; }
};

struct SinOsc : Phasor {
  float wavetable[1024];
  SinOsc(float frequency=440, float phase=0) {
    Phasor::Phasor(frequency, phase);
    for(unsigned i = 0; i < 1024; ++i) { wavetable[i] = sin(float(i)/512*M_PI); }
  }
  float getNextSample() { return wavetable[int(Phasor::getNextSample() * 1024)]; }
};

struct LinInterp {
  float lastTarget = 0, currentTarget = 0, progress = 0, progressIncrement;

  void target(float t) { lastTarget = currentTarget; currentTarget = t; progress = 0; }
  void set(float value) { lastTarget = currentTarget = value; progress = 1; }
  void time(float t) { progressIncrement = 1.0 / (SAMPLE_RATE * t); progress = 0; }

  LinInterp(float initialValue = 0, float transitionTime = float(BLOCK_SIZE) / SAMPLE_RATE) {
    set(initialValue);
    time(transitionTime);
  }

  float getNextSample() {
    float returnValue = progress * currentTarget + (1-progress) * lastTarget;
    if (progress >= 1) 
      progress = 1;
    else
      progress += progressIncrement;
    return returnValue;
  }

  bool zero() {
    // it's zero and it's not changing
    return (currentTarget == 0) && (lastTarget == 0);
  }

  float operator()() { return getNextSample(); }
};

struct Particle : SoundSource {
  Vec3f position, velocity, acceleration, gravAccel, springAccel;
  Color c;

  LinInterp f1, f2, f3, gain;
  SinOsc s1, s2, s3;
  Particle() {
    f1.time(0.01); f2.time(0.01); f3.time(0.01); gain.time(0.08);
    setFrequencies();
    position = r() * initialRadius;
    velocity =
        // this will tend to spin stuff around the y axis
        Vec3f(0, 1, 0).cross(position).normalize(initialSpeed);
    c = HSV(rnd::uniform(), 0.7, 1);
  }

  void draw(Graphics& g) {
    g.pushMatrix();
    g.translate(position);
    g.scale(sphereRadius);
    g.color(c);
    g.draw(sphere);
    switch(arrowToDraw) {
      default:
      case 0:
        break;
      case 1:
        drawArrow(g, velocity);
        break;
      case 2:
        drawArrow(g, acceleration);
        break;
      case 3:
        drawArrow(g, gravAccel);
        break;
      case 4:
        drawArrow(g, springAccel);
        break;
    }
    
    g.popMatrix();
  }
  void drawArrow(Graphics& g, Vec3f whichVector) {
    // point the arrow at the vector we want to illustrate
    rotateZToVector(g, whichVector);
    // scale the arrow to the magnitude of that vector
    g.scale(1, 1, whichVector.mag());
    g.draw(arrow);
  }
  void rotateZToVector(Graphics& g, Vec3f x) {
    /* Rotates such that the z-axis points in the
    same direction as x. In future, would probably use
    the Quat object, but I didn't understand it */
    Vec3f z = Vec3f(0, 0, 1);
    x.normalize();
    Vec3f axis = z.cross(x);
    double angle = acos(x.dot(z)/(x.mag()*z.mag()));
    // surprisingly g.rotate expects degrees
    g.rotate(angle*360/(2*M_PI), axis);
  }


  void setFrequencies() {
    f1.set(1900 + velocity.x*20);
    f2.set(2000 + velocity.y*20);
    f3.set(2100 + velocity.z*20);
    gain.set(0);
  }

  void targetFrequencies() {
    f1.target(1900 + velocity.x*20);
    f2.target(2000 + velocity.y*20);
    f3.target(2100 + velocity.z*20);
    if (springAccel.mag() > 0) {
      gain.target(std::min(springAccel.mag()/3000, 1.0f));
    } else {
      gain.target(gain.currentTarget * 0.96);
    }
  }

  void prepareForBlock() {
    targetFrequencies();
  }
  void calcSample() {
    // use FM of velocity coords * magnitude of accel
    if(!gain.zero()) {
      s1.freq(f1());
      s2.freq(f2() + 1000 * s1());
      s3.freq(f3() + 1000 * s2());
      this->writeSample(s3() * gain());
    }
    this->writeSample(0);
  }
};

struct GravitySimulator : App {
  Listener * listener;
  // StereoPanner * panner;
  // StereoSpeakerLayout * speakerLayout;

  State state;
  cuttlebone::Maker<State> maker;
  Material material;
  Light light;
  bool simulate = true, runOneFrame = false;

  vector<Particle> particles;

  GravitySimulator() {
    addSphere(sphere, 1.0);
    addCone(arrow, 0.25, Vec3f(0, 0, 1)); // to draw arrows for debugging purposes
    sphere.generateNormals();
    arrow.generateNormals(); // to draw arrows for debugging purposes
    light.pos(0, 0, 0);              // place the light
    nav().pos(0, 0, 300 * scaleFactor);  // place the viewer (I changed this to respond to scaleFactor)
    lens().far(4000 * scaleFactor);   // set the far clipping plane (I changed this to respond to scaleFactor)
    particles.resize(particleCount);  // make all the particles


    setInitialStateInfo();
    setVariableStateInfo();
    maker.set(state);
    background(Color(0.07));

    initWindow();
    // initAudio(SAMPLE_RATE, BLOCK_SIZE);
    initAudio(SAMPLE_RATE, BLOCK_SIZE, 2, 0);
    listener = scene.createListener(new StereoPanner(StereoSpeakerLayout()));
    for (auto& p : particles) { scene.addSource(p); p.pos(0, 0, 0); }
  }

  void setInitialStateInfo() {
    state.numParticles = particleCount;
    state.sphereRadius = sphereRadius;
    for(unsigned i = 0; i < particleCount; i++) {
      state.particleColors[i] = particles[i].c;
    }
  }

  void setVariableStateInfo() {
    for(unsigned i = 0; i < particleCount; i++) {
      Particle& p = particles[i];
      state.particlePositions[i] = p.position;
      switch(arrowToDraw) {
        default:
        case 0:
          state.drawDebugVectors = false;
          break;
        case 1:
          state.drawDebugVectors = true;
          state.particleDebugVectors[i] = p.velocity;
          break;
        case 2:
          state.drawDebugVectors = true;
          state.particleDebugVectors[i] = p.acceleration;
          break;
        case 3:
          state.drawDebugVectors = true;
          state.particleDebugVectors[i] = p.gravAccel;
          break;
        case 4:
          state.drawDebugVectors = true;
          state.particleDebugVectors[i] = p.springAccel;
          break;
      }
    }
  }

  void onAnimate(double dt) override {
    if (!simulate && !runOneFrame)
      // skip the rest of this function
      return;
    // I made pressing 5 when the simulation is stopped advance the simulation by a single frame
    // It was useful in debugging the application.
    runOneFrame = false;

    // This turned out to be crucial; I split each frame up into a series of iterations
    // thereby increasing the temporal resolution without slowing down the simulation
    // This resolution was necessary to make the simulation behave appropriately
    for (unsigned k = 0; k < iterationsPerFrame; ++k) {
      // Added this in: it seems important to zero the acceleration before doing the accounting
      for (auto& p : particles) p.acceleration = p.gravAccel = p.springAccel = 0;

      for (unsigned i = 0; i < particles.size(); ++i) {
        for (unsigned j = 1 + i; j < particles.size(); ++j) {
          Particle& a = particles[i];
          Particle& b = particles[j];
          Vec3f difference = (b.position - a.position);
          double d = difference.mag();
          // F = ma where m=1
          Vec3f acceleration = difference / (d * d * d) * gravityFactor;
          // equal and opposite force (symmetrical)
          a.gravAccel += acceleration;
          b.gravAccel -= acceleration;
        }
      }

      for (unsigned i = 0; i < particles.size(); ++i) {
        for (unsigned j = 1 + i; j < particles.size(); ++j) {
          Particle& a = particles[i];
          Particle& b = particles[j];
          Vec3f difference = (b.position - a.position);
          double d = difference.mag();
          Vec3f aTobNormalized = difference / d;

          // handle collision spring repulsion; the spring kicks in when distance is less than 
          // twice the sphere radius, i.e. they are touching. It's a one way spring.
          if(d < 2*sphereRadius) {
            double springAcceleration = collisionSpringConstant * (2*sphereRadius - d);
            // a moves in the opposite direction of aTobNormalized, b in the same direction
            // note that the spring constant is negative
            a.springAccel += aTobNormalized * springAcceleration;
            b.springAccel -= aTobNormalized * springAcceleration;
          }
        }
      }

      for (auto& p : particles) {
        // Limit acceleration
        // First, we limit gravitational acceleration
        if (p.gravAccel.mag() > maximumGravitationalAcceleration) {
          p.gravAccel.normalize(maximumGravitationalAcceleration);
        }
        // then we add in the spring acceleration (unlimited)
        p.acceleration = p.springAccel + p.gravAccel;
        // then we limit the total acceleration
        // (I did it this way so that the initially unlimited spring acceleration can overpower
        // the limited gravitational acceleration, since we really want the spring to win.)
        if (p.acceleration.mag() > maximumSpringAcceleration) {
          p.acceleration.normalize(maximumSpringAcceleration);
        }

        // Euler's Method; Keep the time step small
        // (Note that here I reversed the updating so that velocity is updated first. 
        // This way sudden changes in acceleration due to collision are incorporated immediately.)
        p.velocity += p.acceleration * (timeStep / iterationsPerFrame);
        p.position += p.velocity * (timeStep / iterationsPerFrame);
      }
    }
    setVariableStateInfo();
    maker.set(state);
  }

  void onDraw(Graphics& g) override {
    listener->pose(nav());
    material();
    light();
    g.scale(scaleFactor);
    for (auto p : particles) p.draw(g);
  }

  void onSound(AudioIOData& io) override {
    for (auto& p : particles) { p.prepareForBlock(); }
    while (io()) {
      for (auto& p : particles) { p.calcSample(); }
    }
    scene.render(io);
  }

  void onKeyDown(const ViewpointWindow&, const Keyboard& k) override {
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


int main() {
  GravitySimulator gravitySimulator;
  gravitySimulator.maker.start();
  gravitySimulator.start(); 
}