#ifndef __COMMON__
#define __COMMON__
#define MAX_PARTICLES (500)

#include "allocore/io/al_App.hpp"
using namespace al;


// Common definition of application state
//
struct State {
  unsigned numParticles = 0;
  double sphereRadius;
  bool drawDebugVectors = false;
  Vec3f particlePositions[MAX_PARTICLES];
  Color particleColors[MAX_PARTICLES];
  Vec3f particleDebugVectors[MAX_PARTICLES];
};

#endif
