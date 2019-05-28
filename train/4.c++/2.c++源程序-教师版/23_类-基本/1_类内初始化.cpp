#include <iostream>
using namespace std;
//c++11 类内初始化
struct Student
{
	int id = 0;
	string name = "--";
	int score;
};
void show(Student s)
{
	cout << s.id << " " << s.name << 
		" " << s.score << endl;
}
int main()
{
	//s1　是一个变量，c++里叫做对象,　也可以叫做实例
	Student s1;
	show(s1);
	//类内初始化和变量的初始化只能使用一种
	Student s2;// = {1001, "Tom", 90};
	show(s2);

	return 0;
}
