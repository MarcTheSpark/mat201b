// Example program
#include <iostream>
#include <string>
#include <random>
using namespace std;

int main()
{
  uniform_real_distribution<> dist(0, 10);
  random_device rd;
  mt19937 e2(rd());
  cout << "hello" << dist(e2); 
}
