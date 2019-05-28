#include <iostream>
using namespace std;

#define I16 short
#define PI16 short*

typedef int I32;
typedef int* PI32;

//c++11
using F64 = double;
using PF32 = float*;

int main()
{
	int a;
	I32 b;
	I16 c;
	PI32 p1, p2;	//
	PI16 p3, p4;	//short  *p3, p4;
	
	F64 d;
	cout << sizeof(d) << endl;

	return 0;
}
