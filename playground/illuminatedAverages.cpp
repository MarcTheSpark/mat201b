#define WIN_SIZE (2048)

#include <cassert>
#include <iostream>
#include "Gamma/DFT.h"
#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;
using namespace gam;

class IlluminatedAverages : public App {
public:

  Texture texture;
  STFT stft;
  float magnitudeAverages[WIN_SIZE/2+1];
  float leaky = 0.9999;

  IlluminatedAverages() 
  : stft(
    WIN_SIZE,   // Window size
    WIN_SIZE/4,   // Hop size; number of samples between transforms
    0,      // Pad size; number of zero-valued samples appended to window
    HANN,   // Window type: BARTLETT, BLACKMAN, BLACKMAN_HARRIS,
          //    HAMMING, HANN, WELCH, NYQUIST, or RECTANGLE
    COMPLEX   // Format of frequency samples:
  )
  {
    // texture.allocate(image.array());
    for(unsigned k=0; k<stft.numBins(); ++k) { magnitudeAverages[k] = 0; }
    initWindow(Window::Dim(600, 400), "Illuminated Averages");
    initAudio();
  }

  void onDraw(Graphics& g) override {

    g.pushMatrix();

      // Push the texture/quad back 5 units (away from the camera)
      //
      g.translate(0, 0, -5);

      // See void Texture::quad(...) in the Doxygen
      //
      // texture.quad(g);

    g.popMatrix();
  }

  void onSound(AudioIOData& io) override {
    gam::Sync::master().spu(audioIO().fps());
    while (io()) {
      float s = io.in(0);
      // Input next sample for analysis
      // When this returns true, then we have a new spectral frame
      if(stft(s)){
      
        // Loop through all the bins
        for(unsigned k=0; k<stft.numBins(); ++k){
          // Here we simply scale the complex sample
          magnitudeAverages[k] = leaky * magnitudeAverages[k] + (1 - leaky) * stft.bin(k).mag();
          stft.bin(k).norm(magnitudeAverages[k]);
          stft.bin(k).arg(rnd::uniform() * 2 * M_PI);

          // cout << stft.bin(k).mag() << ", " << stft.bin(k).phase() << endl;
          // stft.bin(k).arg(-1);
          // cout << stft.bin(k).mag() << ", " << stft.bin(k).phase() << endl;
        }
        // cout << "DONE" << endl;
        // cout << stft.numBins() << endl;
      }

      // Get next resynthesized sample
      s = stft();
      io.out(0) = s;
      io.out(1) = s;
    }
  }
};

int main() {
  IlluminatedAverages illuminatedAverages;
  illuminatedAverages.start();
}
