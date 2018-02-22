/*
  This displays the pixels of an image in four different ways, accessed by pressing keys 1 - 4:
  1) (default) pixels are arranged with z=0 in the positive x and y axes, 
  reproducing the original image.
  2) Pixels are arranged in a unit RGB cube according to their color.
  3) Pixels are arranged in an hsv cylinder whose base is on the x/y plane and has diameter 1
  4) The image is stretched in a doughnut around the y axis; the the original x-value of the 
  pixel + 1 becomes the radius, the hue becomes the angle of rotation about the y axis, and 
  the y value remains unchanged. This way, one can see the form of the original imaged stretched
  out according to hue.

  Marc Evans (2018/1/28)
*/


#include <cassert>
#include <iostream>
#include <fstream>
#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;


int fftLength = 1024;
int sampleRate  = 44100;

struct LeafLooper {
  Buffer<Buffer<Vec3f>> radialStrips;
  float centerFrequency = 4000;
  float centerRadius = 1;
  float phase = 0;
  vector<float> normalRadii;
  LeafLooper() {
    for(int i = 0; i < fftLength / 2; i++) {
        float freq = float(i) / fftLength * sampleRate;
        normalRadii.push_back((log(freq) - log(20)) / (log(centerFrequency) - log(20)) * centerRadius);  
    }
  }
  void pushNewStrip(float phase, vector<float> fftMagnitudes) {

  }
};

class LeafLoops : public App {
public:

  Mesh leaf;
  
  Buffer<Vec3f> leafOutline;
  Vec3f leafCenter;

  LeafLoops(){ 
    leaf.primitive(Graphics::LINE_LOOP);
    leaf.stroke(2);
    ifstream input( "mat201b/LeafLoops/LeafPoints.txt" );
    for( string line; getline( input, line ); )
    {
        int commaPos = line.find(", ");
        Vec3f thisPoint(Vec3f(stof(line.substr(0, commaPos)), stof(line.substr(commaPos + 2)), 0));
        leafOutline.append(thisPoint);
        leafCenter += thisPoint;
    }
    leafCenter /= leafOutline.size();
    cout << leafCenter << endl;
    leaf.vertices() = leafOutline;
  }

  void onAnimate(double dt) {
      Buffer<float> fftMagnitudes;
      for(int i = 0; i < 1024; i++) {
        fftMagnitudes.append(sin(float(i)/1023 * M_PI));
      }
  }

  void onDraw(Graphics& g) {

    g.pushMatrix();

    g.translate(0, 0, -5);
    g.draw(leaf);

    g.popMatrix();
  }

  void onKeyDown (const Keyboard &k) {
    switch(k.key()) {
      default:
        break;
    }
  }
};

int main() {
  LeafLoops app;
  app.initWindow(Window::Dim(900, 600), "Leaf Loops");
  app.start();
}
