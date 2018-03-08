#define SOUND_FILE_NAME ("Smashmouth.wav")
#define FFT_SIZE (2048)
#define NUM_CHANNELS (2)
#define SAMPLE_RATE (44100)

#include <cassert>
#include <iostream>
#include <iomanip>
#include "Gamma/DFT.h"
#include "allocore/io/al_App.hpp"
#include "Gamma/SamplePlayer.h"

using namespace al;
using namespace std;
using namespace gam;

// TODO: Make it take in a stereo sound file, output a stereo average!

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

struct Rings : Mesh {
  Rings() {
    addAnnulus((*this), 1, 1.2, 8);
    addAnnulus((*this), 1.4, 1.6, 8);

    // this->translate(Vec3f(4, 0, 0));
    // addSphere((*this), 2);
    this->generateNormals();
    // this->primitive(Graphics::TRIANGLE_FAN);
    for(auto& vertex : this->vertices()) {
      cout << vertex << ", ";
    }
    cout << endl;
  }
  void setRingColor(int ringIndex) {}
};

class IlluminatedAverages : public App {
public:

  Texture texture;
  SamplePlayer<> player;
  STFT stft;
  Rings r;
  Material material;
  Light light;

  // DEFINITELY TRY SEVERAL values of leaky simultaneously!!!!!

  IlluminatedAverages() : stft(FFT_SIZE, FFT_SIZE/4, 0, HANN)
  {
    light.pos(0, 0, 0);              // place the light
    // texture.allocate(image.array());
    player.load(fullPathOrDie(SOUND_FILE_NAME, "..").c_str());
    initWindow(Window::Dim(600, 400), "Illuminated Averages");
    initAudio(SAMPLE_RATE);
  }

  void onDraw(Graphics& g) override {
    material();
    light();
    g.pushMatrix();
    g.translate(0, 0, -5);
    // texture.quad(g);
    g.draw(r);
    g.popMatrix();
  }

  void onSound(AudioIOData& io) override {
    gam::Sync::master().spu(audioIO().fps());
    while (io()) {
      if(stft(player())) {
        // cout << "did fft" << endl;
      }
    }
  }

  void onKeyDown(const ViewpointWindow&, const Keyboard& k) override {
    switch (k.key()) {
      default:
        break;
    }
  }
};

int main() {
  IlluminatedAverages illuminatedAverages;
  illuminatedAverages.start();
}
