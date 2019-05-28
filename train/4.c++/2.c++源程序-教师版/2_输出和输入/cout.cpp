
#include <iostream>
//using namespace std;
using std::cout;
using std::endl;
int main()
{
	//std	标准库的命名空间
	//cout	输出对象
	//<<	输出运算符 二元运算符 优先级
	std::cout << "hello world\n";
	std::cout << 12 << "\n";
	//std::endl 输出一个\n;
	std::cout << 2+3 << std::endl << std::endl;
	int i = 1;
	int j = (1, 2, 5);
	std::cout << (2,3) << std::endl;
	int a = 1;
	//左移运算符
	a = a << 1;
	std::cout << a << std::endl;

	return 0;
}
