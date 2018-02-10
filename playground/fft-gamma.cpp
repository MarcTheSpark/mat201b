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

template <size_t FFT_LENGTH>
struct FFTer {
  unsigned blockSize;
  unsigned sampleRate;
  float circularInputBuffer[FFT_LENGTH];
  unsigned circularInputBufferIndex = 0;
  bool completedOneFFTLength = false;
  float fftBuffer[FFT_LENGTH];
  float hanning[FFT_LENGTH];
  unsigned currentFFTSample = 0;
  RFFT<float> fft;
  unsigned numberOfBins = FFT_LENGTH / 2 + 1;
  gam::Complex<float> bins[FFT_LENGTH / 2 + 1];
  float binFreqs[FFT_LENGTH / 2 + 1];

  FFTer(unsigned blockSize = 128, unsigned sampleRate = 44100) {
    assert(FFT_LENGTH % blockSize == 0);
    this->blockSize = blockSize;
    this->sampleRate = sampleRate;
    for (unsigned i = 0; i < FFT_LENGTH; i++) {
      circularInputBuffer[i] = fftBuffer[i] = 0;
      hanning[i] = pow(sin(float(i)/FFT_LENGTH*M_PI), 2);
    }
    for (unsigned i = 0; i < numberOfBins; ++i) {
      binFreqs[i] = (float)i / numberOfBins * sampleRate / 2.0;
      bins[i] = gam::Complex(0, 0);
    }
  }
  void sendSample(float s) {
    circularInputBuffer[circularInputBufferIndex] = s;
    circularInputBufferIndex++;
    if(circularInputBufferIndex > FFT_LENGTH) {
      circularInputBufferIndex -= FFT_LENGTH;
      completedOneFFTLength = true;
    }
  }
  void operator()(float s) {
    sendSample(s);
  }
  void calc() {
    if (completedOneFFTLength) {
      for(unsigned i = 0; i < FFT_LENGTH; ++i) {
        // set the fft buffer based on the last FFT_LENGTH contributions 
        // to the circular buffer times the Hanning window
        fftBuffer[i] = hanning[i] * circularInputBuffer[(circularInputBufferIndex + i) % FFT_LENGTH];
      }
      fft.forward(fftBuffer);

      for (unsigned i = 0; i < FFT_SIZE / 2 + 1; ++i) {
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
    return bins[i].mag();
  }
  float freq(unsigned binNum) {
    return binFreqs[i];
  }
};

struct AlloApp : App {
  Mesh m;
  SamplePlayer<float, gam::ipl::Linear, phsInc::Loop> player;
  RFFT<float> fft;
  float buffer[FFT_SIZE];
  float hanning[FFT_SIZE];
  unsigned currentFFTSample = 0;

  AlloApp() : fft(FFT_SIZE) {
    assert(FFT_SIZE % BLOCK_SIZE == 0);
    m.primitive(Graphics::LINE_STRIP);
    player.load(fullPathOrDie(SOUND_FILE_NAME, "..").c_str());
    for (unsigned i = 0; i < FFT_SIZE; i++) {
      buffer[i] = 0;
      hanning[i] = pow(sin(float(i)/FFT_SIZE*M_PI), 2);
    }
    initWindow();
    initAudio(SAMPLE_RATE, BLOCK_SIZE);
    nav().pos(0, 0, 10);
  }

  virtual void onAnimate(double dt) {}
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
      if (currentFFTSample < FFT_SIZE) {
        buffer[currentFFTSample] = s * hanning[currentFFTSample];
        currentFFTSample++;
      }
    }

    if(currentFFTSample >= FFT_SIZE) {
      // execute the FFT on the buffer
      fft.forward(buffer);
      // calculate how many bins to expect
      m.vertices().reset();
      unsigned numberOfBins = FFT_SIZE / 2 + 1;

      for (int i = 0; i < numberOfBins; i++) {
        // this is how this FFT implementation lays out the data. i won't explain
        // this here and now. ask me about it sometime.
        //
        gam::Complex<float> c;
        if (i == 0)
          c(buffer[0], 0);
        else if (i == numberOfBins / 2)
          c(buffer[numberOfBins - 1], 0);
        else
          c(buffer[i * 2 - 1], buffer[i * 2]);

        // calculate the center frequency of the current bin
        //
        float frequencyOfBin = (float)i / numberOfBins * 44100.0 / 2.0;

        m.vertex(frequencyOfBin/10000, c.mag()*100);
        // cout << c.mag() << endl;

        // print stuff to the terminal
        //
        // cout << scientific << frequencyOfBin << "Hz: " << scientific << c.mag()
        //     << endl;
      }
          
      //re-zero the buffer
      for (unsigned i = 0; i < FFT_SIZE; i++) buffer[i] = 0;
      currentFFTSample = 0;
    }    
  }
};

int main() { AlloApp().start(); }
