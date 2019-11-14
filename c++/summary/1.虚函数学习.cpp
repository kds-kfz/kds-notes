#include <iostream>
using namespace std;

class Base1
{
    public:
        int m_base1;
        Base1(int para):m_base1(para){}
        virtual void Add() { cout << "Base1 Virtual Add()!"<< "\n";  }
        virtual void Sub() { cout << "Base1 Virtual Sub()!" << "\n"; }
        virtual void Div() { cout << "Base1 Virtual Div()!" << "\n"; }
};

class Base2
{
    public:
        int m_base2;
        Base2(int para) : m_base2(para){}
        virtual void Mul() { cout << "Base2 Virtual Mul()!" << "\n"; }
        virtual void INC() { cout << "Base2 Virtual INC()!" << "\n"; }
        virtual void DEC() { cout << "Base2 Virtual DEC()!" << "\n"; }
};

class Derive : public Base1, public Base2
{
    public:
        int m_derive;
        Derive(int b1, int b2, int d) : Base1(b1), Base2(b2), m_derive(d){}
        virtual void Sub() { cout << "Derive Virtual Sub()!" << "\n"; }
        virtual void INC() { cout << "Derive Virtual INC()!" << "\n"; }
};

int main()
{
    cout << "~~ 1.虚函数学习 ~~" <<endl;

    Derive* d = new Derive(1, 11, 22);

    /*~~ 此时指针指向的位置不是 Derive 的开头位置，而是 Derive 对象中子区域 Base2 的头部 ~~*/
    Base2* b2 = d;
    /*~~ 此时 b2 只能调用 Base2 的虚函数, INC 已被重写 ~~*/
    b2->INC();
    /*~~ 此时指针指向的位置不是 Derive 的开头位置，而是 Derive 对象中子区域 Base1 的头部 ~~*/
    Base1* b1 = d;
    /*~~ 此时 b1 只能调用 Base1 的虚函数, Sub 已被重写 ~~*/
    b1->Sub();

    return 0;	
}
