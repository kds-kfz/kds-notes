#include <iostream>
#include <vector>
using namespace std;

template <typename T1, typename T2>
struct Pair
{
	T1 first;
	T2 second;
	Pair() { }
	Pair(const T1& t1, const T2& t2) : first(t1), second(t2) { }
	bool operator==(const Pair& p) const {
		return first==p.first && second==p.second;
	}
	bool operator<(const Pair& p) const {
		if (first < p.first) 
			return true;
		else if (first==p.first && second < p.second)
			return true;
		return false;
	}	
	bool operator>(const Pair& p) const {
		return !(p < *this || p == *this);
	}
	bool operator<=(const Pair& p) const {
		return *this < p || *this == p;
	}
	//>= != 
};
int main()
{
	Pair<string, int> p1("hello", 123);
	Pair<string, int> p2("hello", 123);
	Pair<int, Pair<int, string> > p3(100, Pair<int, string>(200, "world"));
	cout << p3.first << " " << p3.second.first << " "
		<< p3.second.second << endl;
	Pair<int, string[4] > p4;
	p4.first = 1;
	/*
	p4.second.push_back("hello");
	p4.second.push_back("asd");
	p4.second.push_back("gf");
	p4.second.push_back("gf");
	p4.second.push_back("hgs");
	*/
	p4.second[0] = "fds";
	p4.second[1] = "fdss";
	p4.second[2] = "fdfds";
	p4.second[3] = "fdsgf";
	cout << p4.first << " ";
	for (auto m : p4.second) 
		cout << m << ", ";
	cout << endl;


	cout << p1.first << " " << p1.second << endl;
	cout << p2.first << " " << p2.second << endl;
	if (p1 == p2) {
		cout << "==" << endl;
	} else if (p1 < p2) {
		cout << "<" << endl;
	} else if (p1 > p2) {
		cout << ">" << endl;
	}
};
