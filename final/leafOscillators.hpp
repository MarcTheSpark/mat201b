/*
  Marc Evans (2018/3/8)
  Final Project Leaf Oscillators
*/

#ifndef __LEAF_OSCILLATORS__
#define __LEAF_OSCILLATORS__

#include "allocore/al_Allocore.hpp"

class LeafOscillator {
  // LeafOscillator abstract class
   public:
      // pure virtual function
      virtual float getAngle(float phase) = 0;
      virtual float getRadius(float phase) = 0;
};

struct SingleLeafOscillator : LeafOscillator {
  std::vector<float> angles;
  std::vector<float> radii;
  float phaseAdjustment;
  SingleLeafOscillator(string dataFileName) {
    std::vector<float> data;
    ifstream input(fullPathOrDie(dataFileName));
    for( string line; getline( input, line ); ) {
        data.push_back(stof(line));
    }
    for(int i = 0; i < data.size(); i += 2) {
      al::Vec2f displacement(data.at(i), data.at(i+1));
      angles.push_back(atan2(displacement.y, displacement.x));
      radii.push_back(displacement.mag());
    }
  }

  float getAngle(float phase) {
    return angles.at(int((phase - floor(phase)) * angles.size()));
  }

  float getRadius(float phase) {
    return radii.at(int((phase - floor(phase)) * radii.size()));
  }
};


struct CombinedLeafOscillator : LeafOscillator {
  LeafOscillator& lfo1;
  LeafOscillator& lfo2;
  float weighting = 0;  // weighting of 0 means all lfo1, of 1 means all lfo2
  CombinedLeafOscillator(LeafOscillator& _lfo1, LeafOscillator& _lfo2) :
    lfo1(_lfo1), lfo2(_lfo2) 
  {}  

  void setWeighting(float w) {
    assert(w >= 0 && w <= 1);
    weighting = w;
  }

  float getRadius(float phase) {
    return (1 - weighting) * lfo1.getRadius(phase) + weighting * lfo2.getRadius(phase);
  }

  float getAngle(float phase) {
    // averaging angles takes a little more care, due to the cyclical nature
    float angle1 = lfo1.getAngle(phase);
    float angle2 = lfo2.getAngle(phase);
    if (angle1 - angle2 > M_PI) angle2 += 2 * M_PI;
    else if (angle2 - angle1 > M_PI) angle1 += 2 * M_PI;
    float averageAngle = (1 - weighting) * angle1 + weighting * angle2;
    if (averageAngle > 2 * M_PI) return averageAngle - 2 * M_PI;
    else return averageAngle;
  }
};

SingleLeafOscillator ivyOscillator("LeafDisplacements1.txt");
SingleLeafOscillator birchOscillator("LeafDisplacements2.txt");

#endif