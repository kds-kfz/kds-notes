#include <iostream>
using namespace std;

int num = 1000;

class Person
{
	string name;
	int age;
public:
	int id;
	Person(string n="--", int a=0) : id(++count), name(n), age(a) 
	{	
		cout << this->count << endl;	
	}
	//静态函数，没有this指针, 不能访问类的普通成员变量
	//能访问静态成员变量
	//同一个函数不能有静态和非静态两种版本
	static int get() {
		return count;
	}
	int getId() {
		return id;
	}
	friend ostream& operator<<(ostream& out, const Person& p);
	//静态成员变量，整个类共用一个变量
	static int count;
};
//对静态成员的初始化，
//*必须*在类外对静态成员进行全局的初始化
int Person::count = 1000;//初始化

ostream& operator<<(ostream& out, const Person& p)
{
	out << p.id << " " << p.name << " " << p.age;
	return out;
}
/*
栈区
p1				p2
---------		-------
id				id
name			name
age				age
－－－－－－－－－－－－－－－
数据区
	    count
*/
int main()
{
	Person::count = 1212;	 //赋值
	Person p1("Tom", 12);
//	p1.id p1.name		p1.count
	Person p2("Mike", 13);
//	p2.id p2.name		p2.count
//	p1.count = 999;
	cout << p1.get() << endl;
	cout << p2.get() << endl;
	//通过类访问静态成员对象
	cout << Person::get() << endl;
//	cout << Person::id << endl;
	cout << sizeof(p1) << endl;
	cout << sizeof(p2) << endl;
	cout << p1.get() << endl;
	return 0;
}
