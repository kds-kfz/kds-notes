#include <iostream>

using namespace std;

struct Student
{
	int id;
	char name[20];
};

int main()
{
	//c++里可以省略struct关键字
	Student stu;// == struct Student stu;

	cout << "请输入id 和姓名, 用空格分开" << endl;
	cin >> stu.id >> stu.name;
	cout << "id :" << stu.id 
		<< "  name :" << stu.name 
		<< endl;

	return 0;
}
