#include<iostream>
using namespace std;
class C{
    void player(const B &pl){}
};
class A{
    public:
    virtual void shoe(){}
};
class B :public A{
    public:
    void show(){}
};
int main(){

    return 0;
}
