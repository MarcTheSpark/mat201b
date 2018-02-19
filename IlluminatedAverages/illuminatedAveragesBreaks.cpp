/* this works, but I never delete the memory allocated with new. should I and when? */

#define WIN_SIZE (2048)

#include <cassert>
#include <iostream>
#include "Gamma/DFT.h"
#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;
using namespace gam;


template <size_t FFT_SIZE>
struct SoundStretcher : STFT {
  float magnitudeAverages[FFT_SIZE/2+1];
  float leaky;
  SoundStretcher(float leakCoefficient=0.99) 
  : leaky(leakCoefficient) {
    STFT::STFT(
      FFT_SIZE, FFT_SIZE/4,  // Window size, hop size
      0, HANN,COMPLEX 
    );
    for(unsigned k=0; k<numBins(); ++k) { magnitudeAverages[k] = 0; }
  }
  float operator()(float s) {
    if(STFT::operator()(s)) {
      // Loop through all the bins
      for(unsigned k=0; k<numBins(); ++k){
        // nudge the average in the direction of the current magnitude spectrum
        magnitudeAverages[k] = leaky * magnitudeAverages[k] + (1 - leaky) * bin(k).mag();
        // set (outgoing) bin magnitude to the leaky integrated magnitude
        bin(k).norm(magnitudeAverages[k]);
        // randomize the bin phase
        bin(k).arg(rnd::uniform() * 2 * M_PI);
      }
    }
    // return a resynthesized sample
    return STFT::operator()();
  }
};

class IlluminatedAverages : public App {
public:

  Texture texture;
  vector<SoundStretcher<WIN_SIZE>> stretchers;

  // DEFINITELY TRY SEVERAL values of leaky simultaneously!!!!!

  IlluminatedAverages() 
  {
    for(int i = 1; i < 6; ++i) {
      stretchers.push_back(SoundStretcher<WIN_SIZE>(1-pow(10,-i)));
    }
    // texture.allocate(image.array());
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
      float s = 0;
      for (auto& stretcher : stretchers) {
        s += stretcher(io.in(0));
      }
      s /= pow(stretchers.size(), 0.5);
      io.out(0) = s;
      io.out(1) = s;
    }
  }
};

int main() {
  IlluminatedAverages illuminatedAverages;
  illuminatedAverages.start();
}
