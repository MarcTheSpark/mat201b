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


// NOTE FOR KARL: Some things are wrong here, but I don't have time to fix it before 4, it'll probably be fixed before tomorrow.
// For some reason the OmniStereoRenderer doesn't seem to be in the same location as the regular renderer used in the simulator
// It looks like everythin is far away. At any rate, the pose of the simulator is copied to the renderer, so you should be able to explore the
// shape being produced. Thanks!
// CONTROLS: Press 1 to pause playback, 2 to skip backwards 10 seconds in the soundfile, 3 to skip forwards 10 seconds, 4 to step forward one frame when paused



// Notes to self:

// TODO: make the two leaf shapes go in the same direction
// TODO: get leaves moving around us
// TODO: inherit from allo360whatever class instead

// A way to improve contrast, rhythmic information without making quiet ones disappear. Maybe subtracting last frame from frame?
// How do fragment shaders do fractals? How do they know their surroundings, I guess...
// Will animate: leaf size, leaf orientation (where the top is pointing), color based on structural parameters
// THOSE TOOLKITS
// Did you figure out sound?
// Maybe use the other leaf as well, such a different shape

// Middle ground: extrude as a structure over space


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

class LeafOscillator {
  // LeafOscillator abstract class
   public:
      // pure virtual function
      virtual float getAngle(float phase) = 0;
      virtual float getRadius(float phase) = 0;
};


struct SingleLeafOscillator : LeafOscillator {
  vector<float> angles;
  vector<float> radii;
  float phaseAdjustment;
  SingleLeafOscillator(string dataFileName) {
    std::vector<float> data;
    ifstream input(fullPathOrDie(dataFileName));
    for( string line; getline( input, line ); ) {
        data.push_back(stof(line));
    }
    for(int i = 0; i < data.size(); i += 2) {
      Vec2f displacement(data.at(i), data.at(i+1));
      angles.push_back(atan2(displacement.y, displacement.x));
      radii.push_back(displacement.mag());
    }
  }

  float getAngle(float phase) {
    return angles.at(int((phase - floor(phase)) * angles.size()));
  }

  float getRadius(float phase) {
    return radii.at(int((phase - floor(phase)) * radii.size()));
  }
};

void loadDownbeats(vector<float>& downbeats) {
  ifstream input( fullPathOrDie("LeafLoopsDownbeats.txt" ));
  for( string line; getline( input, line ); ) {
      downbeats.push_back(stof(line));
  }
};

struct CombinedLeafOscillator : LeafOscillator {
  LeafOscillator& lfo1;
  LeafOscillator& lfo2;
  float weighting = 0;  // weighting of 0 means all lfo1, of 1 means all lfo2
  CombinedLeafOscillator(LeafOscillator& _lfo1, LeafOscillator& _lfo2) :
    lfo1(_lfo1), lfo2(_lfo2) 
  {}  

  void setWeighting(float w) {
    assert(w >= 0 && w <= 1);
    weighting = w;
  }

  float getAngle(float phase) {
    return (1 - weighting) * lfo1.getAngle(phase) + weighting * lfo2.getAngle(phase);
  }

  float getRadius(float phase) {
    return (1 - weighting) * lfo1.getRadius(phase) + weighting * lfo2.getRadius(phase);
  }
};

