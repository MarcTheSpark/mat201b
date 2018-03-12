/*
  Marc Evans (2018/3/8)
  Final Project Common state
*/

#ifndef __COMMON__
#define __COMMON__
#define FFT_SIZE (1024)
#define NUM_LEAF_LOOPERS (2)
#define REDUNDANCY (5)

#include "allocore/io/al_App.hpp"
using namespace al;

// Common definition of application state
//
struct StripPseudoMesh
{
	Vec3f vertices[FFT_SIZE];
 	Color colors[FFT_SIZE];
};

struct LeafLooperData
{
	StripPseudoMesh latestStrips[REDUNDANCY];
	Pose p;

	void shiftStrips() {
		// not efficient, but simple
		for(int i = 0; i < REDUNDANCY - 1; ++i) {
			latestStrips[i] = latestStrips[i+1];
		}
	}
};

struct State {
  unsigned framenum = 0;
  Pose navPose;
  LeafLooperData llDatas[NUM_LEAF_LOOPERS];
};

#endif