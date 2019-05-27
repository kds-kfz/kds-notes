#include<iostream>

using namespace std;

class Base1{
public:
    Base1(int num) : num1(num){}
    virtual void foo1(){ cout<<"Base1::foo1 "<<num1<<endl; }
    virtual void foo2(){ cout<<"Base1::foo2 "<<num1<<endl; }
    virtual void foo3(){ cout<<"Base1::foo3 "<<num1<<endl; }

private:
    int num1;
};

class Base2{
public:
    Base2(int num) : num2(num){}
    virtual void foo1(){ cout<<"Base2::foo1 "<<num2<<endl; }
    virtual void foo2(){ cout<<"Base2::foo2 "<<num2<<endl; }
    virtual void foo3(){ cout<<"Base2::foo3 "<<num2<<endl; }

private:
    int num2;
};

class Base3{
public:
    Base3(int num) : num3(num){}
    virtual void foo1(){ cout<<"Base3::foo1 "<<num3<<endl; }
    virtual void foo2(){ cout<<"Base3::foo2 "<<num3<<endl; }
    virtual void foo3(){ cout<<"Base3::foo3 "<<num3<<endl; }

private:
    int num3;
};

class D1 : public Base1{
public:
    D1(int num) : Base1(num){}
    virtual void faa1(){ cout<<"D1::faa1 "<<endl; }  //无覆盖
    virtual void faa2(){ cout<<"D1::faa2 "<<endl; }  //无覆盖
};

class D2 : public Base1{
public:
    D2(int num) : Base1(num){}
    virtual void foo2(){ cout<<"D2::foo2 "<<endl; }  //覆盖 Base1::foo2
    virtual void faa2(){ cout<<"D2::faa2 "<<endl; }  //无覆盖
    virtual void faa3(){ cout<<"D2::faa3 "<<endl; }  //无覆盖
};

class D3 : public Base1, public Base2, public Base3{ //多继承，无覆盖
public:
    D3(int num1, int num2, int num3) : Base1(num1), Base2(num2), Base3(num3){}
    virtual void fcc1(){ cout<<"D3::fcc1 "<<endl; }  //无覆盖
    virtual void fcc2(){ cout<<"D3::fcc2 "<<endl; }  //无覆盖
    virtual void fcc3(){ cout<<"D3::fcc3 "<<endl; }  //无覆盖
};

class D4 : public Base1, public Base2, public Base3{ //多继承，有覆盖
public:
    D4(int num1, int num2, int num3) : Base1(num1), Base2(num2), Base3(num3){}
    virtual void foo1(){ cout<<"D4::foo1 "<<endl; }  //覆盖 Base1::foo1, Base2::foo1, Base3::foo1, 
    virtual void fdd2(){ cout<<"D4::fdd2 "<<endl; }  //无覆盖
};

#if 0
    1. 一般继承(无虚函数覆盖) ---
    vfptr --> Base1::foo1()
    (D1)      Base1::foo2()
              Base1::foo3()
              NULL
    2. 一般继承(有虚函数覆盖) ---
    vfptr --> Base1::foo1()
    (D2)      Base2::foo2()
              Base1::foo3()
              NULL
    3. 多重继承(无虚函数覆盖) ---
    vfptr --> Base1::foo1()
    (D1)      Base1::foo2()
              Base1::foo3()
              NULL
    vfptr --> Base2::foo1()
    (D2)      Base2::foo2()
              Base2::foo3()
              NULL
    vfptr --> Base3::foo1()
    (D3)      Base3::foo2()
              Base3::foo3()
              NULL
    4. 多重继承(有虚函数覆盖) ---
    vfptr --> D4::foo1()
    (D1)      Base1::foo2()
              Base1::foo3()
              D4::fdd2()
              NULL
    vfptr --> D4::foo1()
    (D2)      Base2::foo2()
              Base2::foo3()
              NULL
    vfptr --> D4::foo1()
    (D3)      Base3::foo2()
              Base3::foo3()
              NULL
#endif

typedef void (*Fun)(void);

int main(){
    Base1 *b1 = NULL;
    Base2 *b2 = NULL;
    Base3 *b3 = NULL;

    cout<<"一般继承自Base1，无覆盖 ----"<<endl;
    D1 d1(1);
    b1 = &d1;
    b1->foo1();//Base1::foo1 1

    cout<<"一般继承自Base1，覆盖foo2 ----"<<endl;
    D2 d2(2);
    b1 = &d2;
    b1->foo2();//D2::foo2

    cout<<"多重继承，无覆盖 ----"<<endl;
    D3 d3(1, 2, 3);
    b1 = &d3;
    b2 = &d3;
    b3 = &d3;
    b1->foo1();//Base1::foo1 1
    b2->foo1();//Base2::foo1 2
    b3->foo1();//Base3::foo1 3
    
    cout<<"多重继承，覆盖foo1 ----"<<endl;
    D4 d4(1, 2, 3);
    b1 = &d4;
    b2 = &d4;
    b3 = &d4;
    b1->foo1();//D4::foo1
    b2->foo1();//D4::foo1
    b3->foo1();//D4::foo1

    Base1 b(10);
    Fun pfun = (Fun) * ((int *) *(int *)(&b) );
    pfun();

    return 0;
}
