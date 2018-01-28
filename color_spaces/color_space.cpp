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

// QUESTIONS:
// -- can you have a reference publically declared in the class?
// e.g. Buffer<Vec3f>& lastVertices
// -- Memory leak issues?

#include <cassert>
#include <iostream>
#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;

class MyApp : public App {
public:

  // Image and Texture handle reading and displaying image files.
  //
  Image image;
  Texture texture;
  Mesh pointMesh;
  Buffer<Vec3f> flatVertices, rgbVertices, hsvVertices, magicVertices; 
  Buffer<Vec3f> lastVertices, targetVertices;
  float transitionProgress = -1;

  MyApp() { 

    // Load a .jpg file
    //
    const char *filename = "mat201b/color_spaces/octapus.jpg";

    // I'm using points with a stroke size of 2 to render. I think it looks good, though 
    // it does not obey perspective.
    pointMesh.primitive(Graphics::POINTS);
    pointMesh.stroke(2);

    if (image.load(filename)) {
      printf("Read image from %s\n", filename);
    } else {
      printf("Failed to read image from %s!  Quitting.\n", filename);
      exit(-1);
    }
    // Here we copy the pixels from the image to the texture
    texture.allocate(image.array());

    // Don't bother trying to print the image or the image's array directly
    // using C++ syntax.  This won't work:
    //
    //cout << "Image " << image << endl;
    //cout << "   Array: " << image.array() << endl;

    // Make a reference to our image's array so we can just say "array" instead
    // of "image.array()":
    //
    Array& array(image.array());

    // The "components" of the array are like "planes" in Jitter: the number of
    // data elements in each cell.  In our case three components would
    // represent R, G, B.
    //
    cout << "array has " << (int) array.components() << " components" << endl;

    // Each of these data elements is represented by the same numeric type:
    //
    cout << "Array's type (as enum) is " << array.type() << endl;

    // But that type is represented as an enum (see al_Array.h), so if you want
    // to read it use this function:
    //
    printf("Array's type (human readable) is %s\n", allo_type_name(array.type()));

    // The array itself also has a print method:
    //
    cout << "Array.print: "  << endl << "   ";
    array.print();    

    // Code below assumes this type is 8-bit unsigned integer, so this line
    // guarantees that's the case, or else crashes the program if not:
    //
    assert(array.type() == AlloUInt8Ty);

    // AlloCore's image class provides a type for an RGBA pixel, which is of
    // course templated on the numeric type used to represent each value in the
    // pixel.  Since templating happens at compile time we can't just ask the
    // array at runtime what type to put in here (hence the "assert" above):
    //
    Image::RGBAPix<uint8_t> pixel;

    // Loop through all the pixels.  Note that the columns go from left to
    // right and the rows go from bottom to top.  (So the "row" and "column"
    // are like X and Y coordinates on the Cartesian plane, with the entire
    // image living in the quadrant with positive X and positive Y --- in other
    // words the origin is in the lower left of the image.)
    //
    cout << "Display ALL the pixels !!! " << endl;


    for (size_t row = 0; row < array.height(); ++row) {
      for (size_t col = 0; col < array.width(); ++col) {

        array.read(&pixel, col, row);
        float r = float(pixel.r)/255, g = float(pixel.g)/255, b = float(pixel.b)/255;

        Color c = Color(r, g, b);
        pointMesh.color(c);
        // note that I'm dividing in each case by the height so as to maintain the 
        // aspect ratio of the image. Thus 1 = image height
        float x = float(col)/array.height(), y = float(row)/array.height();
        pointMesh.vertex(x, y);
        HSV hsv = HSV(Color(r, g, b));
        float& h = hsv.h, s = hsv.s, v = hsv.v;

        rgbVertices.append(Vec3f(r, g, b));

        hsvVertices.append(Vec3f(
          s * cos(h * 2 * M_PI)/2, s * sin(h * 2 * M_PI)/2, v
        ));

        // take the picture and stretch it in a doughnut around the y axis using 
        // the hue value this way the form of the picture is still there, since 
        // we use the x and y values of the pixels
        
        magicVertices.append(Vec3f(
          (x + 1) * cos(h * 2 * M_PI) / 2, y, (x + 1) * sin(h * 2 * M_PI) / 2
        ));

      }
    }
    flatVertices = pointMesh.vertices();
  }

  void onAnimate(double dt) {
    if (transitionProgress >= 0) {
      // a negative value for transitionProgress indicates no transition is taking place
      // so only do this if it is positive
      Buffer<Vec3f>& vertices = pointMesh.vertices();
      if (transitionProgress < 1) {
        for(int i = 0; i < vertices.size(); i++) {
          vertices[i] = lerp(lastVertices[i], targetVertices[i], transitionProgress);
        }
        transitionProgress += dt;
      } else { 
        // if we've passed 1, then we are done transitioning; 
        // set vertices to the final value and transitionProgress to -1
        vertices = targetVertices;
        transitionProgress = -1;
      }
    }
  }

  void onDraw(Graphics& g) {

    g.pushMatrix();

    g.translate(0, 0, -5);
    g.draw(pointMesh);

    g.popMatrix();
  }

  void onKeyDown (const Keyboard &k) {
    switch(k.key()) {
      case '1':
        lastVertices = pointMesh.vertices();
        targetVertices = flatVertices;
        transitionProgress = 0;
        break;
      case '2':
        lastVertices = pointMesh.vertices();
        targetVertices = rgbVertices;
        transitionProgress = 0;
        break;
      case '3':
        lastVertices = pointMesh.vertices();
        targetVertices = hsvVertices;
        transitionProgress = 0;
        break;
      case '4':
        lastVertices = pointMesh.vertices();
        targetVertices = magicVertices;
        transitionProgress = 0;
        break;
    }
  }
};

int main() {
  MyApp app;
  app.initWindow(Window::Dim(600, 400), "imageTexture");
  app.start();
}
