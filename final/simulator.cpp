/*
  Marc Evans (2018/3/8)
  Final Project Simulator
*/

#define ANALYSIS_SOUND_FILE_NAME ("EvansLeafLoopsDryDPA.ogg")
#define PLAYBACK_SOUND_FILE_NAME ("EvansLeafLoopsFinal.ogg")
#define SAMPLE_RATE (48000)
#define FFT_SIZE (1024)
#define REDUNDANCY (5)
#define VISUAL_DECAY (0.8)

#include <cassert>
#include <iostream>
#include <fstream>
#include <deque>
#include "allocore/io/al_App.hpp"
#include "Gamma/SamplePlayer.h"
#include "Gamma/DFT.h"
#include "Cuttlebone/Cuttlebone.hpp"
#include "common.hpp"
using namespace al;
using namespace std;
using namespace gam;


// A way to improve contrast, rhythmic information without making quiet ones disappear. Maybe subtracting last frame from frame?
// How do fragment shaders do fractals? How do they know their surroundings, I guess...
// Will animate: leaf size, leaf orientation (where the top is pointing), color based on structural parameters
// THOSE TOOLKITS
// Did you figure out sound?
// Maybe use the other leaf as well, such a different shape

// Middle ground: extrude as a structure over space


vector<float> loadLeafRadii(int rotation=0) {
  vector<float> leafRadii;
  ifstream input( "marc.evans/final/LeafRadii.txt" );
  for( string line; getline( input, line ); ) {
      leafRadii.push_back(stof(line));
  }
  std::rotate(leafRadii.begin(), leafRadii.begin() + (rotation % 360 + 360) % 360, leafRadii.end());
  return leafRadii;
};

