// C++ puzzle 1
// Sort a list of {count, word} pairs.
//
// Compile and run this program in the shell like this:
//
// $ c++ -std=c++11 sort_corpus.cpp -o sort_corpus
//
// $ cat Bible-Corpus-With-Count-Random.txt | ./sort_corpus
//
// Read this code carefully, studying each line. Leave
// most of the code alone. Find where I say "TODO" and
// put your code there. Your edits should make this
// program sort the input numerically in ascending
// order resulting in the output shown in this file:
//
//   Bible-Corpus-With-Count-Sorted.txt
//
// In other words, you program should act like "sort -n"
// on the terminal. Here's how you might use "sort -n"
// on Bible-Corpus-With-Count-Random.txt:
//
// $ cat Bible-Corpus-With-Count-Random.txt | sort -n
//
// Possible Google search terms and/or hints include:
// - comparator
// - functor
// - function object
// - bool operator()(...)
// - bool operator<(...)
//
// Ask questions and seek help on the Gitter.
//
// -- Karl Yerkes / 2018-01-22
//
//
#include <algorithm>  // sort
#include <iostream>   // cout, cin, printf, getline
#include <fstream> // added this so I could print to a file and run a diff
#include <string>     // string, stoi
#include <vector>     // vector
using namespace std;

// HINT
//
//   int std::stoi(const std::string& str)
//
// This function parses an integer in a given string.
// Each of these strings would get the integer 12:
//   "12"
//   "  12"
//   "12foo"
//   "  12 foo"

int main() {
  struct Data {
    string line;
    int number;
    string word;

    Data(const string& s) {
      line = s;
      int spaceLoc = s.find(' ');
      number = stoi(s.substr(0, spaceLoc));
      word = s.substr(spaceLoc+1);
    }
    bool operator <(const Data &dataObj) const
    {
      if (number == dataObj.number) {
        // same number, so sort by word
        return word < dataObj.word;
      } else {
        // different number, so sort by number
        return number < dataObj.number;
      }
    }
  };

  // Turn each given line into a Data object and append
  // these to a std::vector. Leave this part alone unless
  // you want to get fancy.
  //
  vector<Data> data;
  for (string line = ""; cin.good(); getline(cin, line)) {
    if (line != "") data.push_back(Data(line));
  }

  // TODO - use std::sort on our std::vector<Data> named
  // 'data'. There are several ways to do this. You can
  // add a comparator to the Data class and call std::sort
  // with just two arguments. Or, you can write a function,
  // functor, or lambda and pass it as the third argument
  // to std::sort. The first two arguments to std::sort are
  // the begining an end of the std::vector as shown in the
  // next comment.
  //

  sort(data.begin(), data.end());
  
  //
  // sort(data.begin(), data.end());
  //    OR
  // sort(data.begin(), data.end(), ...);
  //

  // Iterate though our std::vector<Data>, printing each
  // to the terminal. Leave this part alone unless you
  // want to get fancy.
  //

  // added this so I could print to a file and run a diff (results were successful)
  ofstream output ("marcSortCorpusOutput.txt");
  for (auto& d : data) {
    cout << d.line << endl;
    if(output.is_open()) {
      output << d.line << endl;
    }
  }
  output.close();
}
