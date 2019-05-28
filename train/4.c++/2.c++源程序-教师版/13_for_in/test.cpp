/*
练习：
	void toUpper(string& s);
	string removeSpot(string s);//移除标点符号
	string toHex(int n);	//转换为16进制
*/
#include <iostream>
using namespace std;


void toUpper(string& s)
{
	for (auto& c : s) {
		if (c <= 'z' && c >= 'a')
			c -= 32;
	}
}
string removeSpot(string s) 
{
	string res;
	for (auto c : s) {
		if (	c <= 'z' && c >= 'a' || 
				c <= 'Z' && c >= 'A' ||
				c <= '9' && c >= '0')
			res += c;
	}
	return res;
}
string toHex(int num) // 123
{
	int a = 0;
	string s = "0123456789abcdef";
	string res;
	while(num) {
		a = num % 16;// 0~15	'0'~'9' 'a'~'f'
		res = s[a] + res;//把数字转成字符
		num /= 16;
	}
	res = "0x" + res;
	return res;
}
int main()
{
	string s = "Hello! my friend.";
	toUpper(s);
	cout << s << endl;
	string r = removeSpot(s);
	cout << r << endl;
	r = toHex(2048); //2^11	8*16*16
	cout << r << endl;
	return 0;
}
