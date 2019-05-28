#include <iostream>
using namespace std;

//有纯虚函数的类叫做抽象类, 抽象类不能定义对象
class Person
{
protected:
	int id;
	string name;
public:
	Person() : id(0) { }
	Person(int i, string n) : id(i), name(n) { }
	//纯虚函数，没有函数体 
	virtual void show() = 0;
	virtual bool isGood() = 0;
	virtual ~Person() { 
		cout << "~P " << endl;
	}
};
//当基类是抽象类，派生类如果要定义对象必须对基类的虚函数进行重写
class Student : public Person
{
	int score;
public:
	Student() : Person(), score(0) { }
	Student(int i, string n, int s) : Person(i, n), score(s) { }
	virtual void show() {
		cout << "id: " << id << "  name:" << name <<
			"  score:" << score << endl;
	}
	bool isGood() {
		return score >= 90;
	}
	~Student() {
		cout << "~S" << endl;
	}
};

class Teacher : public Person
{
	int count;
public:
	Teacher() : count(0) { }
	Teacher(int i, string n, int c) : Person(i, n), count(c) { }
	void show() {
		cout << "id: " << id << "  name:" << name <<
			"  count:" << count << endl;
	}
	bool isGood() {
		return count >= 3;
	}
	~Teacher() {
		cout << "~T" << endl;
	}
};
int main()
{
//	Person p; 抽象的类不能定义实例
//	Teacher t;
	Person* p;// = new Person;	错误
	Person* arr[8];
	int id;
	string name;
	int score;
	int count;
	for (int i=0; i<4; i++) {
		cin >> id >> name >> score;
		arr[i] = new Student(id, name, score);
	}
	for (int i=4; i<8; i++) {
		cin >> id >> name >> count;
		arr[i] = new Teacher(id, name, count);
	}
	cout << "优秀的学生和教师名单:" << endl;
	for (auto m : arr) {
		if (m->isGood())
			m->show();
		delete m;
	}

	return 0;
}
