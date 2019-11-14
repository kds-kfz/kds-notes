#include <iostream>
#include <functional>

using namespace std;

/***************************************************************************************/
/* 函数适配器：                                                                        */
/* 可以看作一个通用的函数适配器                                                        */
/* 该函数可以对函数指针、函数对象、lambda表达式和函数所需的参数 ( 任意个数 ) 进行绑定  */
/* 生成一个新的可调用对象来“适应”原对象的参数列表                                      */
/* 绑定后的返回值可以使用std::function保存                                             */
/* function类型与所绑定的函数特征标一致，也可使用auto关键字来保存                      */
/***************************************************************************************/

class AA
{
    public:
        AA(){ }
        ~AA(){ }
        void showAA(int b) {
            cout << m_a << ", " << b << endl;
        }
        int m_a;
};

void show1(AA a, int x, int y) {
    a.m_a = 2;
    cout << a.m_a << ", " << x << ", " << y << endl;
}

void show2(AA &a, int x) {
    a.m_a = 30;
    cout << a.m_a << ", " << x << endl;
}

int main()
{
    cout << "~~ 5.函数适配器 ~~" <<endl;
 
    AA a;
    a.m_a = 66;

    /*~~ bind普通函数 show1，bind 时使用占位符 ~~*/
    /*~~ 可以使用 function 对象，也可以使用 auto ~~*/
    function<void(AA, int, int)> func1 = bind(&show1, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    //auto func1 = bind(&show2, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    func1(a, 6, 6);

    /*~~ bind 普通函数 show2 ~~*/
    /*~~ bind 时直接将非占位符 a 传入，则会进行拷贝构造，传入a的拷贝 ( 注意：是 bind 时拷贝构造 ) ~~*/
    /*~~ 若想将 a 本身进行传参，需加上 ref ~~*/
    auto func2 = bind(&show2, ref(a), std::placeholders::_1);
    func2(20);

    /*~~ bind 成员函数 &AA::showAA ~~*/
    /*~~ 第二位需指定具体的类对象，可以是指针可以是对象实例 ~~*/
    auto func3 = bind(&AA::showAA, a, 8);
    /*~~ 相当于调用a.*showAA(8) ~~*/
    func3();

    return 0;
}

