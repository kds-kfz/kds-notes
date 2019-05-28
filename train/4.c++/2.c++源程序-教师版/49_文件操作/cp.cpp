#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

int main(int argc, char* argv[])
{
	if (argc != 3) {
		cout << "参数错误" << endl;
		return 0;
	}
	if (strcmp(argv[2], ".") == 0) {
		return 0;
	}
	if (strcmp(argv[1], argv[2]) == 0) {
		cout << "目标文件和源文件名字相同" << endl;
		return 0;
	}
	fstream f1(argv[1], ios::in);
	if (!f1.is_open()) {
		cout << "没有这个文件: " << argv[1] << endl;
		return 0;
	}
	fstream f2(argv[2], ios::out);
	
	char buf[1024];
	int len = 0;
	while(1) {
		f1.read(buf, 1024); 
		len = f1.gcount();
		f2.write(buf, len);
		if (len < 1024)
			break;
	}
	

	
	return 0;
}
