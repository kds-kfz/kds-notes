#include <iostream>
using namespace std;

/*
/// 类适配器 ///
1、有的时候，你会发现，不是很容易去构造一个Adaptee类型的对象
2、当Adaptee中添加新的抽象方法时，Adapter类不需要做任何调整，也能正确的进行动作
3、可以使用多肽的方式在Adapter类中调用Adaptee类子类的方法
*/

// Targets
class Target
{
    public:
        virtual void Request()    // Methods  
        {
            cout << "Target::Request" << endl;
        }
};

// Adaptee
class Adaptee
{
    public:
        void SpecificRequest()   // Methods 
        {
            cout << "Adaptee::SpecificRequest" << endl;
        }
};

// Adapter
class Adapter : public Target, public Adaptee
{
    public:
        void Request()        // Implements ITarget interface
        {
            // Possibly do some data manipulation  
            // and then call SpecificRequest   
            //this->Request();//这里不能这样，段错误，虚函数调虚函数

            this->SpecificRequest();
        }
};

// Client
int main(int argc, char *argv[])
{
    Adapter *d = new Adapter();
    Target *targetObj = d;
    targetObj->Request();

    delete targetObj;
    targetObj = NULL;

    return 0;
}

