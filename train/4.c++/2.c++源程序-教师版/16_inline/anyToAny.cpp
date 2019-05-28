#include <iostream>
#include <cmath>
using namespace std;

int anyToInt(string s, int n)
{
	int num = 0;
	int a = 0;
	for (int i=s.size()-1, j=0; i>=0; i--, j++)
	{
		if (s[i] <= '9' && s[i] >= '0')
			a = s[i] - '0';
		else if (s[i] <= 'Z' && s[i] >= 'A')
			a = s[i] - 'A' + 10;
		else if (s[i] <= 'z' && s[i] >= 'a')
			a = s[i] - 'a' + 10;
		
		num += a * pow(n, j);
	}
	return num;
}
			//42  
string intToAny(int num, int n)
{
	string r;
	int t = 0;
	string s = "0123456789abcdefghijklmnopqrstuvwxyz";
	while(num) {
		t = num % n;
		r = s[t] + r;
		num /= n;
	}
	return r;
}

string anyToAny(string a, int n1, int n2)
{
	int num = anyToInt(a, n1);
	string r = intToAny(num, n2);
	return r;
}

int main()
{
	string a = "123";	//42
		//		(1*10 +2)*10 + 3
		//		(1*8 + 2)*8 + 3
		//		3*1 + 2*8 + 1*8*8
						//
	string b = anyToAny(a, 8, 2);
//	int n = anyToInt(a, 2);	
	cout << b << endl;
	return 0;
}
