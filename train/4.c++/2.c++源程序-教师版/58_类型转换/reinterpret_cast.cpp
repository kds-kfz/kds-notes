#include <iostream>
using namespace std;
//reinterpret_cast 转换底层的二进制数据
int main()
{
	int i = 0x12345678;
/*	
	大端
	12			34			56			78
	0001 0010| 0011 0100| 0101 0110| 0111 1000
	18			52			86		 120
	小端
	78 56 34 12
*/

/*
   大端
   高位								低位
   0000 0000 0000 0000 0000 0000 0000 1010
   小端
   0000 1010 0000 0000 0000 0000 0000 0000 
*/
	char n = i;
	cout << (int)n << endl;
	char* c = reinterpret_cast<char*>(&i);
	for (int i=0; i<4; c++, i++)
		cout << hex << (int)*c << endl;
	
	return 0;
}
