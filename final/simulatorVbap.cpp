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
#define MIN_DIST (5)

#include <cassert>
#include <iostream>
#include <algorithm>
#include "Gamma/SamplePlayer.h"
#include "Gamma/DFT.h"
#include "common.hpp"
#include "utilityFunctions.hpp"
#include "leafOscillators.hpp"
#include "leafLooper.hpp"
#include "meterMaid.hpp"
#include "score.hpp"
#include "alloutil/al_AlloSphereAudioSpatializer.hpp"
#include "alloutil/al_Simulator.hpp"
using namespace al;
using namespace std;
using namespace gam;

#include "alloGLV/al_ControlGLV.hpp"
#include "GLV/glv.h"

// CONTROLS: Press 1 to pause playback, 2 to skip backwards 10 seconds in the soundfile, 3 to skip forwards 10 seconds, 4 to step forward one frame when paused


// Notes to self:
// make colors change per section
// y = -6 is a good place for the harmonics. Maybe they should be blue too.


// Will animate: leaf size, leaf orientation (where the top is pointing), color based on structural parameters
// THOSE TOOLKITS
// Did you figure out sound?
// Middle ground: extrude as a structure over space


CombinedLeafOscillator ll1ComboOscillator(ivyOscillator, birchOscillator);
CombinedLeafOscillator ll2ComboOscillator(ivyOscillator, birchOscillator);
CombinedLeafOscillator ll1HorizontalMotionComboOscillator(ivyOscillator, birchOscillator);
CombinedLeafOscillator ll2HorizontalMotionComboOscillator(ivyOscillator, birchOscillator);
CombinedLeafOscillator ll1VerticalMotionComboOscillator(ivyOscillator, birchOscillator);
CombinedLeafOscillator ll2VerticalMotionComboOscillator(ivyOscillator, birchOscillator);


class AllosphereSpatializerTweak : public AlloSphereAudioSpatializer {
public:
  void initSpatializationTweak() {

    if( bossa || audio ){
      // This is an AlloSphere audio machine
      mSpeakerLayout = new AlloSphereSpeakerLayout();
    } else {
      // mSpeakerLayout = new HeadsetSpeakerLayout();
      mSpeakerLayout = new SpeakerLayout();
      mSpeakerLayout->addSpeaker(Speaker(0, 45, 0, 1.0, 1.0));
      mSpeakerLayout->addSpeaker(Speaker(1, -45, 0, 1.0, 1.0));
    }

    // XXX select which spatializer to use via arguments
    // mSpatializer = new AmbisonicsSpatializer(*mSpeakerLayout,3,3);
    // mSpatializer = new Dbap(*mSpeakerLayout);
    mSpatializer = new Vbap(*mSpeakerLayout);

    mScene = new AudioScene(mAudioIO.framesPerBuffer());
    mListener = mScene->createListener(mSpatializer);
  }
};


struct LeafLoopsWidgets {
  GLVBinding gui;
  glv::Slider looperRadii, ll1LeafType, ll2LeafType, amplitudeExpansion;
  glv::ColorPicker bgColorPicker, ll1ColorPicker, ll2ColorPicker;
  // glv::Slider2D slider2d;
  glv::Table layout;

  LeafLoopsWidgets() {
    // Configure GUI
    gui.style().color.set(glv::Color(0.7), 0.5);

    layout.arrangement(">p");

    looperRadii.setValue(0.4);
    layout << looperRadii;
    layout << new glv::Label("Looper Radii");

    bgColorPicker.setValue(glv::Color(0, 0, 0));
    layout << bgColorPicker;
    layout << new glv::Label("BG Color");

    ll1ColorPicker.setValue(glv::Color(0.9375, 0.9375, 0.3125));
    layout << ll1ColorPicker;
    layout << new glv::Label("LL1 Color");

    ll2ColorPicker.setValue(glv::Color(0.39375, 0.875, 0.538125));
    layout << ll2ColorPicker;
    layout << new glv::Label("LL2 Color");

    ll1LeafType.setValue(0);
    layout << ll1LeafType;
    layout << new glv::Label("LL1 Leaf Type");

    ll2LeafType.setValue(0);
    layout << ll2LeafType;
    layout << new glv::Label("LL2 Leaf Type");

    amplitudeExpansion.setValue(0.31);
    layout << amplitudeExpansion;
    layout << new glv::Label("Amplitude Expansion");
    // slider2d.interval(-1,1);
    // layout << slider2d;
    // layout << new glv::Label("position");

    layout.arrange();

    gui << layout;
  } 

