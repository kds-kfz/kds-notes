#include <iostream>
using namespace std;

/******************************************************************************************/
/* 类适配器:                                                                              */
/* 有的时候，你会发现，不是很容易去构造一个Adaptee类型的对象                              */
/* 当 Adaptee 中添加新的抽象方法时，Adapter 类不需要做任何调整，也能正确的进行动作        */
/* 可以使用多肽的方式在 Adapter 类中调用 Adaptee 类子类的方法                             */
/******************************************************************************************/
/* 类适配器特点:                                                                          */
/* 由于 Adapter 直接继承自 Adaptee 类，所以，在 Adapter 类中                              */
/* 可以对 Adaptee 类的方法进行重定义                                                      */
/* 如果在 Adaptee 中添加了一个抽象方法，那么 Adapter 也要进行相应的改动，这样就带来高耦合 */
/* 如果 Adaptee 还有其它子类，而在 Adapter 中想调用 Adaptee 其它子类的方法时              */
/* 使用类适配器是无法做到的                                                               */
/******************************************************************************************/

// Targets
class Target
{
    public:
        virtual void Request()    // Methods  
        {
            cout << "Target::Request" << endl;
        }
        virtual ~Target(){}
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
            /*~~ 这里不能这样，段错误，虚函数调虚函数 ~~*/
            //this->Request();

            this->SpecificRequest();
        }
        ~Adapter(){}
};

// Client
int main(int argc, char *argv[])
{
    cout << "~~ 2.类适配器 ~~" <<endl;
    
    Adapter *d = new Adapter();
    Target *targetObj = d;
    targetObj->Request();

    delete targetObj;
    targetObj = NULL;

    return 0;
}

