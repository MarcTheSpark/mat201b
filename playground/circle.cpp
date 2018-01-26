#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;

struct Frame : Mesh {
	Frame() {
		primitive(Graphics::LINES);
		// x-axis
		vertex(0, 0, 0);
		color(1, 1, 1);
		vertex(1, 0, 0);
		color(1, 0, 0);
		// y-axis
		vertex(0, 0, 0);
		color(1, 1, 1);
		vertex(0, 1, 0);
		color(0, 1, 0);
		// z-axis
		vertex(0, 0, 0);
		color(1, 1, 1);
		vertex(0, 0, 1);
		color(0, 0, 1);
	}
};

struct MyApp : App {
	Color bg;
	float t = 0;
	Frame frame;
	Mesh c;

	MyApp() {
		initWindow();
		bg = HSV(0, 0, 0);
		c.primitive(Graphics::LINE_LOOP);
		for (int i = 0; i < 100; i++) {
			c.vertex(sin(M_PI * 2 * i / 100), cos(M_PI * 2 * i / 100));
			c.color(1, 1, 1);
		}
		c.vertices()[0].z = 5;
	}

	void onAnimate(double dt) {
		t += dt / 10;
		if (t >= 1) { t -= 1; }
	}

	void onDraw(Graphics& g,  const Viewpoint& vp) {
		g.draw(frame);
		g.draw(c);
	}
};

int main() {
	MyApp().start();
}