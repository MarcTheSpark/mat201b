/*
  Marc Evans (2018/3/8)
  Final Project Meter Maid
  A class for keeping track of the position within a bar or higher lever metrical structure
*/

#ifndef __METER_MAID__
#define __METER_MAID__

#include "utilityFunctions.hpp"
#include <cassert>


static int dummyIntReference;

class MeterMaid {
  
  public:
    MeterMaid(std::string pathToDownbeatsFile) {
      loadDownbeats(pathToDownbeatsFile, downbeats);
      assert(downbeats.size() > 0);
    }

    float getPhasePosition(float t, int& cycleNum=dummyIntReference) {
      if(t < downbeats.at(0)) { cycleNum = 0; return 0; }
      if(t >= downbeats.at(downbeats.size()-1)) { cycleNum = downbeats.size(); return 0; }

      cycleNum = 0;
      while(cycleNum < downbeats.size()) {
        if(t >= downbeats.at(cycleNum)) {
          cycleNum++;
        } else {
          break;
        }
      }

      return (t - downbeats.at(cycleNum-1)) / (downbeats.at(cycleNum) - downbeats.at(cycleNum-1));
    }

  private:
    std::vector<float> downbeats;
};

// int main() {
//   MeterMaid rita("LeafLoopsDownbeats.txt"); 
//   int beatNum = 0;
//   std::cout << rita.getPhasePosition(300, beatNum) << ", " << beatNum << std::endl;
// }

#endif