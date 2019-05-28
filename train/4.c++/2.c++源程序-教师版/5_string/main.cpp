
//#include <string>	//内置的类型 与基本数据类型不同
#include <iostream>
using namespace std;
int main()
{
	char a1[10] = "hello";
	a1[0] = 's';
	//a1 = "world";	错误
	
//	char* const p;	指向不能变
	const char* a2 = "hello";//指向的内容不能变
	//a2[0] = 's';	//段错误
	a2 = "world";
/*
struct String	
{
	char* p;
	size()....
};
*/
//	String s;
//	s.p;

	std::string s1;
	using std::string;
	string s2 = "hello";	//拷贝初始化
	string s3("world");		//直接初始化
	string s4 = s3;			//拷贝初始化
	string s5(10, 'a');		//直接初始化  10个a

	cout << "s1 = " << s1 << endl;
	cout << "s2 = " << s2 << endl;
	cout << "s3 = " << s3 << endl;
	cout << "s4 = " << s4 << endl;
	cout << "s5 = " << s5 << endl;
	//sizeof	测string类型的大小
	cout << "sizeof = " << sizeof(s2) << endl;
	cout << "sizeof = " << sizeof(s1) << endl;
	//.size()	测某个具体对象的字符串长度
	cout << "s1 : size " << s1.size() << endl;
	cout << "s2 : size " << s2.size() << endl;
	cout << "s5 : size " << s5.size() << endl;



	return 0;
}
