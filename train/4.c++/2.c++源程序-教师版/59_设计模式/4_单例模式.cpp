#include <iostream>
using namespace std;
/*
   懒汉模式, 没有用到的时候，不会创造对象
   class Base
   {
   static Base* instance;
   int num;
   Base() : num(0) { }
   public:
   Base(const Base& b) = delete;
   Base& operator=(const Base& b) = delete;
   static Base* createInstance() {
//线程不安全
if (instance == nullptr) {
//加锁
instance = new Base;
//解锁
}
return instance;
}
};
Base* Base::instance = nullptr;
 */
class Base
{	
    //饿汉模式：不管使不使用，都会创造一个对象
    static Base* instance;
    int num;
    Base() : num(0) { }
    public:
    Base(const Base& b) = delete;
    Base& operator=(const Base& b) = delete;
    static Base* createInstance() {
	return instance;
    }	
};
Base* Base::instance = new Base;
int main()
{
    Base* b1 = Base::createInstance();
    Base* b2 = Base::createInstance();
    if (b1 == b2)
	cout << "b1 == b2" << endl;
    return 0;
}
