/*
定义一个类 Student    
			id name next
1 每定义一个学生对象，让他的id　从1001自动增加
2 每创建一个学生都作为结点添加到链表里
	定义一个静态的头指针

只有一个Student类，不能有其他的辅助类型

Student* head = nullptr;

Student s1;	//head = &s1
Student s2; 
Student s3; 
//s1.next = &s2;
//s2.next = &s3;
*/
#include <iostream>
using namespace std;
class Student
{
	int id;
	string name;
	Student* next;
	static int count;
public:
	static Student* head;
	//尾插
	Student() : id(++count), next(nullptr) {
		if (head == nullptr) 
			head = this;
		else {
			auto p = head;
			//找到尾结点的地址
			while(p->next != nullptr)
				p = p->next;
			p->next = this;
		}
	}
	//头插
	Student(string n) : 
		id(++count), name(n), next(head) 
	{
		head = this;
	}
	~Student() {
		cout << "delete " << id << endl;
		if (this == head) 
			head = head->next;
		else {
			auto p = head;
			while(p->next != this) 
				p = p->next;
			p->next = this->next;
		}
	}

	void show() {
		cout << *this << endl;
		//cout << id << " " << name << endl;
	}
	static void showList() {
		auto p = head;
		cout << "全部:" << endl;
		int i = 0;
		while(p) {
		//	cout << p->id << " " << p->name << endl;
			cout << *p << " 第" << ++i << "个结点" <<endl;
			p = p->next;
		}
		cout << "------------------" << endl;
	}
	friend ostream& operator<<(ostream& out, const Student& s);

};
int Student::count = 1000;
Student* Student::head = nullptr;

ostream& operator<<(ostream& out, const Student& s)
{
	out << s.id << " " << s.name;
	return out;
}

int main()
{
	Student s1("Tom"); // id == 1002
	Student* p = new Student("Mike");//id == 1001
	Student s2("Jack"); // id == 1003
	{
		Student s3("Bob");
		cout << "s3 ";
		s3.showList();
	}
	Student s4("Cindy");
	Student s5("David");
	cout << "s5 ";
	Student::showList();
	cout << s1 << " " << s2 << endl;
	p->show();
//	s1.show();
//	s1.showList();
//	p->showList();
	delete p;
	cout << "delete p ";
	Student::showList();

	return 0;
}
