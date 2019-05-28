#include<iostream>
//5.cpp
//虚析构
//通过基类指针释放派生
//2017/10/27
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
    //虚函数
    virtual void show(){
	cout<<id<<" "<<name<<" "<<age<<endl;
    }
    //使用基类指针释放派生对象，基类的析构函数一定要定义成虚析构函数
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
	cout<<id<<" "<<name<<" "<<age<<" "<<score<<endl;
    }
    ~Stu(){
	cout<<"~Stu "<<id<<endl;
    }
};

class Ste : public Person{
    string subject;
    public:
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
    //如果基类有virtual void show()
    //派生里的void show()被重写的show()函数
    void show(){
	cout<<Person::id<<" "<<Person::name<<" "<<Person::age<<" "<<subject<<endl;
    }
    ~Ste(){
	cout<<"~Ste "<<id<<endl;
    }
};
/*************************
  Person p		Ste t
  ----------------------------
  id			id
  name			name
  age			age
  ----------------------------
			subject
  *************************/
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
#if 0
    Person *q=new Person(9999,"person",99);
    Stu *q1=new Stu(1001,"lisi",23,0);
    Ste *q2=new Ste(2001,"lisi",24,"语文");

    q=q1;//合法
    //小的指针指向大的空间是合法的，基类指针可以指向派生对象
    //基类指针可以指向派生对象

//    q2=q;//非法
    //大的指针指向小的空间，会访问不到合法位置
    //派生指针不能指向基类对象
    q->show();//原来基类内容被派生类替换
    q1->show();
    q2->show();
#endif
#if 1
    Person *q=new Person(9999,"person",99);

    Person p(9999,"person",99);
    Ste t1(2001,"lisi",24,"语文");
    Stu s1(1001,"lisi",23,0);

    q->show();//指向指向派生
    delete q;

    q=new Stu(1002,"lisi",23,0);
    q->show();
    delete q;
    //多态：当派生类里有重写的函数时，基类指针q指向任意的派生对象
    //q就能访问哪个对象的函数

    //基类指针不能访问派生的成员
    //cout<<p->score<<endl;//错误

    //多态只能通过指针和引用实现
    Person &r=t1;
    r.show();

    //基类指针指向派生,访问派生函数
    q=&t1;
    q->show();
    
    //赋值，派生给基类赋值,基类自身值改变,不是多态
    p=t1;
    p.show();

    //如果基类show前没有virtual，
    //即使换了指向(基类指针指向派生)
    //还是只能访问本身的show

    //如果基类show前有virtual,但派生类没有show，
    //即使换了指向(基类指针指向派生)
    //也是只能访问本身的show
#endif
    return 0;
    //情况1
    //作用域结束，
    //先调用Ste里的析构，再调用Person里的析构
    //后调用Stu里的析构，再调用Person里的析构
}

