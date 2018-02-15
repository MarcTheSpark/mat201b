#ifndef __COMMON__
#define __COMMON__
#define MAX_BOIDS (500)

#include "allocore/io/al_App.hpp"
using namespace al;


// Common definition of application state
//
struct State {
  unsigned numBoids = 0;
  Pose boidPoses[MAX_BOIDS];
};

#endif
