#include <iostream>
using namespace std;

int main()
{
	int* p ;
	try {
		//nothrow阻止new抛出异常, 申请空间失败会返回一个空指针
		p = new(nothrow) int[10000000000];
		if (p == nullptr) {
			cout << "申请空间失败" << endl;
			return 0;
		}
		//bad_alloc 是一个标准库异常的类
		//exception & bad
	} catch (bad_alloc & bad) {
		cout << "  what():  " <<  bad.what() << endl;
		cout << "end catch" << endl;
		return 0;
	}
	delete [] p;
	cout << "end main()" << endl;

	return 0;
}
