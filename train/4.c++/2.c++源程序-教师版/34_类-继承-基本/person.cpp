#include <iostream>
using namespace std;

class Person
{
protected:
	int id;
	string name;
	int age;
public:
	void show() {
		cout << id << " " << name << " " << age << " ";
	}
};

class Student : public Person
{
	int score;
public:
	void set(int i, string n, int a, int s) {
		id = i;
		name = n;
		age = a;
		score = s;
	}
	void show() {
		Person::show();
		cout << score << endl;
	}
};
class Teacher : public Person
{
	string subject;
public:
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
};

int main()
{

	Student s;
	s.set(1001, "Tom", 12, 90);
	s.show();
	Teacher t;
//	t.set(2001, "Mike", 30, "c");
	t.show();
	return 0;
}
