/*
  Marc Evans (2018/3/8)
  Final Project Score
  A class for keeping track of the position within a bar or higher lever metrical structure
*/

#ifndef __SCORE__
#define __SCORE__

#include <cassert>
#include "leafLooper.hpp"


class Score {
  
  public:
    Score(LeafLooper& _ll1, LeafLooper& _ll2, Nav& _nav, MeterMaid& _beatCycleLookup, MeterMaid& _hyperbeatCycleLookup, 
      MeterMaid& _smallSectionCycleLookup, MeterMaid& _bigSectionCycleLookup, float& _horizontalMotionRadius, float& _verticalMotionRadius, float& _turnSpeed) 
    : ll1(_ll1), ll2(_ll2), nav(_nav), 
      horizontalMotionRadius(_horizontalMotionRadius), verticalMotionRadius(_verticalMotionRadius),
      beatCycleLookup(_beatCycleLookup), hyperbeatCycleLookup(_hyperbeatCycleLookup), 
      smallSectionCycleLookup(_smallSectionCycleLookup), bigSectionCycleLookup(_bigSectionCycleLookup),
      turnSpeed(_turnSpeed)
    {}

    void setFromTime(float t) {
      setBeatsAndPhases(t);
      setColors(t);
      setTrailLength(t);
      setRadiusMul();
      setPosition(t);
    }

    void setColors(float t) {
      switch(bigSectionNum)
      {
        default:
          ll1.llColor = Color(0.9375, 0.9375, 0.3125);
          ll2.llColor = Color(0.39375, 0.875, 0.538125);
          break;
        case 2:
          ll1.llColor = Color(0.9375, 0.5, 0.3125);
          ll2.llColor = Color(0.8375, 0.7, 0.1125);
          break;
        case 3:
          ll1.llColor = Color(0, 0.979167, 0.391667);
          ll2.llColor = Color(0.9375, 0.907812, 0.640625);
          break;
        case 4:
          ll1.llColor = Color(0.9375, 0.9375, 0.3125);
          ll2.llColor = Color(0.39375, 0.875, 0.538125);
          break;
        case 5:
          ll1.llColor = Color(0.9375, 0.5, 0.3125);
          ll2.llColor = Color(0.8375, 0.7, 0.1125);
          break;
        case 6:
          ll1.llColor = Color(0.9375 * (1-bigSectionPhase), 0.5 * (1-bigSectionPhase) + 0.685416 * bigSectionPhase, 0.3125 * (1-bigSectionPhase) + 0.979167 * bigSectionPhase);
          ll2.llColor = Color(0.8375 * (1-bigSectionPhase) + 0.7 * bigSectionPhase, 0.7 * (1-bigSectionPhase), 0.1125 * (1-bigSectionPhase) + 1.0 * bigSectionPhase);
          break;
        case 7:
          ll1.llColor = Color(0.979167 * bigSectionPhase, 0.685416 * (1-bigSectionPhase) + 0.0979166 * bigSectionPhase, 0.979167 * (1-bigSectionPhase));
          ll2.llColor = Color(0.7 + 0.3 * bigSectionPhase, 0.47 * bigSectionPhase, 1.0 * (1-bigSectionPhase) + 0.116667 * bigSectionPhase);
          break;
        case 8:
          ll1.llColor = Color(0.979167, 0.0979166, 0);
          ll2.llColor = Color(1, 0.47, 0.116667);
          break;
        case 9:
          ll1.llColor = Color(0.9375, 0.5, 0.3125);
          ll2.llColor = Color(0.9375, 0.5, 0.3125);
          break;
      }
    }

    void setPosition(float t) {
      switch(bigSectionNum)
      {
        case 6:
          nav.pos(nav.x(), -3.0 - 7 * bigSectionPhase, nav.z());
          break;
        case 7:
          nav.pos(nav.x(), -10.0 + 7 * bigSectionPhase, nav.z());
          break;
      }
    }