void loadDownbeats(vector<float>& downbeats) {
  ifstream input( "marc.evans/final/downbeats.txt" );
  for( string line; getline( input, line ); ) {
      downbeats.push_back(stof(line));
  }
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

  LeafLooperData llData;  // The cuttlebone struct for this lil guy

  float centerFrequency = 4000;
  float centerRadius = 1;

  vector<float> binRadii;
  vector<float> leafRadiiByAngle;
  vector<float> fftMagnitudes;

  // Debug
  Mesh directionCone;
  bool showDirectionCone;

  LeafLooper(vector<float> leafRadii, bool showDirectionCone=false) : stft(
    FFT_SIZE, FFT_SIZE/4,  // Window size, hop size
    0, HANN, COMPLEX 
  ) {
    for(int i = 0; i < FFT_SIZE / 2; i++) {
        float freq = float(i) / FFT_SIZE * SAMPLE_RATE;
        binRadii.push_back((log(freq) - log(20)) / (log(centerFrequency) - log(20)) * centerRadius);  
    }
    for (auto radius : leafRadii) { leafRadiiByAngle.push_back(radius); }
    fftMagnitudes.resize(FFT_SIZE/2);

    // Debug
    this->showDirectionCone = showDirectionCone;
    addCone(directionCone, 0.1, Vec3f(0, 0, -0.8));  // by default we treat objects as facing in the negative z direction
  }

  void pushNewStrip(float phase, float phase2) {
    Buffer<Vec3f> newRadialStripVertices;
    Buffer<Color> newRadialStripColors;
    for(int i = 0; i < FFT_SIZE / 2; i++) {
      float radius = binRadii.at(i) * leafRadiiByAngle.at(int(phase*360) % 360);
      float angle = phase*2*M_PI;
      // loud bins get visualized with random displacement of the second angle
      float adjustedPhase2 = phase2 + rnd::uniformS() * 0.5 * fftMagnitudes.at(i);
      // add randomness to phase 2, and clip it
      newRadialStripVertices.append(Vec3f(-radius*cos(angle)*sin(adjustedPhase2), radius*sin(angle), radius*cos(adjustedPhase2)));
      newRadialStripColors.append(Color(1, 1, 1, fftMagnitudes.at(i)));
    }
    radialStripVertices.push_back(newRadialStripVertices);
    radialStripColors.push_back(newRadialStripColors);
    if(radialStripVertices.size() > 1) {
      Buffer<Vec3f>& lastRadialStripVertices = radialStripVertices.at(radialStripVertices.size()-2);
      Buffer<Color>& lastRadialStripColors = radialStripColors.at(radialStripColors.size()-2);
      Mesh radialStrip;

      llData.shiftStrips();
      StripPseudoMesh& newestPseudoStrip = llData.latestStrips[REDUNDANCY-1];

      radialStrip.primitive(Graphics::TRIANGLE_STRIP);
      for(int i = 0; i < FFT_SIZE / 2; i++) {
        radialStrip.vertex(lastRadialStripVertices[i]);
        newestPseudoStrip.vertices[2*i] = lastRadialStripVertices[i];
        Color lc = lastRadialStripColors[i];
        lc.a *= VISUAL_DECAY;
        radialStrip.color(lc);
        newestPseudoStrip.colors[2*i] = lastRadialStripColors[i];
        radialStrip.vertex(newRadialStripVertices[i]);
        newestPseudoStrip.vertices[2*i + 1] = newRadialStripVertices[i];
        radialStrip.color(newRadialStripColors[i]);
        newestPseudoStrip.colors[2*i + 1] = newRadialStripColors[i];
      }
      for(int i = 0; i < radialStrips.size(); ++i) {
        for(auto& color : radialStrips.at(i).colors()) {
          color.a *= VISUAL_DECAY;
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
    g.pushMatrix();
    g.blendOn();
    g.blendModeTrans();
    g.rotate(p);
    g.translate(-p.pos());
    for(auto& radialStrip : radialStrips) {
      g.draw(radialStrip);
    }
    if(showDirectionCone) {
      g.draw(directionCone);
    }
    g.popMatrix();
  }

  void operator()(float s) {
    if(stft(s)) {
      // Loop through all the bins
      for(unsigned k=0; k<stft.numBins(); ++k){
        // higher pow reduces visual decay time
        // multiplier gets it to roughly the right level
        // tanh squashes it between 0 and 1
        fftMagnitudes[k] = tanh(pow(stft.bin(k).mag(), 1.3) * 1000.0);
      }
    }
  }
};

class LeafLoops : public App {
public:
  
  // DEBUG
  Mesh centerBox;

  State state;
  cuttlebone::Maker<State> maker;

  SamplePlayer<> anaylsisPlayer, playbackPlayer;
  bool paused = false, doOneFrame = false;
  LeafLooper ll1, ll2;

  float currentMeasurePhase = 0;

  vector<float> downbeats;
  float lastTimeTillDownbeat = 100000;

  bool firstDrawDone = false;

  LeafLoops() : ll1(loadLeafRadii(-17)), ll2(loadLeafRadii(-17)) {
    anaylsisPlayer.load(fullPathOrDie(ANALYSIS_SOUND_FILE_NAME, "..").c_str());
    playbackPlayer.load(fullPathOrDie(PLAYBACK_SOUND_FILE_NAME, "..").c_str());
    initWindow(Window::Dim(900, 600), "Leaf Loops");
    initAudio(SAMPLE_RATE);
    nav().pos(0, 0, 5);
    nav().faceToward(Vec3d(0, 0, 0), Vec3d(0, 1, 0));
    ll1.p.pos(-1, 0, 0);
    ll1.p.faceToward(Vec3d(0, 0, 5), Vec3d(0, 1, 0));
    ll2.p.pos(1, 0, 0);
    ll2.p.faceToward(Vec3d(0, 0, 5), Vec3d(0, 1, 0));

    loadDownbeats(downbeats);

    // DEBUG
    centerBox.vertex(-0.1, -0.1, 0);
    centerBox.vertex(-0.1, 0.1, 0);
    centerBox.vertex(0.1, -0.1, 0);
    centerBox.vertex(0.1, 0.1, 0);
    centerBox.primitive(Graphics::TRIANGLE_STRIP);
  }

  float getNextDownbeat() {
    float t = getTime();
    for (int i = 0; i < downbeats.size(); ++i)
    {
      if (downbeats[i] > t) return downbeats[i];
    }
    return t + 1; // not sure if this is a good way to end the piece
  }

  float getTime() {
    return playbackPlayer.pos() / SAMPLE_RATE;
  }

  void onAnimate(double dt) override {
    if (!paused || doOneFrame) {
      {
        float timeTillDownbeat = getNextDownbeat() - getTime();
        float phaseLeftInMeasure = 1 - currentMeasurePhase;
        if (timeTillDownbeat < lastTimeTillDownbeat && timeTillDownbeat > dt) {
          // if timeTillDownbeat went up, then we must have passed a barline, so we should reset phase
          // Also, if dt >= timeTillDownbeat, then we are currently passing a barline, so we should reset phase
          // otherwise increment phase
          currentMeasurePhase += phaseLeftInMeasure * dt / timeTillDownbeat;
        } else {
          currentMeasurePhase = 0;
        }
        lastTimeTillDownbeat = timeTillDownbeat;
        // cout << currentMeasurePhase << endl;
      }
      doOneFrame = false;
      ll1.pushNewStrip(currentMeasurePhase + 0.25, getTime()*4);
      ll2.pushNewStrip(currentMeasurePhase + 0.25, getTime()*4);
      ll1.llData.p = ll1.p;
      ll2.llData.p = ll2.p;
      state.llDatas[0] = ll1.llData;
      state.llDatas[1] = ll2.llData;
      state.framenum++;
      maker.set(state);
    }
  }

  void onDraw(Graphics& g) override {
    ll1.draw(g);
    ll2.draw(g);
    firstDrawDone = true;
    // DEBUG
    // g.draw(centerBox);
  }

  void onSound(AudioIOData& io) override {
    if(!firstDrawDone) { return; }
    gam::Sync::master().spu(audioIO().fps());
    while (io()) {
      for(int chan = 0; chan < 2; ++chan) {
        float sampForAnalysis = anaylsisPlayer.read(chan);
        float sampForPlayback = playbackPlayer.read(chan);
        if(chan == 0) { ll1(sampForAnalysis); }
        if(chan == 1) { ll2(sampForAnalysis); }
        io.out(chan) = sampForPlayback;
      }
      anaylsisPlayer.advance();
      playbackPlayer.advance();
    }
  }

  void onKeyDown (const Keyboard &k) override {
    switch(k.key()) {
      default:
        break;
      case '1':
        paused = !paused;
        break;
    }
  }
};

int main() {
  LeafLoops app;
  app.maker.start();
  app.start();
}
