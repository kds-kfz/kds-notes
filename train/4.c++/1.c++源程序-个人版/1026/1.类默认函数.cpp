#include<iostream>
//1.cpp
//函数默认参数

using namespace std;

class Stu{
    int id;
    string name;
    //拷贝构造是私有的，阻止拷贝构造c++11,之前版本
    //Stu(const Stu &s){}
    public:
    Stu(int i,string n) : id(i),name(n){}
    //Stu(){}//默认的构造函数
    //c++11中，delete使编译器合成默认的构造函数
    Stu()=default;

    //默认的拷贝构造，不写也有默认的
    //Stu(const Stu &s) : id(s.id),name(s.name){}

    //delete使编译器不拷贝构造
    Stu(const Stu &s)=delete;
    
    //默认析构,析构函数不要写成delete函数 
    //如果析构函数是被删除的，对象就不会释放
    //~Stu(){}
    ~Stu()=default;

    //默认的赋值运算符,浅拷贝
    /*
    Stu &operator=(const Stu &s){
	id=s.id;name=s.name;return *this;
    }*/
    Stu &operator=(const Stu &s)=delete;
//    Stu &operator=(const Stu &s)=default;
    
    friend ostream &operator<<(ostream &out,const Stu d);
};

ostream &operator<<(ostream &out,const Stu d){
    out<<d.id<<" "<<d.name;
    return out;
}

int main(){
    Stu s1;
    Stu s2(s1);
    s1=s2;
    cout<<s1<<endl;
    cout<<s2<<endl;
    return 0;
}

