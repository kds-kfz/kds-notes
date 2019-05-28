#include <iostream>
#include <cstdio>
#include <map>
using namespace std;

int main()
{
	FILE* fp = fopen("words", "r");
	if (fp == NULL) {
		cout << "打开失败" << endl;
		return 0;
	}
	char buf1[1024];
	char buf2[1024];
	map<string, string> m;
	while(fscanf(fp, "%s %s\n", buf1, buf2) == 2) {
		m.insert({buf1, buf2});
	}
	fclose(fp);
	string s;
	map<string, string> ::iterator it;
	while(1) {
		system("clear");
		cout << "请输入要查找的单词(q退出): ";
		cin >> s;
		if (s == "q")
			break;
		it = m.find(s);
		if (it != m.end()) {
			cout << "---------------------" << endl;
			cout << it->first << ": " << it->second << endl;
			cout << "---------------------" << endl;
		}
		else
			cout << "没有找到这个单词" << endl;
		cout << "按enter继续" << endl;
		getchar();
		getchar();
	}
	return 0;
}
