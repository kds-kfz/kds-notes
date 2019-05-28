#include<iostream>
//2.cpp
using namespace std;

//c++类初始化
struct Stu{
    //成员变量，成员对象
    int id;//=0;
    string name;//="lisi";
    int score;
    //成员函数，成员方法
    void show(){
	cout<<id<<" "<<name<<" "<<score<<endl;
    };
#if 1
    //成员函数重载
    void show(Stu d){
	cout<<d.id<<" "<<d.name<<" "<<d.score<<endl;
    };
    void show(int id){
	//打传进来的id，就近打印
	cout<<id<<" "<<name<<" "<<score<<endl;
	//*this == s1
	//this指向本对象的指针，this==&s1，是一个运算符，只能在类内使用，不能在类外使用
	//this = new Stu;//错误，不能改变指向
	//this == Stu *const self 
	cout<<this->id<<" "<<name<<" "<<score<<endl;
    }
    //传入一个结构体指针
    //Stu *self == this
    void show(Stu *self){
	cout<<self->id<<" "<<self->name<<" "<<self->score<<endl;
    };
#endif
};

void show(Stu d){
    cout<<d.id<<" "<<d.name<<" "<<d.score<<endl;
}

int main(){
    //函数存在代码区，不占内存
    cout<<"Stu_size="<<sizeof(Stu)<<endl;//48

    Stu s1={1001,"liwu",90};
    Stu s2={1002,"liwu",90};
    s1.show();
    cout<<endl;
    s1.show(s1);
    cout<<endl;
    s1.show(&s1);
    cout<<endl;
    s2.show(1999);
    cout<<endl;
    show(s1);
    return 0;
}
