#include <iostream>
#include <stdio.h>
using namespace std;


int main()
{
	string s("hello\0world");
	
	s.resize(8);
	s += "world";
	//cout可以输出\0
	cout << s << endl;

	//c语言打印，遇到\0停止
	printf("%s\n", &s[0]);
	s[0] = 'w';
	const char *p = s.c_str();//返回内部字符串的首地址
//	*p = 'z';
	printf("%s\n", s.c_str());
	
//	cin >> s;		//遇到空白字符结束
	getline(cin, s);//遇到换行结束	
	cout << s << endl;
	return 0;
}
练习: 输入一行字符串,进行倒置
"hello 今天很热" ---> "热很天今 olleh"
0~127 是英文，否则是中文