  float getLooperRadii() {
    return looperRadii.getValue()*4;
  }

  float getAmplitudeExpansion() {
    return amplitudeExpansion.getValue() * 2;
  }
};

struct LLMotion
{
  float horizontalRadiusMul, verticalRadiusMul;
  MeterMaid horizonalDownbeatPhasor, verticalDownbeatPhasor;
  CombinedLeafOscillator horizonalLeafOscillator, verticalLeafOscillator;

  LLMotion(MeterMaid& _horizonalDownbeatsPhasor, MeterMaid& _verticalDownbeatsPhasor, float _horizontalRadiusMul, float _verticalRadiusMul)
   : horizonalDownbeatPhasor(_horizonalDownbeatsPhasor), 
   verticalDownbeatPhasor(_verticalDownbeatsPhasor),
   horizontalRadiusMul(_horizontalRadiusMul),
   verticalRadiusMul(_verticalRadiusMul),
   horizonalLeafOscillator(ivyOscillator, birchOscillator),
   verticalLeafOscillator(ivyOscillator, birchOscillator)
  {
    horizonalLeafOscillator.setWeighting(0.7);
    verticalLeafOscillator.setWeighting(0.0);
  }

  Vec3f getPosition(float t) {
    float hAngle = horizonalLeafOscillator.getAngle(horizonalDownbeatPhasor.getPhasePosition(t));
    float hRadius = horizonalLeafOscillator.getRadius(horizonalDownbeatPhasor.getPhasePosition(t));
    float vAngle = verticalLeafOscillator.getAngle(verticalDownbeatPhasor.getPhasePosition(t));
    float vRadius = verticalLeafOscillator.getRadius(verticalDownbeatPhasor.getPhasePosition(t));

    float goalX = cos(hAngle) * hRadius * horizontalRadiusMul * (0.9 + pow(sin(horizonalDownbeatPhasor.getPhasePosition(t)*M_PI), 2));
    float goalY = sin(vAngle) * vRadius * verticalRadiusMul * (0.9 + pow(sin(verticalDownbeatPhasor.getPhasePosition(t)*M_PI), 2));
    float goalZ = -sin(hAngle) * cos(vAngle) * vRadius * hRadius * verticalRadiusMul;
    float horizontalDist = hypot(goalX, goalZ);
    if(horizontalDist < MIN_DIST) { // ensure it doesn't get too close
        goalX *= MIN_DIST / horizontalDist;
        goalZ *= MIN_DIST / horizontalDist;
    }
    return Vec3d(goalX, goalY, goalZ);
  }
};

struct LeafLoops : public App, AllosphereSpatializerTweak, InterfaceServerClient {

  State state;
  cuttlebone::Maker<State> maker;

  SamplePlayer<> anaylsisPlayer, playbackPlayer;
  bool paused = false, doOneFrame = false;
  LeafLooper ll1, ll2;

  MeterMaid beatCycleLookup, hyperbeatCycleLookup, smallSectionCycleLookup, bigSectionCycleLookup;
  Score score;

  LLMotion llMotion;

  bool firstDrawDone = false;
  bool turning = true;
  float turnSpeed = 0.001;

  LeafLoopsWidgets glvWidgets;

