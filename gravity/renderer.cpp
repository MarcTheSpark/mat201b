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

double scaleFactor = 0.03;         // resizes the entire scene

//  needs to know particle positions, colors, and arrow vectors

struct GravityRenderer : App {
  Mesh sphere;
  Mesh arrow;

  State state;
  cuttlebone::Taker<State> taker;

  Material material;
  Light light;

  GravityRenderer() {
    addSphere(sphere, 1.0);
    addCone(arrow, 0.25, Vec3f(0, 0, 1)); // to draw arrows for debugging purposes
    sphere.generateNormals();
    arrow.generateNormals(); // to draw arrows for debugging purposes

    light.pos(0, 0, 0);              // place the light
    nav().pos(0, 0, 300 * scaleFactor);  // place the viewer (I changed this to respond to scaleFactor)
    lens().far(4000 * scaleFactor); 
    taker.get(state);
    initWindow();
  }

  virtual void onAnimate(double dt) { taker.get(state); }

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

  void drawNthParticle(Graphics& g, unsigned n) {
    g.pushMatrix();
    g.translate(state.particlePositions[n]);
    g.scale(state.sphereRadius);
    g.color(state.particleColors[n]);
    g.draw(sphere);
    if (state.drawDebugVectors) drawArrow(g, state.particleDebugVectors[n]);
    g.popMatrix();
  }

  virtual void onDraw(Graphics& g, const Viewpoint& v) {
    material();
    light();
    g.scale(scaleFactor);
    if (state.numParticles == 0) return;   // this ensures that the state has been initialized before drawing
    for(unsigned n=0; n < state.numParticles; ++n) {
      drawNthParticle(g, n);
    }
  }
};

int main() {
  GravityRenderer gravityRenderer;
  gravityRenderer.taker.start();
  gravityRenderer.start();
}
