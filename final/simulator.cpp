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
#define MIN_DIST (3)

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


// CONTROLS: Press 1 to pause playback, 2 to skip backwards 10 seconds in the soundfile, 3 to skip forwards 10 seconds, 4 to step forward one frame when paused


// Notes to self:

// TODO: figure out why hybrid leaf jumps
// TODO: make leaf motion code cleaner
// TODO: try extruding something.

// Will animate: leaf size, leaf orientation (where the top is pointing), color based on structural parameters
// THOSE TOOLKITS
// Did you figure out sound?
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

void loadDownbeats(vector<float>& downbeats) {
  ifstream input( fullPathOrDie("LeafLoopsDownbeats.txt" ));
  for( string line; getline( input, line ); ) {
      downbeats.push_back(stof(line));
  }
};

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
CombinedLeafOscillator ll1ComboOscillator(ivyOscillator, birchOscillator);
CombinedLeafOscillator ll2ComboOscillator(ivyOscillator, birchOscillator);
CombinedLeafOscillator ll1HorizontalMotionComboOscillator(ivyOscillator, birchOscillator);
CombinedLeafOscillator ll2HorizontalMotionComboOscillator(ivyOscillator, birchOscillator);
CombinedLeafOscillator ll1VerticalMotionComboOscillator(ivyOscillator, birchOscillator);
CombinedLeafOscillator ll2VerticalMotionComboOscillator(ivyOscillator, birchOscillator);

struct LeafLooper {
  Pose p;
  STFT stft;
  deque<Buffer<Vec3f>> radialStripVertices;
  deque<Buffer<Color>> radialStripColors;
  deque<Mesh> radialStrips;

  Mesh history;
  bool trackHistory = false;

  int maxStrips = 60;

  LeafLooperData llData;  // The cuttlebone struct for this lil guy

  float centerFrequency = 4000;
  float centerRadius = 1;

  LeafOscillator& lfo;
  vector<float> binRadii;
  vector<float> fftMagnitudes;

  Color llColor;

  // DEBUG
  Mesh directionCone;
  bool showDirectionCone = false;

  LeafLooper(LeafOscillator& _lfo, Color _llColor) 
  : stft(
    FFT_SIZE, FFT_SIZE/4,  // Window size, hop size
    0, HANN, COMPLEX 
  ), lfo(_lfo), llColor(_llColor)
  {
    for(int i = 0; i < FFT_SIZE / 2; i++) {
        float freq = float(i) / FFT_SIZE * SAMPLE_RATE;
        binRadii.push_back((log(freq) - log(20)) / (log(centerFrequency) - log(20)) * centerRadius);  
    }
    fftMagnitudes.resize(FFT_SIZE/2);

    history.primitive(Graphics::TRIANGLE_STRIP);

    // DEBUG
    addCone(directionCone, 0.1, Vec3f(0, 0, -0.8));  // by default we treat objects as facing in the negative z direction
    directionCone.generateNormals();
  }

  void draw(Graphics& g) {
    g.color(llColor);
    g.blendOn();
    g.blendModeTrans();
    g.pushMatrix();

    g.translate(p.pos());
    g.rotate(p);
    for(auto& radialStrip : radialStrips) {
      g.draw(radialStrip);
    }
    if(showDirectionCone) {
      g.draw(directionCone);
    }
    g.popMatrix();
    if(trackHistory) {
      g.draw(history);
    }
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
    float maxMag = 0;
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

      if(fftMagnitudes.at(i) > maxMag) { maxMag = fftMagnitudes.at(i); }
    }
    radialStripVertices.push_back(newRadialStripVertices);
    radialStripColors.push_back(newRadialStripColors);
    pushNewStripMesh();
    if(trackHistory) {
      history.vertex(p.pos() + Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * maxMag / 10 );
      history.color(Color(llColor.r, llColor.g, llColor.b, maxMag));
      history.vertex(p.pos() + Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * maxMag / 10 );
      history.color(Color(llColor.r, llColor.g, llColor.b, maxMag));
    }
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

