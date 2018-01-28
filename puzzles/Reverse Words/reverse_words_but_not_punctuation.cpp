// C++ puzzle 0
// Reverse each word in a sentence using std:stack
//
// Compile and run this program in the shell like this:
//
// $ c++ -std=c++11 reverse_words.cpp -o reverse_words
//
// $ ./reverse_words
//
// Read this code carefully, studying each line. Run
// the compiled program several times, trying different
// inputs. Leave most of the code alone. Find where I
// say "TODO - put your code here" and there add some
// code that uses std::stack and basic C++ offerings
// such as loops and conditionals to print out the users
// sentence with each word reversed. See the comments
// at the end of this file for examples of what a
// solution would produce.
//
// Look at these links about std::stack
//
// - https://www.thegeekstuff.com/2016/09/cpp-stack-examples
// - http://www.cplusplus.com/reference/stack/stack
// - http://en.cppreference.com/w/cpp/container/stack
//
// Ask questions and seek help on Gitter.
//
// -- Karl Yerkes / 2018-01-15
//
//
#include <iostream>  // cout, cin, printf, getline
#include <stack>
#include <string>
using namespace std;

int main() {
  // give the user an explaination of how to use this program
  //
  cout << R"(
  Reverse Words
    Type a line of text and I will print that line with each word reversed.
    To quit, leave the line blank and press ENTER or say CTRL-D.

)";

  // wait for a line of text from the user
  //
  string line;
  cout << "> ";
  getline(cin, line);

  // if the line is not blank, do something with it,
  // then get wait for a new line.
  //
  while (line != "") {
    // give the user back her line of text, unharmed
    //
    cout << line << endl;

    // show each character (aka char) in the line of text; see how
    // std::string may be used line an array (e.g., line[index])?
    // also, we may ask its length.
    //
    for (int i = 0; i < line.length(); i++) cout << "'" << line[i] << "' ";
    cout << endl;

    // show the integer value of each character in the line of text;
    // note the alternative for-loop syntax? also, we may use printf
    // as an alternative to c++ streams (i.e., <<, cout, cin, endl).
    // this also shows how to use "leading spaces" (3) with printf.
    //
    for (auto c : line) printf("%*d ", 3, (int)c);
    printf("\n");

    // use std::stack along with C++ offerings such as loops and
    // conditionals to reverse each word in the users sentence but
    // leave the sentence structure intact.

  // My Approach:iterate through the string backwards, thereby reversing 
  // both word order and the order of letters within each word
    stack<string> sentenceBackwards = stack<string>();
    string currentWord = "";
    
    for (int i = line.length()-1; i >= 0; i--) {
      // in this version, anything that is not a letter is treated as its own word
      // so those portions don't get reversed
      if(!isalpha(line[i])) {
        sentenceBackwards.push(currentWord);
        sentenceBackwards.push(string(1, line[i]));
        currentWord = "";
      } else {
        currentWord += line[i];
      }
    }
    sentenceBackwards.push(currentWord);
    while(!sentenceBackwards.empty()) {
      string thisWord = sentenceBackwards.top();
      // This checks if we reversed a word that starts with a capital, and if so, 
      // makes the reversed word start, rather than end, with a capital.
      if(isupper(thisWord[thisWord.length()-1]) && islower(thisWord[0])) {
        thisWord[0] = toupper(thisWord[0]);
        thisWord[thisWord.length()-1] = tolower(thisWord[thisWord.length()-1]);
      }
      cout << thisWord;
      sentenceBackwards.pop();
    }
    cout << endl;

    // wait for the user to enter another line
    //
    cout << "> ";
    getline(cin, line);
  }
}

// This is an example of a session with a solution to this puzzle:
//
// ix $ ./reverse_words_solution
//
//   Reverse Words
//     Type a line of text and I will print that line with each word reversed.
//     To quit, leave the line blank and press ENTER or say CTRL-D.
//
// > this is a sentence
// this is a sentence
// 't' 'h' 'i' 's' ' ' 'i' 's' ' ' 'a' ' ' 's' 'e' 'n' 't' 'e' 'n' 'c' 'e'
// 116 104 105 115  32 105 115  32  97  32 115 101 110 116 101 110  99 101
// siht si a ecnetnes
// > racecar radar kayak diefied level alula
// racecar radar kayak diefied level alula
// 'r' 'a' 'c' 'e' 'c' 'a' 'r' ' ' 'r' 'a' 'd' 'a' 'r' ' ' 'k' 'a' 'y' 'a' 'k' '
// ' 'd' 'i' 'e' 'f' 'i' 'e' 'd' ' ' 'l' 'e' 'v' 'e' 'l' ' ' 'a' 'l' 'u' 'l' 'a'
// 114  97  99 101  99  97 114  32 114  97 100  97 114  32 107  97 121  97 107
// 32 100 105 101 102 105 101 100  32 108 101 118 101 108  32  97 108 117 108 97
// racecar radar kayak deifeid level alula
// > state-of-the-art design
// state-of-the-art design
// 's' 't' 'a' 't' 'e' '-' 'o' 'f' '-' 't' 'h' 'e' '-' 'a' 'r' 't' ' ' 'd' 'e'
// 's' 'i' 'g' 'n' 115 116  97 116 101  45 111 102  45 116 104 101  45  97 114
// 116  32 100 101 115 105 103 110 tra-eht-fo-etats ngised
// >
// ix $
