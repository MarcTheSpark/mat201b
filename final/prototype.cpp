/*
  Marc Evans (2018/2/20)
  Final Project Prototype
  FFT is not actually happening yet, but will be included soon.
*/

#define SOUND_FILE_NAME ("EvansLeafLoopsFinal.ogg")
#define SAMPLE_RATE (48000)
#define FFT_SIZE (1024)

#include <cassert>
#include <iostream>
#include <fstream>
#include <deque>
#include "allocore/io/al_App.hpp"
#include "Gamma/SamplePlayer.h"
#include "Gamma/DFT.h"
using namespace al;
using namespace std;
using namespace gam;

float frameDeltaT = 1./60;

vector<float> loadLeafRadii() {
  vector<float> leafRadii;
  ifstream input( "marc.evans/final/LeafRadii.txt" );
  for( string line; getline( input, line ); ) {
      leafRadii.push_back(stof(line));
  }
  return leafRadii;
};

string fullPathOrDie(string fileName, string whereToLook = ".") {
  SearchPaths searchPaths;
  searchPaths.addSearchPath(whereToLook);
  searchPaths.print();
  string filePath = searchPaths.find(fileName).filepath();
  if (filePath == "") {
    fprintf(stderr, "ERROR loading file \"\" \n");
    exit(-1);
  }
  return filePath;
}

struct LeafLooper {
  Pose p;
  STFT stft;
  deque<Buffer<Vec3f>> radialStripVertices;
  deque<Buffer<Color>> radialStripColors;
  deque<Mesh> radialStrips;
  int maxStrips = 60;

  float centerFrequency = 4000;
  float centerRadius = 1;

  vector<float> binRadii;
  vector<float> leafRadiiByAngle;
  vector<float> fftMagnitudes;

  LeafLooper(vector<float> leafRadii) : stft(
    FFT_SIZE, FFT_SIZE/4,  // Window size, hop size
    0, HANN, COMPLEX 
  ) {
    for(int i = 0; i < FFT_SIZE / 2; i++) {
        float freq = float(i) / FFT_SIZE * SAMPLE_RATE;
        binRadii.push_back((log(freq) - log(20)) / (log(centerFrequency) - log(20)) * centerRadius);  
    }
    for (auto radius : leafRadii) { leafRadiiByAngle.push_back(radius); }
    fftMagnitudes.resize(FFT_SIZE/2);
  }

  void pushNewStrip(float phase) {
    Buffer<Vec3f> newRadialStripVertices;
    Buffer<Color> newRadialStripColors;
    for(int i = 0; i < FFT_SIZE / 2; i++) {
      float radius = binRadii.at(i) * leafRadiiByAngle.at(int(phase*360) % 360);
      float angle = phase*2*M_PI;
      newRadialStripVertices.append(Vec3f(radius*cos(angle), radius*sin(angle), 0));
      newRadialStripColors.append(Color(1, 1, 1, fftMagnitudes.at(i)));
    }
    radialStripVertices.push_back(newRadialStripVertices);
    radialStripColors.push_back(newRadialStripColors);
    if(radialStripVertices.size() > 1) {
      Buffer<Vec3f>& lastRadialStripVertices = radialStripVertices.at(radialStripVertices.size()-2);
      Buffer<Color>& lastRadialStripColors = radialStripColors.at(radialStripColors.size()-2);
      Mesh radialStrip;
      radialStrip.primitive(Graphics::TRIANGLE_STRIP);
      for(int i = 0; i < FFT_SIZE / 2; i++) {
        radialStrip.vertex(lastRadialStripVertices[i]);
        Color lc = lastRadialStripColors[i];
        lc.a *= 0.94;
        radialStrip.color(lc);
        radialStrip.vertex(newRadialStripVertices[i]);
        radialStrip.color(newRadialStripColors[i]);
      }
      for(int i = 0; i < radialStrips.size(); ++i) {
        for(auto& color : radialStrips.at(i).colors()) {
          color.a *= 0.94;
        }
      }
      radialStrips.push_back(radialStrip);
    }
    if(radialStrips.size() > maxStrips) {
      radialStrips.pop_front();
      radialStripVertices.pop_front();
    }
  }

  void draw(Graphics& g) {
    g.blendOn();
    g.blendModeTrans();
    g.rotate(p);
    g.translate(p.pos());
    g.pushMatrix();
    for(auto& radialStrip : radialStrips) {
      g.draw(radialStrip);
    }
    g.popMatrix();
  }

  void operator()(float s) {
    if(stft(s)) {
      // Loop through all the bins
      for(unsigned k=0; k<stft.numBins(); ++k){
        fftMagnitudes[k] = stft.bin(k).mag() * 500.0;
      }
    }
  }
};

class LeafLoops : public App {
public:
  
  SamplePlayer<> player;
  bool paused = false, doOneFrame = false;
  LeafLooper ll1, ll2;
  float t = 0;

  LeafLoops() : ll1(loadLeafRadii()), ll2(loadLeafRadii()) {
    player.load(fullPathOrDie(SOUND_FILE_NAME, "..").c_str());
    initWindow(Window::Dim(900, 600), "Leaf Loops");
    initAudio(SAMPLE_RATE);
    nav().pos(0, 0, 5);
    nav().faceToward(0, 0, 0);
    ll1.p.pos(-3, 0, 0);
    ll2.p.pos(3, 0, 0);
  }

  void onAnimate(double dt) override {
    if (!paused || doOneFrame) {
      doOneFrame = false;
      ll1.pushNewStrip(t/2);
      ll2.pushNewStrip(t/2);
      t += frameDeltaT;
    }
  }

  void onDraw(Graphics& g) override {
    ll1.draw(g);
    ll2.draw(g);
  }

  void onSound(AudioIOData& io) override {
    gam::Sync::master().spu(audioIO().fps());
    while (io()) {
      for(int chan = 0; chan < 2; ++chan) {
        float s = player.read(chan);
        if(chan == 0) { ll1(s); }
        if(chan == 1) { ll2(s); }
        io.out(chan) = s;
      }
      player.advance();
    }
  }

  void onKeyDown (const Keyboard &k) override {
    switch(k.key()) {
      default:
        break;
      case '1':
        paused = !paused;
        break;
      case '2':
        frameDeltaT *= 2;
        break;
      case '3':
        frameDeltaT /= 2;
        break;
      case '4':
        doOneFrame = true;
        break;
    }
  }
};

int main() {
  LeafLoops app;
  app.start();
}
