#include <iostream>
using namespace std;
class Person
{
protected:
	string name;
	int age;
public:
	Person(string n="", int a=0) : name(n), age(a) { }
	virtual void show() {
		cout << "人 " << name << " " << age << "岁" << endl;
	}
};
class Teacher
{
protected:
	string school;
	string subject;
public:
	Teacher(string sc="", string su="") : school(sc), subject(su) { 
	}
	void show() {
		cout << "教师 学校:" << school << " 学科:" << subject << endl;
	}
};
class Proffessor : public Person, public Teacher
{
	string titel;
public:
	Proffessor() = default;
	Proffessor(string n, int a, string sc, string su, string t) : Person(n, a), Teacher(sc, su), titel(t) { }
	void show() {
		cout << "教授 名字:" << name << " 年龄:" <<
			age << " 学校:" << school << " 学科:" <<
			subject << " 职称:" << titel << endl;
	}
};
int main()
{
	
	Proffessor p("Tom", 30, "清华", "数学", "讲师");
	p.show();
	Person p1("Mike", 23);
	Teacher p2("一中", "英语");
	Person* q = &p1;
	
	q->show();
	q = &p;
	q->show();
	
//	q = &p2;
	

	return 0;
}