  LeafLoops() 
    : maker(Simulator::defaultBroadcastIP()),
      InterfaceServerClient(Simulator::defaultInterfaceServerIP()),
      ll1(ll1ComboOscillator, Color(0.9375, 0.9375, 0.3125)), 
      ll2(ll2ComboOscillator, Color(0.39375, 0.875, 0.538125)),
      beatCycleLookup("LeafLoopsDownbeats.txt"),
      hyperbeatCycleLookup("LeafLoopsHyperDownbeats.txt"),
      smallSectionCycleLookup("LeafLoopsSectionDownbeats.txt"),
      bigSectionCycleLookup("LeafLoopsBigSectionDownbeats.txt"),
      llMotion(smallSectionCycleLookup, bigSectionCycleLookup, 6.0, 4.5),
      score(ll1, ll2, nav(), beatCycleLookup, hyperbeatCycleLookup, smallSectionCycleLookup, 
        bigSectionCycleLookup, llMotion.horizontalRadiusMul, llMotion.verticalRadiusMul, turnSpeed) 
    {
    anaylsisPlayer.load(fullPathOrDie(ANALYSIS_SOUND_FILE_NAME).c_str());
    anaylsisPlayer.pos(FFT_SIZE); // give the analysisPlayer a headstart of FFT_SIZE, to compensate for the lag in analysis
    playbackPlayer.load(fullPathOrDie(PLAYBACK_SOUND_FILE_NAME).c_str());
    initWindow(Window::Dim(900, 600), "Leaf Loops");
    nav().pos(0, -3.0, 0);
    nav().faceToward(Vec3d(0, -3.0, -1), Vec3d(0, 1, 0));
    ll1.p.pos(0, 0, -3);
    ll1.p.faceToward(Vec3d(0, 0, 0), Vec3d(0, 1, 0));
    ll2.p.pos(0, 0, 3);
    ll2.p.faceToward(Vec3d(0, 0, 0), Vec3d(0, 1, 0));

    paused = false;

    // initAudio(SAMPLE_RATE);
    // audio
    AlloSphereAudioSpatializer::initAudio(SAMPLE_RATE);
    AllosphereSpatializerTweak::initSpatializationTweak();
    // if gamma
    gam::Sync::master().spu(AlloSphereAudioSpatializer::audioIO().fps());
    scene()->addSource(ll1);
    ll1.dopplerType(DOPPLER_NONE);
    scene()->addSource(ll2);
    ll2.dopplerType(DOPPLER_NONE);
    scene()->usePerSampleProcessing(true);
    // scene()->usePerSampleProcessing(false);

    // Connect GUI to window
    glvWidgets.gui.bindTo(window());
  }

  void onAnimate(double dt) override {
    while (InterfaceServerClient::oscRecv().recv()) {}

    if (!paused || doOneFrame) {
      doOneFrame = false;
      float measurePhase = beatCycleLookup.getPhasePosition(getTime());
      float hypermeasurePhase = hyperbeatCycleLookup.getPhasePosition(getTime());

      ll1.pushNewStrip(measurePhase, hypermeasurePhase);
      ll2.pushNewStrip(measurePhase, hypermeasurePhase);

      setLLPositions();
      score.setFromTime(getTime());
      sendDataToCuttlebone();
    }

    // checkGLVWidgets();
  }

  void checkGLVWidgets() {
    background(HSV(glvWidgets.bgColorPicker.getValue().components));
    state.bgColor = HSV(glvWidgets.bgColorPicker.getValue().components);
    ll1.llColor = HSV(glvWidgets.ll1ColorPicker.getValue().components);
    ll2.llColor = HSV(glvWidgets.ll2ColorPicker.getValue().components);
    ll1.setBinRadii(glvWidgets.getLooperRadii());
    ll2.setBinRadii(glvWidgets.getLooperRadii());
    ll1.lfo.setWeighting(glvWidgets.ll1LeafType.getValue());
    ll2.lfo.setWeighting(glvWidgets.ll2LeafType.getValue());
    ll1.amplitudeExpansion = glvWidgets.getAmplitudeExpansion();
    ll2.amplitudeExpansion = glvWidgets.getAmplitudeExpansion();
  }

