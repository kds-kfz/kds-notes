#include <iostream>
using namespace std;

/*****************************************************************************************/
/* 对象适配器:                                                                           */
/* 适配器 Adapter 类继承自 Target 类，同时，在 Adapter 类中有一个 Adaptee 类型的成员变量 */
/* Adapter 类重写 Request 函数时，在 Request 中                                          */
/* 使用 Adaptee 类型的成员变量调用 Adaptee 的 SpecificRequest 函数，最终完成适配         */
/*****************************************************************************************/
/* 对象适配器特点:                                                                       */
/* 1、有的时候，你会发现，不是很容易去构造一个 Adaptee 类型的对象                        */
/* 2、当 Adaptee 中添加新的抽象方法时，Adapter 类不需要做任何调整，也能正确的进行动作    */
/* 3、可以使用多肽的方式在 Adapter 类中调用 Adaptee 类子类的方法                         */
/*****************************************************************************************/

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
    cout << "~~ 3.对象适配器 ~~" <<endl;
    
    Target *targetObj = new Adapter();
    targetObj->Request();

    delete targetObj;
    targetObj = NULL;

    return 0;
}

