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
	//虚函数
	virtual void show() {
		cout << "人 ";
		cout << id << " " << name << " " << age << endl;
	}
	//使用基类指针释放派生对象，基类的析构函数一定要定义成虚析构函数
	virtual ~Person() {
		cout << "~Person" << endl;
	}
};

class Student : public Person
{
public:
	int score;
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
		cout << "学生 ";
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
	//如果基类里有virtual void show()
	//派生里的void show(), 被重写的show函数
	void show() {
		cout << "教师 ";
		cout << id << " " << name << " " << age << " ";
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
//**多态: 当派生类里有重写的函数的时候，基类指针p指向任意派生的对象，p就访问哪个对象的函数**\\
	
	Person* p = new Person(1001, "Tom", 32);
//	Person p(1001, "Tom", 12);
//	Student s;//调用基类构造函数和自身的构造函数	
//	s.set(1001, "Tom", 12, 90);
//	s.show();

	Teacher* t = new Teacher(2001, "Mike", 30, "math");
//	Teacher t(2001, "Mike", 32, "math");
//	t.show();
	p->show();
	delete p;
//	Person& r = t;	/**多态只能通过指针和引用实现**\
//	r.show();
	p = t;//小的指针指向大的空间是合法的 
			//基类指针可以指向派生对象
			//派生指针不能指向基类对象	
//	t = p;	大的指针指向小的空间,会访问到不合法的位置
	p->show();
	delete p;
//	p = new Student;

//	p.show();
	//基类指针不能访问派生的成员变量
	//cout << p->score << endl;
	return 0;
}
