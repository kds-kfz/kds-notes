#include <iostream>
using namespace std;
int main()
{
	//bool 有两个值 true和false, 
	//				非零和零
	bool ok = true;
	cout << ok << endl;
	ok = false;
	cout << ok << endl;
	//bool类型的大小 :	
	cout << "size = " << sizeof(bool) << endl;

//	if (2 > 3) //比较运算符的返回值都是bool类型
	
	return 0;
}
