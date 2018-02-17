
#include <cassert>
#include <iostream>
#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;

class MyApp : public App {
public:

  Texture texture;

  MyApp() {
    texture.allocate(image.array());
  }

  void onDraw(Graphics& g) {

    g.pushMatrix();

      // Push the texture/quad back 5 units (away from the camera)
      //
      g.translate(0, 0, -5);

      // See void Texture::quad(...) in the Doxygen
      //
      texture.quad(g);

    g.popMatrix();
  }
};

int main() {
  MyApp app;
  app.initWindow(Window::Dim(600, 400), "Illuminated Averages");
  app.start();
}
