#include <iostream>
using namespace std;

struct Person
{
protected://受保护的，本类内和它的派生类可以访问
		  //外部不可访问
private://只能在类内直接访问
	int id;
	string name;
	string sex;
public://类内和类外都能访问
	void init(int id, string name, string sex) {
		this->id = id;
		this->name = name;
		this->sex = sex;
	}
	void show() {
		cout << id << " " << name << " " << sex << endl;
	}
	void setName(string name) {
		this->name = name;
	}
};

int main()
{		
	//如果成员是私有的，不能这样直接初始化
	Person p1;// = {1000, "An", "female"};
	p1.init(1001, "Tom", "male");
	p1.show();
//	p1.id = 1002;
//	p1.name = "Mike";
	p1.setName("Mike");
	p1.show();
	return 0;
}
