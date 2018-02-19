#define SOUND_FILE_NAME ("Rite4.wav")
#define WIN_SIZE (8192)
#define NUM_CHANNELS (2)
#define SAMPLE_RATE (44100)
#define OUTPUT_FILE ("OutRite4.wav")

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

template <size_t FFT_SIZE>
struct SoundStretcherTimeNeutral : STFT {
  /*
  bins with magnitude at or above expansionThreshold proportion of the max bin magnitude get raised to power compressionAmount
  (This leads to amplification and compression of important peaks in any given time frame, 
  and allows quite parts of the sound file to contribute more )
  bins with magnitude below compranderThreshold proportion of the max bin magnitude get raised to a power between compression 
  amount and max expansion amount, depending on how far below the threshold they are)
  (So this means background noise and other insignificant spectral material gets pushed down)
  */
  float compranderThreshold = 0.3;
  float compressionAmount = 0.4;   
  float maxExpansionAmount = 3.0;
  float gain = 0.4;

  float magnitudeAverages[FFT_SIZE/2+1];
  unsigned numFramesAveraged = 0;
  SoundStretcherTimeNeutral() :
  STFT::STFT(
    FFT_SIZE, FFT_SIZE/4,  // Window size, hop size
    0, HANN, COMPLEX 
  )
  {
    for(unsigned k=0; k<numBins(); ++k) { magnitudeAverages[k] = 0; }
  }
  float operator()(float s) {
    if(STFT::operator()(s)) {
      // Loop through all the bins
      float maxMag = 0;
      for(unsigned k=0; k<numBins(); ++k){
        if (bin(k).mag() > maxMag) { maxMag = bin(k).mag(); }
      }
      for(unsigned k=0; k<numBins(); ++k){
        float relativeMag = bin(k).mag() / maxMag;
        float compranderPower = relativeMag < compranderThreshold ? 
          compressionAmount + (compranderThreshold - relativeMag) / compranderThreshold * (maxExpansionAmount - compressionAmount): 
          compressionAmount;
        float comprandedMag = pow(bin(k).mag(), compranderPower) * gain;
        // incorporate into the average such that all frames have equal weighting
        magnitudeAverages[k] = (numFramesAveraged * magnitudeAverages[k] + comprandedMag) / (numFramesAveraged + 1);
        // set (outgoing) bin magnitude to the leaky integrated magnitude
        bin(k).norm(magnitudeAverages[k]);
        // randomize the bin phase
        bin(k).arg(rnd::uniform() * 2 * M_PI);
      }
      numFramesAveraged++;
    }
    // return a resynthesized sample
    return STFT::operator()();
  }
};

class IlluminatedAverages : public App {
public:

  Texture texture;
  SoundStretcherTimeNeutral<WIN_SIZE> stretchers[NUM_CHANNELS];
  SamplePlayer<> player;
  std::vector<float> recording;
  bool isRecording = false;

  // DEFINITELY TRY SEVERAL values of leaky simultaneously!!!!!

  IlluminatedAverages()
  {
    // texture.allocate(image.array());
    player.load(fullPathOrDie(SOUND_FILE_NAME, "..").c_str());
    initWindow(Window::Dim(600, 400), "Illuminated Averages");
    initAudio(SAMPLE_RATE);
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
      for(int chan = 0; chan < NUM_CHANNELS; ++chan) {
        float s = stretchers[chan](player.read(chan));
        io.out(chan) = s;
        if(isRecording) { recording.push_back(s); }
        if(NUM_CHANNELS == 1) {io.out(1) = s;}
      }
      player.advance();
    }
  }

  void onKeyDown(const ViewpointWindow&, const Keyboard& k) override {
    switch (k.key()) {
      default:
        break;
      case 'r':
        if (isRecording) {
          SoundFile sf(OUTPUT_FILE);
          // save data as sound file
          sf.format(SoundFile::WAV);
          sf.encoding(SoundFile::PCM_16);
          sf.channels(NUM_CHANNELS);
          sf.frameRate(SAMPLE_RATE);
          float numFrames = recording.size() / NUM_CHANNELS;
          
          cout << "Opening new file for writing...";
          if(sf.openWrite()){ 
            cout << "OK" << endl; 
            cout << "Writing data... ";
            if(sf.write(recording.data(), numFrames) == numFrames){  cout << "OK" << endl; }
            else{ cout << "fail" << endl; }
            sf.close();
          }
          else{ cout << "fail" << endl; }
          isRecording = false;
        } else {
          cout << "Starting recording..." << endl;
          isRecording = true;
        }
        break;
    }
  }
};

int main() {
  IlluminatedAverages illuminatedAverages;
  illuminatedAverages.start();
}
