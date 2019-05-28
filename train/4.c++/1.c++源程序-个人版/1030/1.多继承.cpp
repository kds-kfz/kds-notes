#include<iostream>
//1.cpp
using namespace std;
//多继承

class B1{
    protected:
    int b1;
    int d;
    public:
    B1() : b1(0),d(0){}
    B1(int a,int b) : b1(a),d(b){}
    void show(){
    cout<<"B1::show"<<endl;
    }
};
class B2{
    protected:
    int b2;
    int d;
    public:
    B2() : b2(0),d(0){}
    B2(int a,int b) : b2(a),d(b){}
    void show(){
    cout<<"B2::show"<<endl;
    }
};

class C : public B1,public B2{
    int d;
    protected:
    public:
    C(int a,int b,int c,int d,int e) : B1(a,b),B2(c,d),d(e){
	cout<<"D"<<endl;
    }
    void show(){
	cout<<b1<<" "<<b2<<" "<<" "<<d<<endl;
	cout<<B1::d<<" "<<B2::d<<endl;
    }
};

/**********************
    B1 b1		B2 b2
    ---------------------------
    b1			b1
    d			d   -->B1
			========
			b2
			d   -->B2
			========
			d   -->C
  **********************/

int main(){
    B1 b1;
    B2 b2;
    C c1(1,2,3,4,5);
    c1.show();
    cout<<"b1_size"<<sizeof(b1)<<endl;
    cout<<"b2_size"<<sizeof(b2)<<endl;
    cout<<"c1_size"<<sizeof(c1)<<endl;
    return 0;
}
