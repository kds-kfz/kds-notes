#include <iostream>
using namespace std;

class Person
{
	int id;
	string name;
	int age;
public:
	//构造函数，与类名完全相同，没有返回值, 用来初始化对象, 在定义对象的时候被调用
	Person(int i, string n, int a) {
		cout << "id " << id <<
			"name " << name << 
			"age " << age << endl;
		id = i;
		name = n;
		age = a;
	}
	//不写任何构造函数的情况下，会产生默认的无参构造函数
	Person() {
		id = 0;
		age = 0;
	}
	void init(int i, string n, int a) {
		id = i;
		name = n;
		age = a;
	}
	void show() {
		cout << id << " " <<name << " " << age << endl;
	}
};
int main()
{
//	Person p1 = {1001, "Tom", 12};//c++11 列表初始化
	Person p1(1001, "Tom", 12);// 调用了构造函数
	p1.show();
	Person p2;//调用默认的无参构造函数，Person() 
	//Person p2(); 声明了一个叫做p2的函数
	p2.show();
//	p1.init(1001, "Tom", 12); 调用普通函数
	return 0;
}
