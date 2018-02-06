#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;

float faceRadius = 1.0;
float initialSpeed = 10;
float timeStep = 0.0625;
double scaleFactor = 0.1;         // resizes the entire scene

Mesh face;  // global prototype; leave this alone

// helper function: makes a random vector
Vec3f r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

struct Face {
  Vec3f position, velocity, acceleration;
  Pose p; // Vec3f + Quatf position+orientation
  Color c;
  Face() {
    cout << p.uu() << endl;
    position = Vec3f(0, 0, 0);
    velocity = Vec3f(0, 0, 0);
    p.faceToward(Vec3f(0, 0, 1), Vec3f(0, 1, 0));
    // position = r() * faceRadius;
    // velocity =
    //     // this will tend to spin stuff around the y axis
    //     Vec3f(0, 1, 0).cross(position).normalize(initialSpeed);
    c = HSV(rnd::uniform(), 0.7, 1);
  }
  void draw(Graphics& g) {
    g.pushMatrix();
    g.translate(position);
    g.color(c);
    //g.multMatrix(p.matrix());
    g.modelView(p.matrix());
    // g.rotate(p);
    g.draw(face);
    g.popMatrix();
  }
  void step(float dt) {
     velocity += acceleration * dt;
     position += velocity * dt;
  }
};

struct MyApp : App {
  Material material;
  Light light;
  bool simulate = true;

  Face f;

  MyApp() {

    light.pos(5, 5, 5);              // place the light
    nav().pos(0, 0, 30*scaleFactor);             // place the viewer
    lens().far(400*scaleFactor);                 // set the far clipping plane
    background(Color(0.07));

    initWindow();
    initAudio();
  }

  void onAnimate(double dt) {
    if (!simulate)
      // skip the rest of this function
      return;
    f.step(timeStep);
  }

  void onDraw(Graphics& g) {
    material();
    light();
    g.scale(scaleFactor);
    f.draw(g);
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
    }
  }
};

void constructFaceMesh() {
  addSphere(face, faceRadius);
  face.translate(Vec3f(faceRadius*2.5, 0, 0));
  addSphere(face, faceRadius);
  face.translate(Vec3f(-faceRadius*1.25, 0, 0));
  face.scale(0.4, 0.4, 0.4);
  face.translate(Vec3f(0, faceRadius*0.4, -faceRadius*0.7));
  addSphere(face, faceRadius);
  face.translate(Vec3f(0, 0, 0.8));
  addCone(face, faceRadius/4, Vec3f(0, 0, -1.0));
  face.translate(Vec3f(0, 0, -0.8));
  face.generateNormals();
}

int main() { constructFaceMesh(); MyApp().start(); }
