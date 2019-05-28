#include<iostream>
//2.cpp
using namespace std;
//多继承, 多态 

class B1{
    protected:
    int b1;
    int d;
    public:
    B1() : b1(0),d(0){}
    B1(int a,int b) : b1(a),d(b){}
    virtual void show(){
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
    virtual void show(){
    cout<<"B2::show"<<endl;
    }
};

class C : public B1,public B2{
    int d;
    protected:
    public:
    //初始化的顺序是继承的顺序和变量定义的顺序
    C(int a,int b,int c,int d,int e) : B1(a,b),B2(c,d),d(e){
	cout<<"D"<<endl;
    }
    void show(){
	cout<<b1<<" "<<b2<<" "<<" "<<d<<endl;
	cout<<B1::d<<" "<<B2::d<<endl;
    }
    //派生增加的虚函数，地址会存在第一个虚函数表后面
    virtual void test(){}
};
#if 0
//B1 *p1=new C
//B2 *p2=new C

/**********************************************
    B1 b1	    B2 b2	    C c1
    ----------------------------------------	    派生没有重写show
    (pvtab1)	    (pvtab2)	    (pvtab1)	    -->[B1::show|B1::fun|C:test]
    b1		    b1		    b1
    d		    d		    d   -->B1
				    ========
				    (pvtab2)	    -->[B2::show]
				    b2
				    d   -->B2
				    ========
				    d   -->C
  *********************************************/    派生重写类show
				    (pvtab1)	    -->[C:show|C::fun]
				    b1
				    d	-->B1
				    ========
				    (pvtab2)	    -->[B2::show]
				    b2
				    d	-->B2
				    ========
				    d	-->C
#endif
int main(){
    B1 b1;
    B2 b2;
    C c1(1,2,3,4,5);
    c1.show();
    cout<<"b1_size = "<<sizeof(b1)<<endl;
    cout<<"b2_size = "<<sizeof(b2)<<endl;
    cout<<"c1_size = "<<sizeof(c1)<<endl;
    return 0;
}
