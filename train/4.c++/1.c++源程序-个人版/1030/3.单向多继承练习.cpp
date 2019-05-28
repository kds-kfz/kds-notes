#include<iostream>
//3.cpp
using namespace std;
#if 0
//单向多继承练习:
Person		Teacher
name		school
age		subject
    \		/
	教授
	professor
	title 职称

定义一个教授对象，并初始化，输出
#endif

class Person{
    protected:
    string name;
    int age;
    public:
    Person() : name("--"),age(0){}
    Person(string n,int a) : name(n),age(a){}
    virtual void show(){
	cout<<name<<" "<<age<<endl;
    }
};

class Teacher{
    protected:
    string school;
    string subject;
    public:
    Teacher() : Teacher("--","--"){}
    Teacher(string s1,string s2) : school(s1),subject(s2){}
    virtual void show(){
	cout<<school<<" "<<subject<<endl;
    }
};
class Perofessor : public Person,public Teacher{
    string title;
    public:
    Perofessor() : Perofessor("--",0,"--","--","--"){}
    Perofessor(string n,int a,string s1,string s2,string t) : 
	Person(n,a),Teacher(s1,s2),title(t){}
    void show(){
	cout<<name<<" "<<age<<" "<<school<<" "<<subject<<" "<<title<<endl;
    }
};

int main(){
    Person p1("apple",22);
    Teacher p2("地高","数学");
    Perofessor p3("oppo",23,"北大","地理","教授");

    Person *p;
//    p=&p2;//错误,两个基类并无关系
    p=&p1;
    p->show();
    p=&p3;
    p->show();
    return 0;
}
