#include <iostream>
#include <vector>
using namespace std;

void print(vector<int>& v)
{
	for (auto m : v)
		cout << m << "=";
	cout << endl;
}

int main()
{
	int a[4];
	//size() 为0
	vector<int> v0;
	//	初始化成n个默认值
	vector<int> v1(-1);
//	cout << v1.size() << endl;
					 //n value
	vector<string> v2(4, "hello");
	//拷贝初始化
	auto v3 = v2;
	//列表初始化
	vector<int> v4{2, 3, 4, 65};
		//		   |			|
		//		  begin()		end()
	vector<int> v5 = {22, 31};
	for (int i=0; i<v5.size(); i++)
		cout << v5[i] << endl;
	
	for (auto m : v4)
		cout << m << " ";
	cout << endl;
	//使用迭代器访问动态数组的成员
	//it相当于一个指针，指向数组的某个位置
	for (vector<int> ::iterator it = v4.begin();
			it != v4.end(); it++)
		cout << *it << ", ";
	cout << endl;
	
	for (auto it = v3.begin(); it != v3.end(); it++)
		cout << *it << "-";
	cout << endl;
	cout << "增加" << endl;
	auto it = v5.begin();
//	it++;
	//插入在it的位置, 需要获得返回值,返回值是新的迭代器位置
	vector<int> ::iterator p;
	//	(iterator pos, value)
	p = it = v5.insert(it, 90);
	it = v5.insert(it, 91);
	it = v5.insert(it, 92);
	it = v5.insert(it, 93);
	it = v5.insert(it, 94);
	cout << &v5[0] << endl;
	p = v5.erase(p);
	p = v5.erase(p);
	p = v5.erase(p);
	cout << v5.size() << endl;
	cout << &v5[0] << endl;
	/*
	[22, 31]
0x100 ^	
	 it	
	 如果it不更新，it指向的还是旧的空间所在的位置
	[90, 22, 31]
0x200
	 ^
	 it
	 */
/*	
	v5.push_back(1);
	v5.push_back(2);
	v5.push_back(3);
	v5.push_back(4);
	v5.push_back(5);
	v5.push_back(6);
	v5.push_back(7);
	v5.push_back(8);
	v5.push_back(9);
	v5.push_back(10);
	v5.push_back(11);
	v5.push_back(12);
	v5.push_back(13);
	v5.push_back(14);
	v5.push_back(15);
	cout << v5.size() << endl;
	print(v5);
	cout << "删除" << endl;
	v5.pop_back();
	v5.pop_back();
	v5.pop_back();
	v5.pop_back();
	v5.pop_back();
	v5.pop_back();
	v5.pop_back();
	v5.pop_back();
	v5.pop_back();
	v5.pop_back();
	v5.pop_back();
	v5.pop_back();
	v5.pop_back();
	v5.pop_back();
	cout << v5[0] << endl;
	cout << v5.size() << endl;
*/
	print(v5);
//	for (int i=0; i<20; i++)//	访问了非法空间
//		cout << v5[i] << endl;
	return 0;
}


