#include <iostream>
using namespace std;
class Animal
{
protected:
	string name;
	int age;
	string sex;
	bool birth;
public:
	Animal() {}
	Animal(string n, int a, string s, bool b) :
		name(n), age(a), sex(s), birth(b) {
	}
	virtual void show() = 0;
	virtual ~Animal() { cout << "~A" << endl;} 
};
class Horse : virtual public Animal
{
public:
	Horse() { }
	Horse(string n, int a, string s) :
		Animal(n, a, s, true)
	{
	}
	void show() override {
		cout << "马 " << endl;
	}
	~Horse() {cout << "~H" << endl; }
};

class Donkey : virtual public Animal
{
public:
	Donkey() { }
	Donkey(string n, int a, string s) :
		Animal(n, a, s, true)
	{
	}
	void show() override {
		cout << "驴 " << endl;
	}
	~Donkey() {cout << "~D" << endl; }
};

class Mule : public Horse, public Donkey
{
public:
	Mule() { }
	Mule(string n, int a, string s) :
		Animal(n, a, s, false)
	{
	}
	~Mule() {cout << "~M" << endl; }
	void show() override {
		cout << "骡子 " << endl;
	}
	
};

int main()
{
	Animal* p = new Horse("Mark", 2, "m");
	p->show();
	delete p;
	p = new Donkey("Mike", 1, "f");
	p->show();
	delete p;
	p = new Mule("Tom", 3, "m");
	p->show();
	delete p;

	return 0;
}
