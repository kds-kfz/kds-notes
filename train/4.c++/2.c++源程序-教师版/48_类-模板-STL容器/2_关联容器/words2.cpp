#include <iostream>
#include <map>
using namespace std;

int main()
{
	string s;
	map<string, int> m;
						 //EOF
	cout << "请输入单词 (Ctrl+D结束)" << endl;
	while(cin >> s) {
	//	cout << s << endl;
	//	if (m.find(s) == m.end())
	//		m.insert({s, 1});
	//	else
			m[s]++;
	}
	for (auto i : m) {
		cout << i.first << " " << i.second << endl;
	}
	return 0;
}
