#include <iostream>
using namespace std;

/*
/// 对象适配器 ///
1、有的时候，你会发现，不是很容易去构造一个Adaptee类型的对象
2、当Adaptee中添加新的抽象方法时，Adapter类不需要做任何调整，也能正确的进行动作
3、可以使用多肽的方式在Adapter类中调用Adaptee类子类的方法
*/

class Target
{
    public:
        Target(){}
        virtual ~Target(){}
        virtual void Request()
        {
            cout << "Target::Request" << endl;
        }
};

class Adaptee
{
    public:
        void SpecificRequest()
        {
            cout << "Adaptee::SpecificRequest" << endl;
        }
};

class Adapter : public Target
{
    public:
        Adapter() : m_Adaptee(new Adaptee) {}

        ~Adapter()
        {
            if (m_Adaptee != NULL)
            {
                delete m_Adaptee;
                m_Adaptee = NULL;
            }
        }

        void Request()
        {
            m_Adaptee->SpecificRequest();
        }

    private:
        Adaptee *m_Adaptee;
};

int main(int argc, char *argv[])
{
    Target *targetObj = new Adapter();
    targetObj->Request();

    delete targetObj;
    targetObj = NULL;

    return 0;
}

