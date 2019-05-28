#include <iostream>
//教师版，单链表，基类创无参构造链表，派生继承
//派生申请空间时，就可以建好单链表
using namespace std;
class Person
{
    protected:
	int id;
	string name;
	Person* next;
    public:
	static Person* head;
	Person() : id(0), next(head) { 
	    head = this;
	}
	Person(int i, string n) : id(i), name(n), next(head) { 
	    head = this; 
	}
	Person* getNext() { return next; }
	virtual void show() { }
	virtual bool isGood() { return false; }
};
Person* Person::head = nullptr;

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
};
int main()
{
    //	Person* arr[8];
    int id;
    string name;
    int score;
    int count;
    //编译成功后，运行./aout < t.txt
    //将文本内容输入到可执行文件
    //t.txt保存的是输入的内容
    for (int i=0; i<4; i++) {
	cin >> id >> name >> score;
	new Student(id, name, score);
    }
    for (int i=4; i<8; i++) {
	cin >> id >> name >> count;
	new Teacher(id, name, count);
    }
    cout << "优秀的学生和教师名单:" << endl;
    Person* p = Person::head;
    while(p) {
	if (p->isGood())
	    p->show();
	p = p->getNext();
    }
    return 0;
}
