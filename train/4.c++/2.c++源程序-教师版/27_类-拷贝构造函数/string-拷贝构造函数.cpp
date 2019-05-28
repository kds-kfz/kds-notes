#include <iostream>
#include <cstring>
using namespace std;
class String
{
	char* ptr;
	int len;
public:
	String() {
	//	ptr = nullptr;
	//	len = 0;
		ptr = new char[1]{'\0'};
		len = 0;
	}
	String(const char* s) {
		cout << "const char* "<< endl;
		len = strlen(s);
		ptr = new char[len+1];
		memcpy(ptr, s, len+1);
		//strcpy(ptr, s);
	}
	String(char c, int n) {
		len = n;
		ptr = new char[n+1]{};
		memset(ptr, c, n);
	//	ptr[len] = '\0';
	}
//	String s4(s3);
			// String s(s3);
	//const String& 拷贝构造函数的原型, 
	//1，引用可以改变实参，不应该改变实参的内容
	//2，const & 可以引用右值(匿名变量)
	String(const String& s) {
		cout << "copy " << endl;
		len = s.len;
		ptr = new char[s.len+1]{};
		memcpy(ptr, s.ptr, len);
	}

	int size() {
		return len;
	}
	bool empty() {
		return len == 0;
	}
	const char* c_str() {
		return ptr;
	}
	void resize(int n) {
		char* p = new char[n+1]{};
		memcpy(p, ptr, n);
		//p[n] = '\0';
		delete [] ptr;
		ptr = p;
		len = n;
	}
	// ptr = "hello" 
	//        hlo  
	//	p  = "000"
	//				1,  2
	void erase(int pos, int n) {
		if (pos < 0 || pos >= len) {
			cout << "超出范围" << endl;
			abort();
		} 
		if (n < 0) {
			cout << "长度小于0" << endl;
			abort();
		}
		n = n > len - pos ? len - pos : n;
		
		char* p = new char[len-n+1]{};
		memcpy(p, ptr, pos);
		memcpy(p+pos, ptr+pos+n, len-pos-n);
		delete [] ptr;
		ptr = p;
		len = len - n;
		//replace(pos, n, "");
	}
	void insert(int pos, const char* s) {
		replace(pos, 0, s);
	}
	/*
	void insert(int pos, String s) {
	}
	*//*
			  pos+n
	           | |
	// ptr = "hello",  s = "world"
			   --
	//               1,     2
	// ptr = "hworldlo";
			   -----
*/
	void replace(int pos, int n, const char* s) {
		if (pos < 0 || pos >= len) {
			cout << "超出范围" << endl;
			abort();
		} 
		if (n < 0) {
			cout << "长度小于0" << endl;
			abort();
		}
		n = n > len - pos ? len - pos : n;
		int sl = strlen(s);
		char* p = new char[len+sl-n+1]{};
		memcpy(p, ptr, pos);
		memcpy(p+pos, s, sl);
		memcpy(p+pos+sl, ptr+pos+n, len-pos-n);
		len = sl + len - n;
		delete [] ptr;
		ptr = p;
	}
	/*
	String substr(int pos, int n) {
		
	}
	*/

	void show() {
		cout << (void*)ptr << " ";
		for (int i=0; i<len; i++)
			cout << ptr[i];
		cout << endl;
	}
	//String s(s2);
	String show(String s) {
		s.show();
		String t("banana");
		return t;
	}
	void free() {
		cout << "delete " << (void*)ptr << endl;
		delete [] ptr;
	}
};
int main()
{
//	String s1;
//	String s2("hello");
	String s3('s', 4);
	//同类型参数的构造函数，叫做拷贝构造函数
	//如果没有手动写拷贝构造函数，会合成一个默认的
	String s4(s3); //String(String)
	
	//编译时加 -fno-elide-constructors 关闭返回值优化
	String s5 = s4.show(s3);
	    // s5 = 匿名对象 = s4.show(s3);
		// s5 = "hello";
		// s5 = String("hello");
	//默认的拷贝构造只是简单的变量赋值
/*
	//浅拷贝 ,　只是指针的简单赋值
	//s4.ptr = s3.ptr; s4.len = s3.len;
	//浅拷贝:
	int* p1 = new int(4);
	int* p2 = p1;
	*p1 = 10;
	cout << *p1 << " " << *p2 << endl;
*/	
	/*
	通过同类型的对象，构造本对象的时候，调用拷贝构造函数
	1, 定义一个对象 
	2, 函数传参, 传的类型是非引用类型
	3, 函数返回值, 返回类型是非引用类型
	*/
	
//	delete p1;
//	delete p2;
/*	
	//深拷贝, 内容相同，每个指针都有自己的空间
	int* p3 = new int(4);
	int* p4 = new int(*p3);
	*p3 = 10;
	cout << *p3 << " " << *p4 << endl;
	delete p3;
	delete p4;
*/
//	s1.show();
//	s2.show();
	s3.show();
	s4.show();

//	s2.replace(-1, 10, "world");
//	s2.insert(1, "apple");
//	s2.show();
/*	s2 = s3;//s2.ptr = s3.ptr, s2.len = s3.len;
	cout << s2.size() << endl;
	s2.resize(10);
	cout << s2.size() << endl;
	s2.show();
	if (s1.empty())
		cout << "是空" << endl;
	else 
		cout << "不是空" << endl;
	const char* p = s3.c_str();
	cout << p << endl;
*/
	s3.free();
	s4.free();
//	s1.show();
//	cout << "s2 ";
//	s2.show();
//	cout << "s3 ";
//	s3.show();
	
	/*
	string s;
	string s2('a', 10);
//	string s3(s2);	先不写, 需要拷贝构造函数
	string s1("hello");
	cout << &s1 << endl;
	cout << (void*)&s1[0] << endl;
	s1 = "ssssssssssssssssssssssssssssssssssssssss";
	cout << (void*)&s1[0] << endl;
*/
	return 0;
}