  // Motion
  float ll1HorizontalMotionPhase = 0, ll1VerticalMotionPhase = 0;
  float ll1HorizontalMotionFreq = 0.08, ll1VerticalMotionFreq = 0.05;
  float ll1HMotionRadiusMul = 8.0, ll1VMotionRadiusMul = 4.0;

  float ll2HorizontalMotionPhase = 0, ll2VerticalMotionPhase = 0;
  float ll2HorizontalMotionFreq = 0.08, ll2VerticalMotionFreq = 0.05;
  float ll2HMotionRadiusMul = 8.0, ll2VMotionRadiusMul = 4.0;

  LeafLoops() : ll1(ll1ComboOscillator, Color(0.8, 0.8, 0.0)), ll2(ll2ComboOscillator, Color(0.0, 0.8, 0.8)) {
    anaylsisPlayer.load(fullPathOrDie(ANALYSIS_SOUND_FILE_NAME).c_str());
    anaylsisPlayer.pos(FFT_SIZE); // give the analysisPlayer a headstart of FFT_SIZE, to compensate for the lag in analysis
    playbackPlayer.load(fullPathOrDie(PLAYBACK_SOUND_FILE_NAME).c_str());
    initWindow(Window::Dim(900, 600), "Leaf Loops");
    initAudio(SAMPLE_RATE);
    nav().pos(0, 20, 0);
    nav().faceToward(Vec3d(0, 0, 0), Vec3d(0, 0, -1));
    ll1.p.pos(0, 0, -3);
    ll1.p.faceToward(Vec3d(0, 0, 0), Vec3d(0, 1, 0));
    ll2.p.pos(0, 0, 3);
    ll2.p.faceToward(Vec3d(0, 0, 0), Vec3d(0, 1, 0));

    loadDownbeats(downbeats);

    initializeMotionVariables();

    paused = false;
  }

  void initializeMotionVariables() {
    ll1VerticalMotionComboOscillator.weighting = 0;
    ll1HorizontalMotionComboOscillator.weighting = 0;
    ll1HorizontalMotionPhase = 0, ll1VerticalMotionPhase = 0;

    ll2VerticalMotionComboOscillator.weighting = 0;
    ll2HorizontalMotionComboOscillator.weighting = 0;
    ll2HorizontalMotionPhase = 0.5, ll2VerticalMotionPhase = 0.0;
  }

  void onAnimate(double dt) override {
    if (!paused || doOneFrame) {
      doOneFrame = false;
      float measurePhase = getMeasurePhase(dt);

      ll1.pushNewStrip(currentMeasurePhase, getTime()*4);
      ll2.pushNewStrip(currentMeasurePhase, getTime()*4);

      setLLPositions(dt);
      sendDataToCuttlebone();
    }
  }

