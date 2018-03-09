/*
  Marc Evans (2018/3/8)
  Final Project Renderer
*/

#include <cassert>
#include <iostream>
#include <fstream>

#include <deque>
#include "allocore/io/al_App.hpp"
#include "Cuttlebone/Cuttlebone.hpp"
#include "common.hpp"
using namespace al;
using namespace std;

#define FFT_SIZE (1024)
#define NUM_LEAF_LOOPERS (2)
#define REDUNDANCY (5)
#define VISUAL_DECAY (0.8)


struct LeafLooper {
  Pose p;
  deque<Mesh> radialStrips;
  int maxStrips = 60;

  LeafLooper() {}

  void pushNewStrip(Mesh strip) {
    radialStrips.push_back(strip);
    if(radialStrips.size() > maxStrips) {
      radialStrips.pop_front();
    }
    for(auto& radialStrip : radialStrips) {
      for(auto& color : radialStrip.colors()) {
        color.a *= VISUAL_DECAY;
      }
    }
  }

  void draw(Graphics& g) {
    g.pushMatrix();
    g.blendOn();
    g.blendModeTrans();
    g.rotate(p);
    g.translate(-p.pos());
    for(auto& radialStrip : radialStrips) {
      g.draw(radialStrip);
    }
    g.popMatrix();
  }

};

class LeafLoops : public App {
public:
  LeafLooper lls[NUM_LEAF_LOOPERS];
  State state;
  cuttlebone::Taker<State> taker;
  unsigned framenum = 0;

  LeafLoops() {
    initWindow(Window::Dim(900, 600), "Leaf Loops");
    nav().pos(0, 0, 5);
    nav().faceToward(Vec3d(0, 0, 0), Vec3d(0, 1, 0));
    lls[0].p.pos(-1, 0, 0);
    lls[0].p.faceToward(Vec3d(0, 0, 5), Vec3d(0, 1, 0));
    lls[1].p.pos(1, 0, 0);
    lls[1].p.faceToward(Vec3d(0, 0, 5), Vec3d(0, 1, 0));
  }

  void onAnimate(double dt) override {
    taker.get(state);
    for(; framenum < state.framenum; framenum++) {
      for(int whichlooper=0; whichlooper < NUM_LEAF_LOOPERS; ++whichlooper) {
        LeafLooperData& llData = state.llDatas[whichlooper];
        lls[whichlooper].p = llData.p;
        if(state.framenum - framenum <= REDUNDANCY) {
          StripPseudoMesh& thisStrip = llData.latestStrips[REDUNDANCY - (state.framenum - framenum)];
          
          Mesh newStrip;
          newStrip.primitive(Graphics::TRIANGLE_STRIP);
          for(int i = 0; i < FFT_SIZE; ++i) {
            newStrip.vertex(thisStrip.vertices[i]);
            newStrip.color(thisStrip.colors[i]);
          }
          lls[whichlooper].pushNewStrip(newStrip);
        }
      }
    }
  }

  void onDraw(Graphics& g) override {
    for(LeafLooper& ll : lls) {
      ll.draw(g);
    }
  }

  void onKeyDown (const Keyboard &k) override {
    switch(k.key()) {
      default:
        break;
    }
  }
};

int main() {
  LeafLoops app;
  app.taker.start();
  app.start();
}
