#include <iostream>
using std::cout;
using std::endl;
int main()
{
	int a, b;
	char arr[20];
	//cin	输入的对象
	//>>	输入运算符
//	cout << "请输入两个数字 " << endl;
//	std::cin >> a >> b;	//scanf("%d%d", &a, &b);
	cout << "请输入一个字符串" << endl;
	std::cin >> arr;	//scanf("%s", arr);
	
	cout << arr << endl;	//printf("%s", arr);
	cout << (void*)arr << endl;//printf("%p", arr);

//	cout << a << " " << b << endl;
	
	return 0;
}
练习：
写一个c++程序，定义一个结构体，
	struct Student { 
		int id;
		char name[20];
	};
	"请输入学生的id 和姓名" 以空格分开
	输出结果 ...


