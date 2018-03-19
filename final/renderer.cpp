/*
  Marc Evans (2018/3/8)
  Final Project Renderer
*/

#include <cassert>
#include <iostream>
#include <fstream>

#include "common.hpp"
#include "alloutil/al_OmniStereoGraphicsRenderer.hpp"

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

  Mesh trail;
  deque<Vec3f> trailVertices;
  deque<Color> trailColors;
  int maxTrailLength;
  float trailAlphaDecayFactor;
  bool doTrail;

  Color llColor;

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
    trail.primitive(Graphics::TRIANGLE_STRIP);
  }

  void pushNewTrailPoints(PseudoMesh<NUM_TRAIL_POINTS_PER_FRAME>& newTrailPoints) {
    for(int i=0; i < NUM_TRAIL_POINTS_PER_FRAME; ++i) {
      trailVertices.push_back(newTrailPoints.vertices[i]);
      trailColors.push_back(newTrailPoints.colors[i]);
    }

    while(trailVertices.size() > maxTrailLength) {
      trailVertices.pop_front();
      trailColors.pop_front();
    }

    for (Color& c : trailColors) { c.a *= trailAlphaDecayFactor; }

    trail.vertices().reset();
    trail.colors().reset();

    for(int i = 0; i < trailVertices.size() - NUM_TRAIL_POINTS_PER_FRAME; ++i) { 
      trail.vertex(trailVertices.at(i));
      trail.color(trailColors.at(i));
      trail.vertex(trailVertices.at(i + NUM_TRAIL_POINTS_PER_FRAME)); 
      trail.color(trailColors.at(i + NUM_TRAIL_POINTS_PER_FRAME));
    }
  }

  void draw(Graphics& g) {
    g.pushMatrix();
    g.blendOn();
    g.blendModeTrans();
    g.translate(p.pos());
    g.rotate(p);
    for(auto& radialStrip : radialStrips) {
      g.draw(radialStrip);
    }
    g.popMatrix();
    if(doTrail) {
      g.draw(trail);
    }
  }
};

class LeafLoops : public OmniStereoGraphicsRenderer {
public:
  LeafLooper lls[NUM_LEAF_LOOPERS];
  State state;
  cuttlebone::Taker<State> taker;
  unsigned framenum = 0;

  LeafLoops() {
    initWindow(Window::Dim(900, 600), "Leaf Loops");
    pose.pos(0, 0, 0);
    pose.faceToward(Vec3d(0, 0, -1), Vec3d(0, 1, 0));
    lls[0].p.pos(-1, 0, -7);
    lls[0].p.faceToward(Vec3d(0, 0, 0), Vec3d(0, 1, 0));
    lls[1].p.pos(1, 0, -7);
    lls[1].p.faceToward(Vec3d(0, 0, 0), Vec3d(0, 1, 0));
  }

  void onAnimate(double dt) override {
    taker.get(state);
    pose.set(state.navPose);
    omni().clearColor() = state.bgColor;
    for(; framenum < state.framenum; framenum++) {
      for(int whichlooper=0; whichlooper < NUM_LEAF_LOOPERS; ++whichlooper) {
        LeafLooperData& llData = state.llDatas[whichlooper];
        lls[whichlooper].p = llData.p;
        lls[whichlooper].maxTrailLength = llData.maxTrailLength;
        lls[whichlooper].trailAlphaDecayFactor = llData.trailAlphaDecayFactor;
        lls[whichlooper].doTrail = llData.doTrail;

        if(state.framenum - framenum <= REDUNDANCY) {
          PseudoMesh<FFT_SIZE>& thisStrip = llData.latestStrips[REDUNDANCY - (state.framenum - framenum)];
          
          Mesh newStrip;
          newStrip.primitive(Graphics::TRIANGLE_STRIP);
          for(int i = 0; i < FFT_SIZE; ++i) {
            newStrip.vertex(thisStrip.vertices[i]);
            newStrip.color(thisStrip.colors[i]);
          }
          lls[whichlooper].pushNewStrip(newStrip);
        }
        if(state.framenum - framenum <= REDUNDANCY) {
          PseudoMesh<NUM_TRAIL_POINTS_PER_FRAME>& theseTrailPoints = llData.latestTrailPoints[REDUNDANCY - (state.framenum - framenum)];
          lls[whichlooper].pushNewTrailPoints(theseTrailPoints);
        }
      }
    }
  }

  void onDraw(Graphics& g) override {
    // you may need these later
    // shader().uniform("texture", 1.0);
    // shader().uniform("lighting", 1.0);
    //
    for(LeafLooper& ll : lls) {
      ll.draw(g);
    }
  }
};

int main() {
  LeafLoops app;
  app.taker.start();
  app.start();
}
