#include<iostream>
//1.cpp
//类成员函数后加const

using namespace std;

class Integer{
    int *num;
    public:
    //mutable，定义一个变量可变的
    //const，不可变
    //二者只能用一种
    mutable int sum;

    Integer(){
	num=new int(0);
    }
    Integer(int n){
	num=new int(n);
    }
    Integer(const Integer &i){
//	num=new int(*i.Getnum(12));
	num=new int(*i.num);
    }
    ~Integer(){
	delete num;
    }
    //const加在函数前表示返回值是const，返回值不可改变
    //int const Getnum(){}
    //在成员函数后面加const，表示这个函数不能改变类的成员变量
    //num不能改变指向
    //只有成员函数可以加const属性
    int Getnum() const{
	//num=new int(5);//错误，不能换指向
	//sum=12;
	cout<<"const"<<endl;
	return *num;
    }
    int Getnum(){
	cout<<"no const"<<endl;
	return *num;
    }
    void Setnum(int n){
	*num=n;
    }

};

int main(){
#if 1
    Integer a1(9);
    cout<<a1.Getnum()<<endl;
    a1.Setnum(44);
    cout<<a1.Getnum()<<endl;

    const Integer a2(90);
    //a2是不可改变的
    //a2.Setnum(77);//错误
    //a2的函数有可能改变它的内容
    //所以我们的函数不能改变a2的成员变量
    cout<<a2.Getnum()<<endl;
#endif
#if 0
    Integer a3(55);
    cout<<a3.Getnum()<<endl;
    a3.sum=123;
    cout<<a3.sum<<endl;
#endif
    return 0;
}
