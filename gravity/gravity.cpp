#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;

// some of these must be carefully balanced; i spent some time turning them.
// change them however you like, but make a note of these settings.
unsigned particleCount = 50;     // try 2, 5, 50, and 5000
double maximumGravitationalAcceleration = 30;  // prevents explosion, loss of particles
double maximumSpringAcceleration = 400;  // prevents explosion, loss of particles
double initialRadius = 50;        // initial condition
double initialSpeed = 0;         // initial condition
double gravityFactor = 1e5;       // see Gravitational Constant
double timeStep = 0.0625;         // keys change this value for effect
double scaleFactor = 0.03;         // resizes the entire scene
double sphereRadius = 3;  // increase this to make collisions more frequent

double collisionSpringConstant = -1000.0; // for collision handling
unsigned iterationsPerFrame = 10;
// I added this visualization for debugging. 0=no arrows drawn
// 1=velocity, 2=net acceleration, 3=gravitational accel, 4=spring accel
unsigned arrowToDraw = 0;

Mesh sphere;  // global prototype; leave this alone
Mesh arrow;  // to draw arrows for debugging purposes

// helper function: makes a random vector
Vec3f r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

struct Particle {
  Vec3f position, velocity, acceleration, gravAccel, springAccel;
  Color c;
  Particle() {
    position = r() * initialRadius;
    velocity =
        // this will tend to spin stuff around the y axis
        Vec3f(0, 1, 0).cross(position).normalize(initialSpeed);
    c = HSV(rnd::uniform(), 0.7, 1);
  }
  void draw(Graphics& g) {
    g.pushMatrix();
    g.translate(position);
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
    same direction as x */
    Vec3f z = Vec3f(0, 0, 1);
    x.normalize();
    Vec3f axis = z.cross(x);
    double angle = acos(x.dot(z)/(x.mag()*z.mag()));
    g.rotate(angle*360/(2*M_PI), axis);
  }
};

struct MyApp : App {
  Material material;
  Light light;
  bool simulate = true, runOneFrame = false;

  vector<Particle> particle;

  MyApp() {
    addSphere(sphere, sphereRadius);
    addCone(arrow, sphereRadius/4, Vec3f(0, 0, 1)); // to draw arrows for debugging purposes
    sphere.generateNormals();
    arrow.generateNormals(); // to draw arrows for debugging purposes
    light.pos(0, 0, 0);              // place the light
    nav().pos(0, 0, 300 * scaleFactor);  // place the viewer (I changed this to respond to scaleFactor)
    lens().far(4000 * scaleFactor);   // set the far clipping plane (I changed this to respond to scaleFactor)
    particle.resize(particleCount);  // make all the particles
    background(Color(0.07));

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

    // This turned out to be crucial; I split each frame up into a series of iterations
    // thereby increasing the temporal resolution without slowing down the simulation
    // This resolution was necessary to make the simulation behave appropriately
    for (unsigned k = 0; k < iterationsPerFrame; ++k) {
      // Added this in: it seems important to zero the acceleration before doing the accounting
      for (auto& p : particle) p.acceleration = p.gravAccel = p.springAccel = 0;

      for (unsigned i = 0; i < particle.size(); ++i) {
        for (unsigned j = 1 + i; j < particle.size(); ++j) {
          Particle& a = particle[i];
          Particle& b = particle[j];
          Vec3f difference = (b.position - a.position);
          double d = difference.mag();
          // F = ma where m=1
          Vec3f acceleration = difference / (d * d * d) * gravityFactor;
          // equal and opposite force (symmetrical)
          a.gravAccel += acceleration;
          b.gravAccel -= acceleration;
        }
      }

      for (unsigned i = 0; i < particle.size(); ++i) {
        for (unsigned j = 1 + i; j < particle.size(); ++j) {
          Particle& a = particle[i];
          Particle& b = particle[j];
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

      for (auto& p : particle) {
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
  }

  void onDraw(Graphics& g) {
    material();
    light();
    g.scale(scaleFactor);
    for (auto p : particle) p.draw(g);
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

int main() { MyApp().start(); }