#define SOUND_FILE_NAME ("Beet7Mvt2KleiberMono.wav")
#define FFT_SIZE (33554432)

#include <cassert>
#include <iostream>
#include <iomanip>
#include "Gamma/DFT.h"
#include "allocore/io/al_App.hpp"
#include "Gamma/SamplePlayer.h"

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

class IlluminatedAverages : public App {
public:

  Texture texture;
  RFFT<float> fft;
  SamplePlayer<float, gam::ipl::Linear, phsInc::Loop> player;
  float * fftBuffer;
  int currentSample = 0;

  // DEFINITELY TRY SEVERAL values of leaky simultaneously!!!!!

  IlluminatedAverages() :
  fft(FFT_SIZE)
  {
    // texture.allocate(image.array());
    player.load(fullPathOrDie(SOUND_FILE_NAME, "..").c_str());
    fftBuffer = new float[FFT_SIZE];

    for (int i = 0; i < FFT_SIZE; ++i) {
      if (i < player.size()) {
        fftBuffer[i] = player[i];
      } else { fftBuffer[i] = 0; }
    }
    fft.forward(fftBuffer);
    unsigned numberOfBins = FFT_SIZE / 2 + 1;
    for (unsigned i = 0; i < numberOfBins; ++i) {
      gam::Complex<float> c;
      if (i == 0)
        // fftBuffer[0] is DC
        fftBuffer[0] = 0;
      else if (i == numberOfBins - 1)
        // fftBuffer[numberOfBins-1] (corresponding to i = numberOfBins/2) is Nyquist
        fftBuffer[FFT_SIZE -1] = 0;
      else
        // fftBuffer[i * 2 - 1] is the real part for a bin
        // fftBuffer[i * 2] is the imaginary part for a bin
        fftBuffer[i * 2] = rnd::uniformS() * M_PI;
        // c(fftBuffer[i * 2 - 1], fftBuffer[i * 2]);
    }
    fft.inverse(fftBuffer);

    initWindow(Window::Dim(600, 400), "Illuminated Averages");
    initAudio();
  }

  void onDraw(Graphics& g) override {

    g.pushMatrix();
      g.translate(0, 0, -5);
      // texture.quad(g);
    g.popMatrix();
  }

  void onSound(AudioIOData& io) override {
    gam::Sync::master().spu(audioIO().fps());
    while (io()) {
      float s = fftBuffer[currentSample];
      io.out(0) = s;
      io.out(1) = s;
      currentSample++;
    }
  }

  ~IlluminatedAverages() {
    App::~App();
    delete [] fftBuffer;
  }
};

int main() {
  IlluminatedAverages illuminatedAverages;
  illuminatedAverages.start();
}
