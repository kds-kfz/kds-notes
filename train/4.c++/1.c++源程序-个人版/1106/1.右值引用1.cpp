#include<iostream>
//1.右值引用1.cpp
using namespace std;
#if 1
//c++11，右值引用，只能引用右值    
int sum(int &&a,int &&b){
    cout<<"&&"<<endl;
    return a+b;
}
#endif
#if 0
//既可以右值引用，也可以左值引用
int sum(const int &a,const int &b){
    cout<<"const &"<<endl;
    return a+b;
}
#endif
class Value{
    int n;
    public:
    Value(int i) : n(i){
	cout<<"Value"<<endl;
    }
    Value(const Value &v) : n(v.n){
	cout<<"copy Value"<<endl;
    }
    ~Value(){
	cout<<"~Value "<<n<<endl;
    }
    /*
    Value &Getself(){
	return *this;
    }
    */
    Value &Getself(){
	return *this;
    }
    Value GetT(){
	cout<<"GetT"<<endl;
	Value t(78);
	return t;//==return Value(78);//返回局部对象
    }
    Value &operator=(const Value &v){
	n=v.n;
	cout<<"operator = "<<endl;
	return *this;
    }
};
int main(){
#if 0
    int a=10;
    int b=20;
    int &c=a;
    c=b;
    //右值引用情况：常量（纯右值），匿名变量（将亡值）：不能取地址
    //右值引用
    //产生匿名变量

    //右值引用
    const int &d1=100;
    cout<<&d1<<" "<<d1<<endl;

    //右值引用
    int &&d2=12;
    cout<<&d2<<" "<<d2<<endl;
    d2=55;
    //延长右值的生存期，跟左值一样
    cout<<&d2<<" "<<d2<<endl;

    int num=sum(a,b);//错误，类型不匹配
    cout<<"num = "<<num<<endl;

    //左值引用，错误
//    int &num=sum(2,4);
    //右值引用，延长右值生存期,跟左值一样长 
    int &&num1=sum(2,4);
    cout<<"num1 = "<<num1<<endl;
#endif
#if 1
    Value v1(33);
    /*
    Value v2(66);
    v2=v1.Getself();
    v2=v1.GetT();
    //内部构造一个t，返回值是t的内容
    //拷贝构造一个匿名对象，接收类t的内容，t销毁
    //匿名对象把值赋给v2，匿名对象销毁
    */
    //匿名对象生存期很短，直接销毁，需要另外一个对象来接收它的值
    //Value rv2=v1.GetT();
    //延长了匿名对象的生存期，离开作用于后才销毁
    Value &&rv2=v1.GetT();
    cout<<"end"<<endl;
#endif
    return 0;
}
