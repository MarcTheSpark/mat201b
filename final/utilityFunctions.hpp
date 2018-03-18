/*
  Marc Evans (2018/3/8)
  Final Project Utility Functions
*/

#ifndef __UTILITIES__
#define __UTILITIES__

#include "allocore/al_Allocore.hpp"
#include <fstream>


std::string fullPathOrDie(std::string fileName, std::string whereToLook = ".") {
  al::SearchPaths searchPaths;
  searchPaths.addSearchPath(whereToLook);
  searchPaths.print();
  std::string filePath = searchPaths.find(fileName).filepath();
  if (filePath == "") {
    fprintf(stderr, "ERROR loading file \"\" \n");
    exit(-1);
  }
  return filePath;
}

void loadDownbeats(std::string pathToDataFile, std::vector<float>& downbeats) {
  std::ifstream input(fullPathOrDie(pathToDataFile));
  for( std::string line; getline( input, line ); ) {
      downbeats.push_back(stof(line));
  }
};

#endif