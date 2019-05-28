#include <iostream>
#include <set>
#include <algorithm>
//集合
using namespace std;
/*
	交集
	set_intersection(s1.begin(), s1.end(), 
		s2.begin(), s2.end(), inserter(s4, s4.begin()))
	并集
	set_union()
	差集
	set_difference()
	对称差集
	set_symmetric_difference()
*/
int main()
{
	//有序，唯一
	set<int> s1 = {1, 2, 3, 4};
	set<int> s2(s1.begin(), s1.end());
	set<int> s3 = {3, 4, 5, 6};
	set<int> s4;
	//求交集
	set_intersection(s1.begin(), s1.end(), s3.begin(), 
			s3.end(), inserter(s4, s4.begin()));
/*
	cout << s1.size() << endl;
	for (auto m : s1) 
		cout << m << ", ";
	cout << endl;
	for (auto it = s2.begin(); it != s2.end(); it++)
		cout << *it << ". ";
	cout << endl;
	
	s1.insert(21);
	s1.erase(8);
	auto it = s1.find(8);
	if (it != s1.end())
		cout << "it " << *it << endl;
	else
	cout << "没有找到" << endl;
*/	
	for (auto m : s4) 
		cout << m << ", ";
	cout << endl;
	return 0;
}
