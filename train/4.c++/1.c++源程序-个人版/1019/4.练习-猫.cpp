#include<iostream>
//4.cpp
using namespace std;
//练习
class Cat{
    string name;
    int *age;
    public:
    Cat(string n,int a){
	name=n;
	age=new int(a);
	//*age=a;野指针赋值，不明确
    }
    Cat(const Cat &c){
	cout<<"拷贝构造"<<endl;
	name=c.name;
	age=new int(*c.age);
    }
    //explicit 阻止隐型的类型转换
    explicit Cat(const char *c){
	cout<<"显性类型"<<endl;
	name=c;
	age=new int(0);
    }
#if 0
    Cat(const char *c){
	cout<<"隐性类型"<<endl;
	name=c;
	age=new int(0);
    }
#endif
    Cat(){
	cout<<"无参"<<endl;
	age=new int(0);
    }
    void show(){
	cout<<"name :"<<name<<"\n"<<"age :"<<*age<<endl;
    }
    void free(){
	delete []age;
    }
};

int main(){

    Cat a;//调用默认的无参构造
    a.show();
    Cat b("lili",2);
    b.show();
    Cat c(b);//调用默认的拷贝构造，直接简单赋值
    c.show();
#if 1
//    Cat d="mao";//使用里隐性的类型转换a,只有1个参数的情况
    //Cat e=Cat("mao");
    Cat f=Cat("fei");//使用里显性的类型转换
#endif
    a.free();
    b.free();
    c.free();
//    d.free();
//    e.free();
    f.free();
    return 0;
}
