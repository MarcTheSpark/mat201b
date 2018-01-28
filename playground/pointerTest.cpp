#include <iostream>
using namespace std;

int main() {
	int a = 5;
	int * b = &a; // b gets the memory address of a
	*b = 9; // overwrite a, the value pointed to by be
	cout << a << endl; // print a
	cout << &a << endl; // print the memory address of a
	cout << b << endl; // print b (same)
	cout << &b << endl; // print the memory address of b (pointer to a pointer)
	cout << *&b << endl; // dereference that to get b (the memory address of a)
	cout << **&b << endl; // dereference that to get a back
}