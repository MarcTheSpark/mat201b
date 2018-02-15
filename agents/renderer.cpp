// MAT201B
// Fall 2015
// Author: Karl Yerkes
//
// Cuttlebone "Laptop Graphics Renderer"
//

#include "Cuttlebone/Cuttlebone.hpp"
#include "allocore/io/al_App.hpp"

#include "common.hpp"

using namespace al;

float boidRadius = 0.1;
Mesh boid;

float followLerp = 0.07;
float lerpRampUpTime = 1.0;

//  needs to know particle positions, colors, and arrow vectors

struct FlockingRenderer : App {

  State state;
  cuttlebone::Taker<State> taker;
  vector<Color> boidColors;
  int perspective = -1;            // -1 means free motion, otherwise it's the index of the boid to follow
  float lerpAmount = 0;

  Material material;
  Light light;

  FlockingRenderer() {
    light.pos(0, 0, 0);              // place the light
    nav().pos(0, 0, 0);  // place the viewer (I changed this to respond to scaleFactor)
    lens().far(400); 
    taker.get(state);
    initWindow();
  }

  virtual void onAnimate(double dt) { 
    taker.get(state); 
    if(perspective >= 0) { 
      nav().set(nav().lerp(state.boidPoses[perspective], lerpAmount)); 
      if (lerpAmount < followLerp) {
        lerpAmount += dt / lerpRampUpTime * followLerp;
      } else { lerpAmount = followLerp; }
    }
  }

  void drawNthBoid(Graphics& g, unsigned n) {
    Pose& p = state.boidPoses[n];
    if (n >= boidColors.size()) { 
      boidColors.push_back(Color(HSV(rnd::uniform(), 0.7, 1))); 
    } else { g.color(boidColors[n]); }
    g.pushMatrix();
    g.translate(p.pos());
    g.rotate(p);
    g.draw(boid);
    g.popMatrix();
  }

  virtual void onDraw(Graphics& g, const Viewpoint& v) {
    material();
    light();
    if (state.numBoids == 0) return;   // this ensures that the state has been initialized before drawing
    for(unsigned n=0; n < state.numBoids; ++n) {
      drawNthBoid(g, n);
    }
  }

  void onKeyDown(const ViewpointWindow&, const Keyboard& k) {
    switch (k.key()) {
      default:
      case 'o':
        nav().home();
        perspective = -1;
        break;
      case 'p':
        lerpAmount = 0;
        perspective = rand() % state.numBoids;
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
  FlockingRenderer flockingRenderer;
  flockingRenderer.taker.start();
  flockingRenderer.start();
}