  void setLLPositions() {
    Vec3f ll1Position = llMotion.getPosition(getTime());
    Pose newGoal;
    newGoal.pos(ll1Position);
    newGoal.faceToward(Vec3d(0, 0, 0));
    ll1.p = ll1.p.lerp(newGoal, 0.01);

    newGoal.pos(-ll1Position);
    newGoal.faceToward(Vec3d(0, 0, 0));
    ll2.p = ll2.p.lerp(newGoal, 0.01);

    if (turning) { nav().turnU(turnSpeed); }
  }

  float getTime() {
    return playbackPlayer.pos() / SAMPLE_RATE;
  }

  void sendDataToCuttlebone() {
    // transfer the poses to the cuttlebone-friendly data structure of each leaf looper
    ll1.llData.p = ll1.p;
    ll2.llData.p = ll2.p;
    ll1.llData.maxTrailLength = ll1.maxTrailLength;
    ll2.llData.maxTrailLength = ll2.maxTrailLength;
    ll1.llData.trailAlphaDecayFactor = ll1.trailAlphaDecayFactor;
    ll2.llData.trailAlphaDecayFactor = ll2.trailAlphaDecayFactor;
    ll1.llData.doTrail = ll1.doTrail;
    ll2.llData.doTrail = ll2.doTrail;

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
    ll1.pose(ll1.pose().lerp(ll1.p, 0.01));
    ll2.pose(ll2.pose().lerp(ll2.p, 0.01));
    float mul1 = 1; //pow((nav().pos() - ll1.p.pos()).mag(), -2);
    float mul2 = 1; //pow((nav().pos() - ll2.p.pos()).mag(), -2);
    // cout << mul1 << ", " << mul2 << endl;
    while (io()) {
      for(int chan = 0; chan < 2; ++chan) {
        float sampForAnalysis = anaylsisPlayer.read(chan);
        float sampForPlayback = playbackPlayer.read(chan);
        if(chan == 0) { ll1(sampForAnalysis); }
        if(chan == 1) { ll2(sampForAnalysis); }

        if(chan == 0) { ll1.writeSample(sampForPlayback * mul1); }
        if(chan == 1) { ll2.writeSample(sampForPlayback * mul2); }
        // io.out(chan) = sampForPlayback;
      }
      anaylsisPlayer.advance();
      playbackPlayer.advance();
    }
    listener()->pose(nav());
    scene()->render(io);
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
        nav().pos(0, -3.0, 0);
        nav().faceToward(Vec3d(0, -3.0, -1), Vec3d(0, 1, 0));
        break;
      case '0':
        ll1.doTrail = !ll1.doTrail;
        ll2.doTrail = !ll2.doTrail;
        break;
      case 't':
        turning = !turning;
        break;
      case 'p':
        // print all the glv settings to keep track of
        Color c = HSV(glvWidgets.bgColorPicker.getValue().components);
        cout << "BGColor: Color(" << c.r << ", " << c.g << ", " << c.b << ")" << endl;
        c = HSV(glvWidgets.ll1ColorPicker.getValue().components);
        cout << "LL1Color: Color(" << c.r << ", " << c.g << ", " << c.b << ")" << endl;
        c = HSV(glvWidgets.ll2ColorPicker.getValue().components);
        cout << "LL2Color: Color(" << c.r << ", " << c.g << ", " << c.b << ")" << endl;
        cout << "Looper Radii: " << glvWidgets.getLooperRadii() << endl;
        cout << "LL1 Leaf Type: " << glvWidgets.ll1LeafType.getValue() << endl;
        cout << "LL2 Leaf Type: " << glvWidgets.ll2LeafType.getValue() << endl;
        cout << "Amplitude Expansion: " << glvWidgets.getAmplitudeExpansion() << endl;
        cout << "Position: " << nav().pos() << endl;
        break;

    }
  }
};

int main() {
  LeafLoops app;
  app.AlloSphereAudioSpatializer::audioIO().start();
  app.InterfaceServerClient::connect();
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
