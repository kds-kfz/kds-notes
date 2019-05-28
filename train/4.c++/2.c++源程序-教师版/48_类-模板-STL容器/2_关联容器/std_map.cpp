#include <iostream>
#include <map>
//映射
using namespace std;

int main()
{
	pair<int, string> p;
	//key是唯一的，有序
	//  key	 value
	map<int, string> m;
	//make_pair()函数是制作一个pair
	m.insert(make_pair(1003, "Cindy"));
	m.insert({1002, "Mike"});
	m.insert(pair<int, string>(1001, "Tom"));
	//如果key 有相同的，后面的插入会失败
	//pair<iterator, bool>
	auto b = m.insert({1002, "David"});
	cout << b.second << endl;

	bool x = m.erase(1002);
	cout << x << endl;
	
	//m[key] 可以查找key对应的value
	m[1001] = "Bob";
	cout << m[1001] << endl;
	//m[key]如果没有key, m会增加一个pair
	cout << m[2000] << endl;
	//增加一个pair
	m[1003] = "An";
	//m.find(key), 如果找到key，返回相应的位置，如果没有找到，返回 m.end() 位置
	map<int, string> ::iterator i = m.find(1011);
	if (i != m.end()) {
		cout << "找到了 ";
		cout << i->first << " " << i->second << endl;
	} else {
		cout << "没有找到" << endl; 
	}
	//upper_bound(key) 返回大于key的位置
	auto it1 = m.upper_bound(1001);
	//lower_bound(key) 返回不小于key的位置
	auto it2 = m.lower_bound(1001);

	cout << "it1 " << it1->first << " " << it1->second << endl; 
	cout << "it2 " << it2->first << " " << it2->second << endl; 
	//pair<map<>::iterator, map<>::iterator> 返回上面两个位置
	auto it3 = m.equal_range(1001);
	cout << "it3 l " << it3.first->first << " " <<
		it3.first->second << endl;
	cout << "it3 u " << it3.second->first << " " << 
		it3.second->second << endl;
	
	for (auto i : m) {
		cout << i.first << " " << i.second << endl;
	}
	for (auto it = m.begin(); it != m.end(); it++) {
		cout << it->first << "," << it->second << endl;
	}

	return 0;
}
