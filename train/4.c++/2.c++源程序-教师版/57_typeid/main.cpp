#include <iostream>
#include <typeinfo>
#include <vector>
#include <map>
using namespace std;

struct Stu
{
	int id;
	string name;
};
int main()
{
	int a = 9;
	const int* pi = &a;
	int* const pj = &a;
	
	const short s = 3;
	short s2;
	long l = 32;
	float f = 3.2f;
	double d = 21.2;
	char c = 's';
	int arr[4] = {3, 5, 3, 1};
	double arr2[4];
	string str = "fd";
	Stu stu;
	int** p;
	int& r = a;
	//typeid 只能识别底层const, 不能识别顶层const 和 引用 
	//const char*  
	auto name = typeid(a).name();
//	auto n = typeid(a);
//	cout << typeid(n).name() << endl;

//	"i"	"s" "PKi"
	if (typeid(s) == typeid(short))//比较两个类型是否相同
		cout << "s == short" << endl;
	else 
		cout << "s != s2" << endl;
	cout << typeid(name).name() << endl;
	cout << typeid(pi).name() << endl;
	cout << typeid(pj).name() << endl;
	//a 的类型名字
	cout << typeid(a).name() << endl;
	cout << typeid(s).name() << endl;
	cout << typeid(l).name() << endl;
	cout << typeid(f).name() << endl;
	cout << typeid(d).name() << endl;
	cout << typeid(c).name() << endl;
	cout << typeid(arr).name() << endl;
	cout << typeid(arr2).name() << endl;
	cout << typeid(str).name() << endl;
	cout << typeid(stu).name() << endl;
	cout << typeid(p).name() << endl;
	cout << typeid(r).name() << endl;
	vector<int> v;
	vector<string> v2;
	map<int, string> m;
	cout << typeid(v).name() << endl;
	cout << typeid(v2).name() << endl;
	cout << typeid(m).name() << endl;
	return 0;
}
