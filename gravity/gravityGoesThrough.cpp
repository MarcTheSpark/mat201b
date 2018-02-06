#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;

// Stepping through frame by frame, we see that on the frame that the collision occurs the velocity is still old and causes the ball
// to move through to the other side; the acceleration reacts with a delay.


// some of these must be carefully balanced; i spent some time turning them.
// change them however you like, but make a note of these settings.
unsigned particleCount = 2;     // try 2, 5, 50, and 5000
double maximumGravitationalAcceleration = 30;  // prevents explosion, loss of particles
double maximumSpringAcceleration = 400;  // prevents explosion, loss of particles
double initialRadius = 50;        // initial condition
double initialSpeed = 0;         // initial condition
double gravityFactor = 1e5;       // see Gravitational Constant
double timeStep = 0.0625;         // keys change this value for effect
double scaleFactor = 0.03;         // resizes the entire scene
double sphereRadius = 3;  // increase this to make collisions more frequent
double collisionSpringConstant = -1000.0; // for collision handling

Mesh sphere;  // global prototype; leave this alone
Mesh arrow;

// helper function: makes a random vector
Vec3f r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

struct Particle {
  Vec3f position, velocity, acceleration, oldAcceleration, gravAccel, springAccel;
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
    drawArrow(g, acceleration);
    g.popMatrix();
  }
  void drawArrow(Graphics& g, Vec3f whichVector) {
    rotateZToVector(g, whichVector);
    g.scale(1, 1, whichVector.mag());
    g.draw(arrow);
  }
  void rotateZToVector(Graphics& g, Vec3f x) {
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
    addCone(arrow, sphereRadius/4, Vec3f(0, 0, 1));
    sphere.generateNormals();
    arrow.generateNormals();
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
    runOneFrame = false;

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
          a.springAccel += aTobNormalized * springAcceleration;
          b.springAccel -= aTobNormalized * springAcceleration;
        }
      }
    }

    // Limit acceleration
    for (auto& p : particle) {
      if (p.gravAccel.mag() > maximumGravitationalAcceleration) {
        p.gravAccel.normalize(maximumGravitationalAcceleration);
      }
      p.acceleration = p.springAccel + p.gravAccel;
      if (p.acceleration.mag() > maximumSpringAcceleration) {
        p.acceleration.normalize(maximumSpringAcceleration);
      }
      if (p.springAccel.mag() > 0) {
        cout << p.gravAccel << ", " << p.springAccel << ", " << p.acceleration << ", " << p.velocity;        
      }
      // Euler's Method; Keep the time step small
      p.position += p.velocity * timeStep;
      p.velocity += p.acceleration * timeStep;
      if (p.springAccel.mag() > 0) {
        cout << ", " << p.velocity << endl;
      }
    }

    // for (auto& p : particle) {
    //     p.position += (p.velocity + p.oldAcceleration * timeStep / 2) * timeStep;
    // }
    // for (auto& p : particle) {
    //     p.velocity += (p.oldAcceleration + p.acceleration) * timeStep / 2;
    //     p.oldAcceleration = p.acceleration;
    // }
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
        // pause the simulation
        runOneFrame = true;
        break;
    }
  }
};

int main() { MyApp().start(); }