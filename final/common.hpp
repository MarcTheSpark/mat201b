/*
  Marc Evans (2018/3/8)
  Final Project Common state
*/

#ifndef __COMMON__
#define __COMMON__
#define FFT_SIZE (1024)
#define NUM_LEAF_LOOPERS (2)
#define REDUNDANCY (5)
#define NUM_TRAIL_POINTS_PER_FRAME (5)

#include <cmath>
#include <iostream>
#include <vector>
#include <deque>

#include "Cuttlebone/Cuttlebone.hpp"
#include "allocore/al_Allocore.hpp"
using namespace al;

// Common definition of application state
//
template <size_t MESH_SIZE>
struct PseudoMesh
{
	Vec3f vertices[MESH_SIZE];
 	Color colors[MESH_SIZE];
};

struct LeafLooperData
{
	PseudoMesh<FFT_SIZE> latestStrips[REDUNDANCY];
	PseudoMesh<NUM_TRAIL_POINTS_PER_FRAME> latestTrailPoints[REDUNDANCY];

	Pose p;
	int maxTrailLength;
	float trailAlphaDecayFactor;
	bool doTrail;

	void shiftStrips() {
		// not efficient, but simple
		for(int i = 0; i < REDUNDANCY - 1; ++i) {
			latestStrips[i] = latestStrips[i+1];
		}
	}

	void shiftTrail() {
		// not efficient, but simple
		for(int i = 0; i < REDUNDANCY - 1; ++i) {
			latestTrailPoints[i] = latestTrailPoints[i+1];
		}
	}
};

struct State {
  unsigned framenum = 0;
  Pose navPose;
  Color bgColor;
  LeafLooperData llDatas[NUM_LEAF_LOOPERS];

  State() : bgColor(0, 0, 0) {}
};

#endif