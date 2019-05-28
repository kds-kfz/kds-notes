#include <iostream>
using namespace std;

int main()
{
	string s1("hello");
	string s2("world");
	s1.size();
	//.empty()判断是否是空的字符串
	cout << "is empty ? " << s1.empty() << endl;
//	s1[5] = 'q';
//	cout << s1[1] << endl;

//	s2[0] = 'a';
//	cout << s2[0] << endl;
//	s1.resize(8);
	//赋值
//	s2 = s1;
	s2 = s1 + s2;
	s2 = s2 + 'w';
	s2 = s2 + "we";
	s2 += 97;
//	s2 = s2 + 97;	//不支持
	//如果string连续加法，有可能会出错,尽量每次只执行一次加法
//	s2 = s2 + s1;//s2 += s1;
	if (s1 > s2) {
		cout << "s1 > s2" << endl;
	} else {
		cout << "s1 <= s2" << endl;
	}
	/*
	for (int i=0; i<s2.size(); i++) {
		cout << s2[i] << ",";
	}
	cout << endl;
*/
	//.erase(pos, len)	从pos的位置删除len个字符
	s1.erase(1, 10);
	
	//.insert(pos, args) args : const char* , string
	//	从pos的位置插入一个字符串
	s1.insert(1, "ooo");
	
	//.replace(pos, len, args) 从pos开始,len个字符被args替换
	s1.replace(1, 2, "zzz");
	cout << s1 << endl;
	
	//.substr(pos, len) 提取一段字符串返回
	string q = s1.substr(1, 4);

	cout << q << endl;
	cout << s1 << endl;

/*	c语言的string.h内的函数
	strlen	.size()
	strcpy	=
	strcat	+	拼接	"hello\0\0" "world"	到\0的位置
	strcmp	> < >= <= != ==
*/

	//.resize(n)	重置大小
/*	s2.resize(s1.size());
	for (int i=0; i<s1.size(); i++) {
		s2[i] = s1[i];
	}
*/	
	cout << "s1 = " << s1 << " s1.size() " << s1.size() << endl;
	cout << "s2 = " << s2 << " s2.size() " << s2.size() << endl;


	return 0;
}
