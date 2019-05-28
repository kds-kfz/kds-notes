#include<iostream>
//7.cpp
using namespace std;
//不同的枚举名，内部内部成员名字不能一样
//枚举类，成员可以重名，但是使用需要加域名
enum class Color{
    Red=5,
    Blue,
    Yellow
};

enum class Dir{
    Up=10,
    Down,
    Left,
    Right
};

int main(){
    //打印枚举类(变量值)需要加强转
    cout<<(int)Color::Red<<endl;
    Dir d=Dir::Up;
    if(d==Dir::Up)
	cout<<"Up"<<endl;
    return 0;
}
