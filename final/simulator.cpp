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
#include <algorithm>
#include "Gamma/SamplePlayer.h"
#include "Gamma/DFT.h"
#include "common.hpp"
#include "utilityFunctions.hpp"
#include "leafOscillators.hpp"
#include "leafLooper.hpp"
#include "meterMaid.hpp"
#include "alloutil/al_AlloSphereAudioSpatializer.hpp"
#include "alloutil/al_Simulator.hpp"
using namespace al;
using namespace std;
using namespace gam;

#include "alloGLV/al_ControlGLV.hpp"
#include "GLV/glv.h"

// CONTROLS: Press 1 to pause playback, 2 to skip backwards 10 seconds in the soundfile, 3 to skip forwards 10 seconds, 4 to step forward one frame when paused


// Notes to self:

// TODO: Clean up the trashfire that is setLLPositions
// TODO: Controls over color, size, leafShape, min distance, background color in the simulator


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



struct LeafLoops : public App, AlloSphereAudioSpatializer, InterfaceServerClient {

  State state;
  cuttlebone::Maker<State> maker;

  SamplePlayer<> anaylsisPlayer, playbackPlayer;
  bool paused = false, doOneFrame = false;
  LeafLooper ll1, ll2;

  MeterMaid beatCycleLookup;

  bool firstDrawDone = false;

  // Motion
  float ll1HorizontalMotionPhase = 0, ll1VerticalMotionPhase = 0;
  float ll1HorizontalMotionFreq = 0.08, ll1VerticalMotionFreq = 0.05;
  float ll1HMotionRadiusMul = 8.0, ll1VMotionRadiusMul = 7.0;

  float ll2HorizontalMotionPhase = 0, ll2VerticalMotionPhase = 0;
  float ll2HorizontalMotionFreq = 0.08, ll2VerticalMotionFreq = 0.05;
  float ll2HMotionRadiusMul = 8.0, ll2VMotionRadiusMul = 7.0;

  LeafLoops() 
    : maker(Simulator::defaultBroadcastIP()),
      InterfaceServerClient(Simulator::defaultInterfaceServerIP()),
      ll1(ll1ComboOscillator, Color(0.8, 0.8, 0.0)), 
      ll2(ll2ComboOscillator, Color(0.0, 0.3, 0.8)),
      beatCycleLookup("LeafLoopsDownbeats.txt") {
    anaylsisPlayer.load(fullPathOrDie(ANALYSIS_SOUND_FILE_NAME).c_str());
    anaylsisPlayer.pos(FFT_SIZE); // give the analysisPlayer a headstart of FFT_SIZE, to compensate for the lag in analysis
    playbackPlayer.load(fullPathOrDie(PLAYBACK_SOUND_FILE_NAME).c_str());
    initWindow(Window::Dim(900, 600), "Leaf Loops");
    nav().pos(0, 20, 0);
    nav().faceToward(Vec3d(0, 0, 0), Vec3d(0, 0, -1));
    ll1.p.pos(0, 0, -3);
    ll1.p.faceToward(Vec3d(0, 0, 0), Vec3d(0, 1, 0));
    ll2.p.pos(0, 0, 3);
    ll2.p.faceToward(Vec3d(0, 0, 0), Vec3d(0, 1, 0));

    initializeMotionVariables();

    paused = false;

    // initAudio(SAMPLE_RATE);
    // audio
    AlloSphereAudioSpatializer::initAudio(SAMPLE_RATE);
    AlloSphereAudioSpatializer::initSpatialization();
    // if gamma
    gam::Sync::master().spu(AlloSphereAudioSpatializer::audioIO().fps());
    scene()->addSource(ll1);
    ll1.dopplerType(DOPPLER_NONE);
    scene()->addSource(ll2);
    ll2.dopplerType(DOPPLER_NONE);
    scene()->usePerSampleProcessing(true);
    // scene()->usePerSampleProcessing(false);
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
    while (InterfaceServerClient::oscRecv().recv()) {}

    if (!paused || doOneFrame) {
      doOneFrame = false;
      float measurePhase = beatCycleLookup.getPhasePosition(getTime());

      ll1.pushNewStrip(measurePhase, getTime()*4);
      ll2.pushNewStrip(measurePhase, getTime()*4);

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
    ll1.pose(ll1.p);
    ll2.pose(ll2.p);
    float mul1 = 1; //pow((nav().pos() - ll1.p.pos()).mag(), -2);
    float mul2 = 1; //pow((nav().pos() - ll2.p.pos()).mag(), -2);
    // cout << mul1 << ", " << mul2 << endl;
    while (io()) {
      for(int chan = 0; chan < 2; ++chan) {
        float sampForAnalysis = anaylsisPlayer.read(chan);
        float sampForPlayback = playbackPlayer.read(chan);
        if(chan == 0) { ll1(sampForAnalysis); }
        if(chan == 1) { ll2(sampForAnalysis); }

        // if(chan == 0) { ll1.writeSample(sampForPlayback * mul1); }
        // if(chan == 1) { ll2.writeSample(sampForPlayback * mul2); }
        io.out(chan) = sampForPlayback;
      }
      anaylsisPlayer.advance();
      playbackPlayer.advance();
    }
    listener()->pose(nav());
    // scene()->render(io);
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
        ll1.doTrail = !ll1.doTrail;
        ll2.doTrail = !ll2.doTrail;
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
