// MAT201B
// Fall 2015
// Author(s): Karl Yerkes
//
// Shows how to:
// - Use Gamma's RFFT<> class
// - Visualize the FFT with a Mesh
//
#define SOUND_FILE_NAME ("dolphin.wav")
#define BLOCK_SIZE (128)
#define FFT_SIZE (2048)
#define SAMPLE_RATE (44100)

#include <iomanip>
#include "Gamma/DFT.h"
#include "Gamma/SamplePlayer.h"
#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;
using namespace gam;


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

template <size_t fftSize>
struct FFTer {
  unsigned blockSize;
  unsigned sampleRate;
  float circularInputBuffer[fftSize];
  unsigned circularInputBufferIndex = 0;
  bool completedOneFFTLength = false;
  float fftBuffer[fftSize];
  float hanning[fftSize];
  unsigned currentFFTSample = 0;
  RFFT<float> fft;
  unsigned numberOfBins = fftSize / 2 + 1;
  gam::Complex<float> bins[fftSize / 2 + 1];
  float binFreqs[fftSize / 2 + 1];

  FFTer(unsigned sampleRate = 44100, unsigned blockSize = 128) : fft(fftSize) {
    assert(fftSize % blockSize == 0);
    this->blockSize = blockSize;
    this->sampleRate = sampleRate;
    for (unsigned i = 0; i < fftSize; i++) {
      circularInputBuffer[i] = fftBuffer[i] = 0;
      hanning[i] = pow(sin(float(i)/fftSize*M_PI), 2);
    }
    for (unsigned i = 0; i < numberOfBins; ++i) {
      binFreqs[i] = (float)i / numberOfBins * sampleRate / 2.0;
      bins[i] = gam::Complex<float>(0, 0);
    }
  }
  void sendSample(float s) {
    circularInputBuffer[circularInputBufferIndex] = s;
    circularInputBufferIndex++;
    if(circularInputBufferIndex >= fftSize) {
      circularInputBufferIndex -= fftSize;
      completedOneFFTLength = true;
    }
  }
  void operator()(float s) {
    sendSample(s);
  }
  void calc() {
    if (completedOneFFTLength) {
      for(unsigned i = 0; i < fftSize; ++i) {
        // set the fft buffer based on the last fftSize contributions 
        // to the circular buffer times the Hanning window
        fftBuffer[i] = hanning[i] * circularInputBuffer[(circularInputBufferIndex + i) % fftSize];
      }

      fft.forward(fftBuffer);

      for (unsigned i = 0; i < fftSize / 2 + 1; ++i) {
        gam::Complex<float> c;
        if (i == 0)
          c(fftBuffer[0], 0);
        else if (i == numberOfBins / 2)
          c(fftBuffer[numberOfBins - 1], 0);
        else
          c(fftBuffer[i * 2 - 1], fftBuffer[i * 2]);
        bins[i] = c;
      }
    }
  }
  float mag(unsigned binNum) {
    return bins[binNum].mag();
  }
  float freq(unsigned binNum) {
    return binFreqs[binNum];
  }
  unsigned numBins() {
    return numberOfBins;
  }
};

struct AlloApp : App {
  Mesh m;
  SamplePlayer<float, gam::ipl::Linear, phsInc::Loop> player;

  FFTer<FFT_SIZE> marcFFT;

  AlloApp() : marcFFT(SAMPLE_RATE, BLOCK_SIZE) {
    assert(FFT_SIZE % BLOCK_SIZE == 0);
    m.primitive(Graphics::LINE_STRIP);
    player.load(fullPathOrDie(SOUND_FILE_NAME, "..").c_str());
    initWindow();
    initAudio(SAMPLE_RATE, BLOCK_SIZE);
    nav().pos(0, 0, 10);
  }

  virtual void onAnimate(double dt) {
    m.vertices().reset();
    for(unsigned i = 0; i < marcFFT.numBins(); ++i) {
      m.vertex(marcFFT.freq(i) / 10000, marcFFT.mag(i)*100);
    }
  }

  virtual void onDraw(Graphics& g, const Viewpoint& v) {
    g.color(1, 1, 1, 1);
    g.draw(m);
  }
  virtual void onKeyDown(const ViewpointWindow&, const Keyboard& k) {}

  virtual void onSound(AudioIOData& io) {
    gam::Sync::master().spu(audioIO().fps());
    while (io()) {
      float s = player();
      io.out(0) = s;
      io.out(1) = s;

      marcFFT(s);
    }
    marcFFT.calc();
  }
};

int main() { AlloApp().start(); }
