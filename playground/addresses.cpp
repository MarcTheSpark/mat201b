#import <iostream>
using namespace std;

struct Bob {
	float bobfloat = 5;
};

int main() {
	Bob b;
	Bob * bobPointer = &b;
	Bob& bobRef = b;
	cout << b.bobfloat << endl;
	cout << (*bobPointer).bobfloat << endl;
	cout << bobRef.bobfloat << endl;
}