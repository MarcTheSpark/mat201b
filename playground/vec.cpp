#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;

int main() {
  Vec3f a(0, 0, 1), b(0, 1, 0);
  cout << a.dot(b) << endl;
  cout << a.cross(b) << endl;
  Vec3f c(1, 1, 1);
  cout << c.dot(a) << ", " << c.cross(a) << endl;
}
