#include <iostream>
#include <set>
using namespace std;

enum Color
{
	Zero = 0,
	Red = 31,
	Grean,
	Yellow,
	Blue,
	Purple,
	DarkBle,
	White
};
void CC(Color c) {
	cout << "\033[" << c << "m" << flush;
}
//cat words
//argv[0] == "cat"
//argv[1] == "words"
	//输入了几个命令  
int main(int argc, char* argv[])
{
	if (argc != 2) {
		cout << "参数错误" << endl;
		return 0;
	}
	//cout << argv[0] << " " << argv[1] << endl;
	FILE* fp = fopen(argv[1], "r");
	set<string> s1 = {"int", "double", "char", "FILE", "void", "enum", "namespace", "const", "float"};
	set<string> s2 = {"while", "if", "else", "return", "using", "case", "new", "delete", "defalut"};
	char buf[40];
	string s;
	char c;
	
	while((c = fgetc(fp)) != EOF) {
		if (c != ' ' && c != '\n' && c != '\t')
			s += c;
		else if (c == ' ' || c == '\t' || c == '\n') {
			if (s1.find(s) != s1.end())
				CC(Grean);
			else if (s2.find(s) != s2.end()) 
				CC(Yellow);
			else 
				CC(Zero);
			cout << s << c;
			s.resize(0);
		}
	}
	CC(Zero);
	cout << s << endl;
	fclose(fp);
	return 0;
}
