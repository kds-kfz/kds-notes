#include <iostream>
//双向链表
#include <list>
using namespace std;
int main()
{
	//0个结点
	list<int> l0;
	cout << "size = " << l0.size() << endl;
	cout << l0.front() << endl;
	cout << l0.back() << endl;
	l0.front() = 90;
	cout << l0.front() << endl;
	cout << l0.back() << endl;
//	l0.resize(1);
//	cout << l0.front() << endl;
//	cout << l0.back() << endl;
	
	//n个结点，每个都是默认值
	list<int> l1(4);
	//n个结点，都是value
	list<int> l2(4, 10);
	list<int> l3 = {2, 3, 4};
	list<int> l4(l3);
	l2.empty();
	l2.size();
	l2.pop_back();
	l2.pop_front();
	l2.push_front(12);
	l2.push_back(34);
	for (list<int>::iterator it = l2.begin();
			it != l2.end(); it++)
		cout << *it << ", ";
	cout << endl;
	//删除所有值为value的结点
	l2.remove(10);
	//list内的成员方法，从小到大排序
	l2.sort();
	//l2.insert(it, value); 
	//l2.erase(it);
	cout << "front = " << l2.front() << endl;
	//头
	l2.front() = 100;
	//尾
	l2.back() = 200;
	for (auto m : l2)
		cout << m << " ";
	cout << endl;
	return 0;
}