    void setRadiusMul() {
      float smallDownbeatiness = pow(sin(smallSectionPhase*M_PI), 0.5);
      float bigDownbeatiness = pow(sin(bigSectionPhase), 0.5);
      horizontalMotionRadius = 6.5 * (1.1 + bigDownbeatiness/3);
      verticalMotionRadius = 8.0 * (1.1 + smallDownbeatiness/3);
      float amplitudeExpansion = 0.5;
      ll1.amplitudeExpansion = amplitudeExpansion;
      ll2.amplitudeExpansion = amplitudeExpansion;
      ll1.setBinRadii(1 + (1 - bigDownbeatiness)*2.5 + (1 - smallDownbeatiness));
      ll2.setBinRadii(1 + (1 - bigDownbeatiness)*2.5 + (1 - smallDownbeatiness));

    }

    void setTrailLength(float t) {
      if(framesSinceLastBigSection < 7) {
        if(bigSectionNum == 1 || bigSectionNum == 2 || bigSectionNum == 3 || bigSectionNum == 5 || bigSectionNum == 8 || bigSectionNum == 12) {
          // clearing after a big juncture
          ll1.maxTrailLength = 5;
          ll2.maxTrailLength = 5;
        }
      } else {
        switch(bigSectionNum)
        {
          default:
            ll1.setTrailLengthInSeconds(15);
            ll2.setTrailLengthInSeconds(15);
            break;
          case 8:
          case 9:
          case 10:
          case 11:
            ll1.setTrailLengthInSeconds(30);
            ll2.setTrailLengthInSeconds(30);
            break;
        }
      }
    }

    void setBeatsAndPhases(float t) {
      int oldBeatNum = beatNum, oldHyperbeatNum = hyperbeatNum, oldSmallSectionNum = smallSectionNum, oldBigSectionNum = bigSectionNum;
      beatPhase = beatCycleLookup.getPhasePosition(t, beatNum);
      hyperbeatPhase = hyperbeatCycleLookup.getPhasePosition(t, hyperbeatNum);
      smallSectionPhase = smallSectionCycleLookup.getPhasePosition(t, smallSectionNum);
      bigSectionPhase = bigSectionCycleLookup.getPhasePosition(t, bigSectionNum);
      if (oldBeatNum != beatNum) { framesSinceLastBeat = 0; } else { framesSinceLastBeat++; }
      if (oldHyperbeatNum != hyperbeatNum) { framesSinceLastHyperBeat = 0; } else { framesSinceLastHyperBeat++; }
      if (oldSmallSectionNum != smallSectionNum) { framesSinceLastSmallSection = 0; } else { framesSinceLastSmallSection++; }
      if (oldBigSectionNum != bigSectionNum) { framesSinceLastBigSection = 0; } else { framesSinceLastBigSection++; }
      // cout << beatNum << ", " << beatPhase << ", ";
      // cout << hyperbeatNum << ", " << hyperbeatPhase << ", ";
      // cout << smallSectionNum << ", " << smallSectionPhase << ", ";
      // cout << bigSectionNum << ", " << bigSectionPhase << endl;
      // cout << framesSinceLastBeat << ", " << framesSinceLastHyperBeat << ", " << framesSinceLastSmallSection << ", " << framesSinceLastBigSection << endl;
    }

  private:
    LeafLooper &ll1, &ll2;
    MeterMaid &beatCycleLookup, &hyperbeatCycleLookup, &smallSectionCycleLookup, &bigSectionCycleLookup;
    float &horizontalMotionRadius, &verticalMotionRadius, &turnSpeed;
    Nav& nav;
    int beatNum, hyperbeatNum, smallSectionNum, bigSectionNum;
    int framesSinceLastBeat = 0, framesSinceLastHyperBeat = 0, framesSinceLastSmallSection = 0, framesSinceLastBigSection = 0;
    float beatPhase, hyperbeatPhase, smallSectionPhase, bigSectionPhase;
};

// int main() {
//   MeterMaid rita("LeafLoopsDownbeats.txt"); 
//   int beatNum = 0;
//   std::cout << rita.getPhasePosition(300, beatNum) << ", " << beatNum << std::endl;
// }

#endif