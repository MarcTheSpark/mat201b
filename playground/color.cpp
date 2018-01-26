#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;

struct MyApp : App {
	Color bg;
	float t = 0;

	MyApp() {
		initWindow();
		bg = HSV(0, 1, 1);
	}

	void onAnimate(double dt) {
		t += dt / 10;
		if (t >= 1) { t -= 1; }
		bg = HSV(t, 1, 1);
		background(bg);
		cout << t << endl;
	}

	void onDraw(Graphics& g,  const Viewpoint& vp) {

	}
};

int main() {
	MyApp().start();
}