  void setLLPositions(double dt) {
    { // ll1
      float ll1HAngle = ll1HorizontalMotionComboOscillator.getAngle(ll1HorizontalMotionPhase);
      float ll1HRadius = ll1HorizontalMotionComboOscillator.getRadius(ll1HorizontalMotionPhase);
      float ll1VAngle = ll1VerticalMotionComboOscillator.getAngle(ll1VerticalMotionPhase);
      float ll1VRadius = ll1VerticalMotionComboOscillator.getRadius(ll1VerticalMotionPhase);
      Pose newGoal;
      float goalX = cos(ll1HAngle) * ll1HRadius * ll1HMotionRadiusMul;
      float goalY = sin(ll1VAngle) * ll1VRadius * ll1VMotionRadiusMul;
      float goalZ = -sin(ll1HAngle) * cos(ll1VAngle) * ll1VRadius * ll1HRadius * ll1HMotionRadiusMul;
      float horizontalDist = hypot(goalX, goalZ);
      if(horizontalDist < MIN_DIST) { // ensure it doesn't get too close
          goalX *= MIN_DIST / horizontalDist;
          goalZ *= MIN_DIST / horizontalDist;
      }
      newGoal.pos(Vec3d(goalX, goalY, goalZ));
      newGoal.faceToward(Vec3d(0, 0, 0));
      ll1.p = ll1.p.lerp(newGoal, 0.01);
      ll1HorizontalMotionPhase += dt * ll1HorizontalMotionFreq;
      ll1VerticalMotionPhase += dt * ll1VerticalMotionFreq;
    }
    { // ll2  ::: THIS IS STUPID TO COPY!!!
      float ll2HAngle = ll2HorizontalMotionComboOscillator.getAngle(ll2HorizontalMotionPhase);
      float ll2HRadius = ll2HorizontalMotionComboOscillator.getRadius(ll2HorizontalMotionPhase);
      float ll2VAngle = ll2VerticalMotionComboOscillator.getAngle(ll2VerticalMotionPhase);
      float ll2VRadius = ll2VerticalMotionComboOscillator.getRadius(ll2VerticalMotionPhase);
      Pose newGoal;
      float goalX = cos(ll2HAngle) * ll2HRadius * ll2HMotionRadiusMul;
      float goalY = sin(ll2VAngle) * ll2VRadius * ll2VMotionRadiusMul;
      float goalZ = -sin(ll2HAngle) * cos(ll2VAngle) * ll2VRadius * ll2HRadius * ll2HMotionRadiusMul;
      float horizontalDist = hypot(goalX, goalZ);
      if(horizontalDist < MIN_DIST) { // ensure it doesn't get too close
          goalX *= MIN_DIST / horizontalDist;
          goalZ *= MIN_DIST / horizontalDist;
      }
      newGoal.pos(Vec3d(goalX, goalY, goalZ));
      newGoal.faceToward(Vec3d(0, 0, 0));
      ll2.p = ll2.p.lerp(newGoal, 0.01);
      ll2HorizontalMotionPhase += dt * ll2HorizontalMotionFreq;
      ll2VerticalMotionPhase += dt * ll2VerticalMotionFreq;
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
        ll1ComboOscillator.setWeighting(std::max(ll1ComboOscillator.weighting - 0.05, 0.0));
        break;
      case '6':
        ll1ComboOscillator.setWeighting(std::min(ll1ComboOscillator.weighting + 0.05, 1.0));
        break;
      case '-':
        nav().pos(0, 20, 0);
        nav().faceToward(Vec3d(0, 0, 0), Vec3d(0, 0, -1));
        break;
      case '=':
        nav().pos(0, 0, 0);
        nav().faceToward(Vec3d(0, 0, -1), Vec3d(0, 1, 0));
        break;
      case '0':
        ll1.trackHistory = !ll1.trackHistory;
        ll2.trackHistory = !ll2.trackHistory;
        break;
    }
  }
};

int main() {
  LeafLoops app;
  app.maker.start();
  app.start();
}

/*   SEGFAULT LLDB INFO:

Process 40775 stopped
* thread #9, name = 'com.apple.audio.IOThread.client', stop reason = EXC_BAD_ACCESS (code=1, address=0x3101c3a9)
    frame #0: 0x0000000003c032cb CoreAudio`AUConverterBase::RenderBus(unsigned int&, AudioTimeStamp const&, unsigned int, unsigned int) + 871
CoreAudio`AUConverterBase::RenderBus:
->  0x3c032cb <+871>: callq  *0x250(%rax)
    0x3c032d1 <+877>: xorl   %eax, %eax
    0x3c032d3 <+879>: cmpb   $0x0, -0x2d(%rbp)
    0x3c032d7 <+883>: je     0x3c032e5                 ; <+897>
Target 0: (_Users_mpevans_Documents_Winter2018_MAT201B_AlloSystem_marc_evans_final_simulator) stopped.

Also seems to be similar:
https://github.com/AudioNet/node-core-audio/issues/29
*/
