/*
  Marc Evans (2018/1/28)
*/


#include <cassert>
#include <iostream>
#include <fstream>
#include <deque>
#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;


int fftLength = 1024;
int sampleRate  = 44100;

vector<float> loadLeafRadii() {
  vector<float> leafRadii;
  ifstream input( "mat201b/LeafLoops/LeafRadii.txt" );
  for( string line; getline( input, line ); ) {
      leafRadii.push_back(stof(line));
  }
  return leafRadii;
}

struct LeafLooper {
  deque<Buffer<Vec3f>> radialStripVertices;
  deque<Buffer<Color>> radialStripColors;
  deque<Mesh> radialStrips;
  int maxStrips = 60;

  float centerFrequency = 4000;
  float centerRadius = 1;

  vector<float> binRadii;
  vector<float> leafRadiiByAngle;

  LeafLooper(vector<float> leafRadii) {
    for(int i = 0; i < fftLength / 2; i++) {
        float freq = float(i) / fftLength * sampleRate;
        binRadii.push_back((log(freq) - log(20)) / (log(centerFrequency) - log(20)) * centerRadius);  
    }
    leafRadiiByAngle = leafRadii;

  }
  void pushNewStrip(float phase, vector<float> fftMagnitudes) {
    Buffer<Vec3f> newRadialStripVertices;
    Buffer<Color> newRadialStripColors;
    for(int i = 0; i < fftLength / 2; i++) {
      float radius = binRadii[i] * leafRadiiByAngle[int(phase*360) % 360];
      float angle = phase*2*M_PI;
      Vec3f newVertex(radius*cos(angle), radius*sin(angle), 0);
      newRadialStripVertices.append(newVertex);
      // newRadialStripColors.append(Color(1, 1, 1, fftMagnitudes[i]));
    }
    // radialStripVertices.push_back(newRadialStripVertices);
    // radialStripColors.push_back(newRadialStripColors);
    // if(radialStripVertices.size() > 1) {
    //   Buffer<Vec3f>& lastRadialStripVertices = radialStripVertices[radialStripVertices.size()-2];
    //   Buffer<Color>& lastRadialStripColors = radialStripColors[radialStripColors.size()-2];
    //   Mesh radialStrip;
    //   radialStrip.primitive(Graphics::TRIANGLE_STRIP);
    //   for(int i = 0; i < fftLength / 2; i++) {
    //     radialStrip.vertex(lastRadialStripVertices[i]);
    //     Color lc = lastRadialStripColors[i];
    //     lc.a *= 0.94;
    //     radialStrip.color(lc);
    //     radialStrip.vertex(newRadialStripVertices[i]);
    //     radialStrip.color(newRadialStripColors[i]);
    //   }
    //   for(int i = 0; i < radialStrips.size(); ++i) {
    //     for(auto& color : radialStrips.at(i).colors()) {
    //       color.a *= 0.94;
    //     }
    //   }
    //   radialStrips.push_back(radialStrip);
    // }
    // if(radialStrips.size() > maxStrips) {
    //   radialStrips.pop_front();
    //   radialStripVertices.pop_front();
    // }
  }
  void draw(Graphics& g) {
    g.blendOn();
    g.blendModeTrans();
    for(auto& radialStrip : radialStrips) {
      g.draw(radialStrip);
    }
  }
};

class LeafLoops : public App {
public:

  // Mesh leaf;
  
  LeafLooper ll;
  float t;

  LeafLoops() : ll(loadLeafRadii()) { 
    // leaf.primitive(Graphics::LINE_LOOP);
    // leaf.stroke(2);

    // for(int i=0; i < 360; ++i) {
    //   leaf.vertex(Vec3f(cos(2*M_PI*i/360)*leafRadii[i], sin(2*M_PI*i/360)*leafRadii[i], 0));
    // }
  }

  void onAnimate(double dt) {
      vector<float> fftMagnitudes;
      for(int i = 0; i < 1024; i++) {
        fftMagnitudes.push_back(sin(sqrt(float(i)/512) * M_PI));
      }
      ll.pushNewStrip(t/2, fftMagnitudes);
      t += dt;
  }

  void onDraw(Graphics& g) {

    g.pushMatrix();

    g.translate(0, 0, -5);
    // g.draw(leaf);
    ll.draw(g);
    g.popMatrix();
  }

  void onKeyDown (const Keyboard &k) {
    switch(k.key()) {
      default:
        break;
    }
  }
};

int main() {
  LeafLoops app;
  app.initWindow(Window::Dim(900, 600), "Leaf Loops");
  app.start();
}
