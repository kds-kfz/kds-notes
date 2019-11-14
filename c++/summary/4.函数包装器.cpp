#include <iostream>
#include <functional>
#include <cstring>

using namespace std;

/***************************************************************************************/
/* 函数包装器：                                                                        */
/* 是在头文件functional中声明的，它从调用特征标的角度定义了一个对象                    */
/* 可用于包装调用特征标相同的函数指针、函数对象或lambda表达式                          */
/***************************************************************************************/

class AA
{
    public:
        AA(){}
        void showAA(int b) {
            cout << b << endl;
        }
        int m_a;
};

double show1(char x, int y) {
    cout << x <<", "<< y << endl;
    return 0.0;
}

void show2(AA a, int x, int y) {
    cout << a.m_a <<", "<< x <<", "<< y << endl;
}

int main()
{
    cout << "~~ 4.函数包装器 ~~" <<endl;
   
    AA aa;
    aa.m_a = 66;

    /*~~ 普通函数 ~~*/
    function<double(char, int)> fc1 = &show1;
    fc1('a', 6);

    /*~~ 参数带类对象的函数 ~~*/
    function<void(AA, int, int)> fc2 = &show2;
    fc2(aa, 6, 6);

    /*~~ 类成员函数，第一个参数为类类型，调用时第一个参数为类对象 ~~*/
    function<void(AA, int)> fun3 = &AA::showAA;
    fun3(aa, 333);

    /*~~ lambda表达式 ~~*/
    /*~~ 因为返回 int 和 double，所以需要显示指出返回值类型为 int 或者 double ~~*/
    /*~~ 若只返回一种类型，lambda 表达式可以自动推断，不需要显示指出 ~~*/
    function<double(const char *)> fc4 = [](const char *str)->double{
        cout << str << endl;
        if (strlen(str)) {
            return static_cast<int>(strlen(str));
        }
        else {
            return 0.1;
        }
    };
    cout << fc4("asdasdasd111") << endl;
    cout << fc4("") << endl;
    return 0;
}
