#include <iostream>
using namespace std;

class Person
{
protected:
public:
	int id;
	string name;
	int age;
	Person(int i, string n, int a) : id(i), name(n), age(a) {
		cout << "Person 3 " << id << endl;
	}
	Person() {
		cout << "Person 0 " << id << endl;
	}
	void show() {
		cout << id << " " << name << " " << age << " ";
	}
	~Person() {
		cout << "~Person" << endl;
	}
};

class Student : public Person
{
	int score;
public:
/*	
	Student() {//Preson() 
		cout << "Student " << endl;
	}
	*/
	void set(int i, string n, int a, int s) {
		id = i;
		name = n;
		age = a;
		score = s;
	}
	void show() {
		Person::show();
		cout << id << " " << name << " " << age << " ";
		cout << score << endl;
	}
	~Student() {
		cout << "~S " << id << endl;
	}
};
class Teacher : public Person
{
	string subject;
public:
	//构造派生对象,默认调用基类的无参构造函数
	Teacher() : Person(0, "", 0){ //Person() 
		cout << "Teacher " << endl;
	}
	Teacher(int i, string n, int a, string s) 
		: /*id(i),错误 Person部分还没初始化,还没有id*/
		 Person(i, n, a), subject(s)
	{
		cout << "Teacher 4 " << id << endl;
		// id = i;
	}
	void set(int i, string n, int a, string s) {
		id = i;
		name = n;
		age = a;
		subject = s;
	}
	void show() {
		Person::show();
		cout << subject << endl;
	}
	~Teacher() {
		cout << "~T " << id << endl;
	}
};
/*
Person p		Teacher t			
----------		--------
id				id	
name			name	Person 1	
age				age		
				=======	
				score	Teacher 2
*/
int main()
{
	Person p(1001, "Tom", 32);
	Student s;//调用基类构造函数和自身的构造函数
	
	s.set(1001, "Tom", 12, 90);
	s.show();
	Teacher t(2001, "Mike", 30, "math");
	//Person()
	//Teacher()
	//~Teacher()
	//~Person()
	//t.id t.name t.age || t.subject
	//	Person			||
	//	             Teacher
//	t.set(2001, "Mike", 30, "c");
	t.show();
	return 0;
}
