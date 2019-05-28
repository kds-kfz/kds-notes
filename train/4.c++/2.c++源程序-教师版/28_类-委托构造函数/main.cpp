#include <iostream>
using namespace std;

class Student
{
	int id;
	string name;
	int age;
public:
//	构造		委托构造函数, 一个构造函数可以委托其他的构造函数实现赋值
	Student() : Student(0, "", 0) {
		cout << "Student 0" << endl; 
	//	id = 0;
	//	age = 0;
	}
	//初始化列表, 数组不能使用
	//初始化顺序是定义的顺序
	Student(int i, string n, int a) : 
		id(i), name(n), age(a)
	{
		cout << "Student 3 " << endl;
		show();
//		id = i;
//		name = n;
//		age = a;
	}
//	Student s; 

//	拷贝构造
	Student(const Student& s) :
		id(s.id), name(s.name), age(s.age)
	{
		cout << "copy " << endl;
//		id = s.id;
//		name = s.name;
//		age = s.age;
	}
	void show() { 
		cout << id << " " << name << " " << age << endl;
	}
};

int main()
{
	Student s1;
	Student s2(1001, "Tom", 12);
	Student s3(s2);
//	Student s4(1001);
	s1.show();
	return 0;
}
