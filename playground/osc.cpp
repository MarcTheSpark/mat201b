#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;

Mesh them;

struct MyApp : App, osc::PacketHandler {
  Material material;
  Light light;
  Pose other;

  void onMessage(osc::Message& m) {
    if (m.addressPattern() == "/xyz") {
      double x, y, z;
      m >> x;
      m >> y;
      m >> z;
      other.pos(Vec3f(x, y, z));
    } else if (m.addressPattern() == "/xyzw") {
      double x, y, z, w;
      m >> x;
      m >> y;
      m >> z;
      m >> w;
      other.quat(Quatf(x, y, z, w));
    } else
      m.print();
  }

  MyApp() {
    light.pos(5, 5, 5);              // place the light
    initWindow();
    initAudio();
    oscRecv().open(60777, "", 0.016, Socket::UDP);
    oscRecv().handler(*this);
    oscRecv().start();
    oscSend().open(60777, "192.168.1.103", 0.016, Socket::UDP);
    them.generateNormals();
  }

  void onAnimate(double dt) {
    oscSend().send("/xyz", nav().pos().x, nav().pos().y, nav().pos().z);
    oscSend().send("/xyzw", nav().quat().x, nav().quat().y, nav().quat().z,
nav().quat().w);
    // oscSend().send("/name", 
    //   "Marc"
    // );
  }

  void onDraw(Graphics& g) {
    material();
    light();
    g.pushMatrix();
    g.translate(other.pos());
    g.color(Color(0, 0, 1));
    g.rotate(other.quat());
    g.draw(them);
    g.popMatrix();
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
        break;
      case '2':
        break;
      case '3':
        break;
      case '4':
        break;
    }
  }
};

void constructBoidMesh() {
  // constructs the face we know and love by successive additions to the mesh
  addSphere(them, 1);
  them.translate(Vec3f(1*2.5, 0, 0));
  addSphere(them, 1);
  them.translate(Vec3f(-1*1.25, 0, 0));
  them.scale(0.4, 0.4, 0.4);
  them.translate(Vec3f(0, 1*0.4, -1*0.7));
  addSphere(them, 1);
  them.translate(Vec3f(0, 0, 0.8));
  addCone(them, 0.25, Vec3f(0, 0, -1.0));
  them.translate(Vec3f(0, 0, -0.8));
  them.generateNormals();
}

int main() { constructBoidMesh(); MyApp().start(); }

// #include "allocore/io/al_App.hpp"
// using namespace al;
// using namespace std;

// struct MyApp : App, osc::PacketHandler {
//   MyApp() {
//     initWindow();
//     initAudio();
//     // I listen on 60777
//     oscRecv().open(60777, "", 0.016, Socket::UDP);
//     oscRecv().handler(*this);
//     oscRecv().start();
//     // you better be listening on 60777
//     oscSend().open(60777, "192.168.1.103", 0.016, Socket::UDP);
//     addSphere(them);
//   }
//   Mesh them;
//   Pose other;
//   void onAnimate(double dt) {
//     oscSend().send("/xyz", nav().pos().x, nav().pos().y, nav().pos().z);
//     oscSend().send("/xyzw", nav().quat().x, nav().quat().y, nav().quat().z,
//                    nav().quat().w);
//   }
//   void onMessage(osc::Message& m) {
//     if (m.addressPattern() == "/xyz") {
//       float x, y, z;
//       m >> x;
//       m >> y;
//       m >> z;
//       other.pos(Vec3f(x, y, z));
//     } else if (m.addressPattern() == "/xyzw") {
//       float x, y, z, w;
//       m >> x;
//       m >> y;
//       m >> z;
//       m >> w;
//       other.quat(Quatf(x, y, z, w));
//     } else
//       m.print();
//   }

//   void onDraw(Graphics& g) {
//     g.translate(other.pos());
//     g.rotate(other.quat());
//     g.draw(them);
//   }
// };

// int main() { MyApp().start(); }
