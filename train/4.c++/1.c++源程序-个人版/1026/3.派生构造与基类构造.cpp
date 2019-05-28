#include<iostream>
//3.cpp
//派生构造与基类构造

using namespace std;

#if 0
练习:
基类：
Pesron{
    protected:
    int id;
    string name;
    int age;
    public:
    void show(){}
};
派生类:
//学生
class Stu{
    int score;
    public:
    void init(){}
    void show(){}
};
//老师
class Ste{
    string subject;
    public:
    void init(){}
    void show(){}
};

#endif
class Person{
    protected:
    int id;
    string name;
    int age;
    public:
    Person(int i,string n,int a) : id(i),name(n),age(a){
	cout<<"有参 Person"<<endl;
    }
    Person(){
	cout<<"无参 Person"<<endl;
    }
    void show(){
	cout<<id<<" "<<name<<" "<<age<<" ";
    }
    ~Person(){
	cout<<"~Person"<<endl;
    }
};

class Stu : public Person{
    int score;
    public:
    
//  Stu(int i,string n,int a,int s) : id(i),name(n),age(a),score(s){}
//  错误，这时基类还没有完成构造
    //Stu有参构造时，先调用基类有参，后执行自身有参构造，
    //注意写法顺序，与构造顺序相同
//    Stu(int i,string n,int a,int s) : score(s),Person(i,n,a){}//正确
    //Stu s1(1001,"lisi",24,0); === score(0).Person(1001,"lisi",24)

//    Stu(int s,int i,string n,int a) : score(s),Person(i,n,a){}//正确
    //Stu s1(0,1001,"lisi",23); === score(0).Person(1001,"lisi",24)

    Stu(int i,string n,int a,int s) : Person(i,n,a),score(s){}//正确
    //Stu s1(1001,"lisi",23,0); === Person(1001,"lisi",23),score(0);

    //执行派生无参之前先调用基类无参构造
    //如果没有基类无参构造则，出现错误
    Stu(){
	cout<<"Stu"<<endl;
    }
    void init(int i,string n,int a,int s){
	id=i;
	name=n;
	age=a;
	score=s;
    }
    void show(){
//	cout<<id<<" "<<name<<" "<<age<<" "<<score<<endl;
	Person::show();
	cout<<score<<endl;
    }
    ~Stu(){
	cout<<"~Stu "<<id<<endl;
    }
};

class Ste : public Person{
    string subject;
    public:
    /*
    Ste(int i,string n,int a,string s) : 
	Person::id(i),Person::name(n),Person::age(a),subject(s){}
    */
    Ste(){
	cout<<"Ste"<<endl;
    }
    Ste(int i,string n,int a,string s) : Person(i,n,a),subject(s){}

    void init(int i,string n,int a,string s){
	Person::id=i;// == id=i;
	Person::name=n;// == name=n;
	Person::age=a;// == age=n;
	subject=s;
    }
    void show(){
//	cout<<Person::id<<" "<<Person::name<<" "<<Person::age<<" "<<subject<<endl;
	Person::show();
	cout<<subject<<endl;
    }
    ~Ste(){
	cout<<"~Ste "<<id<<endl;
    }
};

int main(){
#if 0
    //Person p;
    //情况1
    //调用基类构造和自身构造
    //先调用无参Person，再调用无参Stu
    Stu s1;
    //先调用无参Person，再调用无参Ste
    Ste t1;
    
    s1.init(1001,"lisi",23,0);
    t1.init(2001,"lisi",24,"语文");
    s1.show();
    t1.show();
#endif
#if 1
    Stu s1(1001,"lisi",23,0);
    Ste t1(2001,"lisi",24,"语文");
    s1.show();
    t1.show();
#endif
    return 0;
    //情况1
    //作用域结束，
    //先调用Ste里的析构，再调用Person里的析构
    //后调用Stu里的析构，再调用Person里的析构
}

