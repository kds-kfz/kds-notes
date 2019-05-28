#include<iostream>
//1.模板类.cpp
using namespace std;
//函数模板，存方法
//模板类一般是作为容器，存数据类型
//模板类
template <typename T>
class Value{
    T i;
    public:
    //有局限性，这里的无参构造不确定
//    Value()=delete;
//    Value() : i(0){};
    Value(T n) : i(n){}
    void Setnum(T n){
	i=n;
    }
    T Getnum()const{
	return i;
    }
};
class Integer{
    int i;
    public:
    Integer() : i(0){}
    Integer(int a) : i(a){}
    void Setnum(int n){
	i=n;
    }
    int Getnum() const{
	return i;
    }
    Integer operator+(const Integer &n){
	return Integer(i+n.i);
    }
};
class Double{
    double i;
    public:
    Double() : i(0.0){}
    Double(double d) : i(d){}
    void Setnum(double n){
	i=n;
    }
    double Getnum(){
	return i;
    }
    Double operator+(const Double &n){
	return Double(i+n.i);
    }
};

int main(){
#if 0
    Integer a1;
    cout<<a1.Getnum()<<endl;
    Integer a2(22);
    cout<<a2.Getnum()<<endl;

    Double b1;
    cout<<b1.Getnum()<<endl;
    Double b2(9.8);
    cout<<b2.Getnum()<<endl;
#endif
#if 1
    Value <int>v1(45);
    cout<<v1.Getnum()<<endl;
    Value <string>v2("hello");
    cout<<v2.Getnum()<<endl;
#endif
    return 0;
}
