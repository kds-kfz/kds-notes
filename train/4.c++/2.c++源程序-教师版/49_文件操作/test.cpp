#include <iostream>
#include <fstream>
#include <string.h>
using namespace std;

int main()
{
	//*************初始化**********//
	fstream f1; //没有绑定任何文件的对象
	fstream f2("file"); //绑定文件file 读写
//	fstream f3("file", ios::in);//以mode方式打开
		//	ios::in | ios::binary  以多种方式打开	
		//mode 如下:
		//ios::in	读
		//ios::out  写, 如果没有文件会创建，如果有文件清空
		//ios::app	以读/写方式打开, 追加
		//ios::binary 二进制
		//ios::trunc  打开文件并清空
		//ios::ate	  打开一个已存在的文件并定位到文件末尾
	//***********打开，关闭***********//
	f1.open("file");
//	f1.open("file", mode);
	
	f1.close();
	f1.is_open();//判断是否打开文件
	//**********单字节IO操作*********//
	/*
	char ch = f2.get();
	cout << "get() " << ch << endl;
	f2.get(ch);
	cout << "get(ch) " << ch << endl;
	//向文件里写一个字符
	f2.put('z');
	
	//----不常用----//
	f2.putback('x'); //把ch放回流里
	f2.unget();		//不获得字符，指针向前移动一个字符
	ch = f2.peek();//把一个字节以整型方式返回，不从流中删除
	cout << "peek " << ch << endl;
	while((ch = f2.get()) != EOF) {
		cout << ch << ", ";
	}
	*/
	//*********多字节IO操作********//
	char buf[1024];
			//  n    ch
	f2.get(buf, 10, '\n');	//读取最多n-1个字符, 或者到ch结束, ch还保存在流中
	f2.getline(buf, 10, '\n');//ch丢弃了
	f2.read(buf, 10);//读n个字节,不考虑\0
	
	int i = f2.gcount();	//返回上次读取的字节个数
	cout << i << endl;
	f2.write(buf, 10);//写n个字符到文件里
	//----不常用----//
			//n   ch
	f2.ignore(10, ' '); //读取并忽略最多n个字符,或者是到ch结束，包括ch
	char ch;	
	//*********流随机访问***********//
	int a = f2.tellg();	//返回输入流的位置
	int b = f2.tellp(); //返回输出流的位置
	//fseek(fp, SEEK_SET); SEEK_CUR SEEK_END
	f2.seekg(12); //直接定位到据开始12的位置
//	f2.seekp(43); //
	cout << f2.tellg() << endl;
	
	f2.seekg(12, ios::beg);
	cout << f2.tellg() << endl;
	f2.seekp(12, ios::end);
			//ios::cur
			//ios::end
	cout << f2.tellg() << endl;
	//实现cp 命令 
	//./Cp file1 file2
	/*
	int main(int argc, char* argv[]) {
		argv[1], argv[2] 	
	}
	*/
	return 0;
}
