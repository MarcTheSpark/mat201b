#include <iostream>
using namespace std;
#define SIZEOFARRAY(x) (sizeof(x) / sizeof(x[0]))

int sumOfIntArray(int a[], int length) {
  int sum = 0;
  cout << sizeof(*a[0]);
  for (int i = 0; i < length; i++) sum += a[i];
  return sum;
}

int main() {
  int ar[] = {1, 1, 2, 3, 5, 8, 13, 21, 34, 55};
  cout << sumOfIntArray(ar, SIZEOFARRAY(ar)) << endl;
}