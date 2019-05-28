#include <iostream>
using namespace std;

//c++11	右值引用, 只能引用右值
int sum(int&& a, int&& b)
{
    //	int t;
    //	return t;
    cout << "&& " << endl;
    return a + b;
}
//	可以引用左值，也可以引用右值
/*
   int sum(const int& a, const int& b) {
   cout << "const &" << endl;
   return a + b;
   }
 */
class Value
{
    int n;
    public:
    Value(int i) : n(i) {
	cout << "Value " << endl;
    }
    Value(const Value& v) : n(v.n) { 
	cout << "copy Value " << endl;
    }
    ~Value() {
	cout << "~Value " << n << endl;
    }
    Value& getSelf() {
	return *this;
    }
    Value getT() {
	cout << "get T " << endl;
	Value t(100);
	return t; //return Value(19); //返回局部对象
    }

    Value& operator=(const Value& v) {
	n = v.n;
	cout << "operator = " << endl;
	return *this;
    }

};

int main()
{
    int i = 9;
    int j = 90;
    int& ri = i;
    ri = j;
    //右值　：　常量，　匿名变量　不能直接取地址的
    //			纯右值　将亡值

    //右值引用
    const int& r1 = 100;
    cout << &r1 << " " << r1 << endl;	

    //右值引用
    int&& r2 = 12;
    cout << &r2 << " " << r2 << endl;
    r2 = 29;
    cout << &r2 << " " << r2 << endl;
    //延长右值的生存期，跟左值一样长

//    int num = sum(i, j); //错误, 类型不匹配
    //	int num = sum(12, 32);
//    cout << num << endl;
    Value v1(9);
    //	Value v2(0);

    //v2 = v1.getSelf();
    /*
       v2 = v1.getT(); // 内部构造一个t, 返回值是t的内容,
    //拷贝构造一个匿名对象，接收了t的内容, t销毁,
    //匿名对象把值赋给v2, 匿名对象销毁
     */
    //Value rv2 = v1.getT();//匿名对象生存期很短，直接销毁, 需要另一个对象接收它的值
    Value&& rv2 = v1.getT();//延长了匿名对象的生存期，离开作用域之后才销毁
    cout << "end " << endl;

    return 0;
}