SingleLeafOscillator ivyOscillator("LeafDisplacements1.txt");
SingleLeafOscillator birchOscillator("LeafDisplacements2.txt");
CombinedLeafOscillator comboOscillator1(ivyOscillator, birchOscillator);
CombinedLeafOscillator comboOscillator2(ivyOscillator, birchOscillator);

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

  LeafOscillator& lfo;
  vector<float> binRadii;
  vector<float> fftMagnitudes;

  // DEBUG
  Mesh directionCone;
  bool showDirectionCone = false;

  LeafLooper(LeafOscillator& _lfo) 
  : stft(
    FFT_SIZE, FFT_SIZE/4,  // Window size, hop size
    0, HANN, COMPLEX 
  ), lfo(_lfo)
  {
    for(int i = 0; i < FFT_SIZE / 2; i++) {
        float freq = float(i) / FFT_SIZE * SAMPLE_RATE;
        binRadii.push_back((log(freq) - log(20)) / (log(centerFrequency) - log(20)) * centerRadius);  
    }
    fftMagnitudes.resize(FFT_SIZE/2);

    // DEBUG
    addCone(directionCone, 0.1, Vec3f(0, 0, -0.8));  // by default we treat objects as facing in the negative z direction
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

  void pushNewStrip(float phase, float phase2) {
    // ADD A NEW STRIP OF VERTICES AND COLORS
    Buffer<Vec3f> newRadialStripVertices;
    Buffer<Color> newRadialStripColors;
    for(int i = 0; i < FFT_SIZE / 2; i++) {
      float radius = binRadii.at(i) * lfo.getRadius(phase);
      float angle = lfo.getAngle(phase);
      // loud bins get visualized with random displacement of the second angle
      float adjustedPhase2 = phase2 + rnd::uniformS() * 0.5 * fftMagnitudes.at(i); // 0.5*M_PI; 
      // add randomness to phase 2, and clip it
      newRadialStripVertices.append(Vec3f(-radius*cos(angle)*sin(adjustedPhase2), radius*sin(angle), radius*cos(adjustedPhase2)));
      newRadialStripColors.append(Color(1, 1, 1, fftMagnitudes.at(i)));
    }
    radialStripVertices.push_back(newRadialStripVertices);
    radialStripColors.push_back(newRadialStripColors);
    pushNewStripMesh();
  }

  private:
  void pushNewStripMesh() {
    // Construct a new strip mesh out of the last two sets of vertices and colors we added
    if(radialStripVertices.size() > 1) {
      Buffer<Vec3f>& newRadialStripVertices = radialStripVertices.at(radialStripVertices.size()-1);
      Buffer<Color>& newRadialStripColors = radialStripColors.at(radialStripColors.size()-1);
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
};

class LeafLoops : public App {
public:

  State state;
  cuttlebone::Maker<State> maker;

  SamplePlayer<> anaylsisPlayer, playbackPlayer;
  bool paused = false, doOneFrame = false;
  LeafLooper ll1, ll2;

  float currentMeasurePhase = 0;

  vector<float> downbeats;
  float lastTimeTillDownbeat = 100000;

  bool firstDrawDone = false;

  LeafLoops() : ll1(comboOscillator1), ll2(comboOscillator2) {
    anaylsisPlayer.load(fullPathOrDie(ANALYSIS_SOUND_FILE_NAME).c_str());
    anaylsisPlayer.pos(FFT_SIZE); // give the analysisPlayer a headstart of FFT_SIZE, to compensate for the lag in analysis
    playbackPlayer.load(fullPathOrDie(PLAYBACK_SOUND_FILE_NAME).c_str());
    initWindow(Window::Dim(900, 600), "Leaf Loops");
    initAudio(SAMPLE_RATE);
    nav().pos(0, 0, 0);
    nav().faceToward(Vec3d(0, 0, -1), Vec3d(0, 1, 0));
    ll1.p.pos(0, 0, -3);
    ll1.p.faceToward(Vec3d(0, 0, 0), Vec3d(0, 1, 0));
    ll2.p.pos(0, 0, 3);
    ll2.p.faceToward(Vec3d(0, 0, 0), Vec3d(0, 1, 0));

    loadDownbeats(downbeats);

    ll1.showDirectionCone = false;
    paused = false;
  }

  void onAnimate(double dt) override {
    if (!paused || doOneFrame) {
      doOneFrame = false;
      float measurePhase = getMeasurePhase(dt);

      ll1.pushNewStrip(currentMeasurePhase, getTime()*4);
      ll2.pushNewStrip(currentMeasurePhase, getTime()*4);

      sendDataToCuttlebone();
    }
  }

  float getMeasurePhase(double dt) {
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
    // returns currentMeasurePhase, but obviously currentMeasurePhase could also just be accessed directly
    return currentMeasurePhase;
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

  void sendDataToCuttlebone() {
    // transfer the poses to the cuttlebone-friendly data structure of each leaf looper
    ll1.llData.p = ll1.p;
    ll2.llData.p = ll2.p;
    // set those to cuttlebone-friendly data structures in the cuttlebone maker
    state.llDatas[0] = ll1.llData;
    state.llDatas[1] = ll2.llData;
    // increment framenum
    state.framenum++;
    state.navPose = nav();
    // send it along!
    maker.set(state);
  }

  void onDraw(Graphics& g) override {
    ll1.draw(g);
    ll2.draw(g);
    firstDrawDone = true;
  }

  void onSound(AudioIOData& io) override {
    if(!firstDrawDone || (paused && !doOneFrame)) { return; }
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
      case '2':
        anaylsisPlayer.pos(std::max(anaylsisPlayer.pos() - 10 * SAMPLE_RATE, 0.0));
        playbackPlayer.pos(std::max(playbackPlayer.pos() - 10 * SAMPLE_RATE, 0.0));
        break;
      case '3':
        anaylsisPlayer.pos(std::min(anaylsisPlayer.pos() + 10 * SAMPLE_RATE, double(anaylsisPlayer.frames())));
        playbackPlayer.pos(std::min(playbackPlayer.pos() + 10 * SAMPLE_RATE, double(playbackPlayer.frames())));
        break;
      case '4':
        doOneFrame = true;
        break;
      case '5':
        comboOscillator2.setWeighting(std::max(comboOscillator2.weighting - 0.05, 0.0));
        break;
      case '6':
        comboOscillator2.setWeighting(std::min(comboOscillator2.weighting + 0.05, 1.0));
        break;
    }
  }
};

int main() {
  LeafLoops app;
  app.maker.start();
  app.start();
}
