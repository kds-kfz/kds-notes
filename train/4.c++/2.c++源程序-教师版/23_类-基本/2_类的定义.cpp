#include <iostream>
using namespace std;
struct Student
{
	//成员变量/成员对象
	int id;
	string name;
	int score;
	//成员函数/成员方法
	void show() {
		cout << "s.show() " << endl;
		cout << id << " " << name << " " << score <<     endl;
	}
	void show(Student s) {
		cout << "s.show(s) " << endl;
		cout << s.id << " " << s.name << " " << s.score << endl;
		cout << id << name << score << endl;
	}
	void show(int id) {
		cout << id << " " << name << " " << score <<     endl;
	//this 是一个指向本对象的指针 this == &s1
	//							  *this == s1
	//	this = new Student; 错误, 不能换指向
	// Student const* this = &s1;
		cout << this->id << " " << name << " " << score <<     endl;
	} 
	void show(Student* const self) {
	//	self = new Student;
		cout << "*" << endl;
		cout << self->id << " " << self->name << " " << self->score << endl;
	}
};
void show(Student s) 
{
	cout << s.id << " " << s.name << " " << s.score << endl;
}
int main()
{
	cout << "size = " << sizeof(Student) << endl;
	Student s1 = {1001, "Tom", 90};
	Student s2 = {1002, "Mike", 89};
	show(s1);
	show(s2);
	s1.show(&s1);
	s2.show();
	string s;
	s.size();
	
//	size(s);


	return 0;
}
