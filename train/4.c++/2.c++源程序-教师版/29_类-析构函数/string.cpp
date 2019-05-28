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
					//	"he\0llo"
	void insert(int pos, const String& s) {
		replace(pos, 0, s);
	}
	/*
			  pos+n
	           | |
	// ptr = "hello",  s = "world"
			   --
	//               1,     2
	// ptr = "hworldlo";
			   -----
*/

	void replace(int pos, int n, const String& s) {
		if (pos < 0 || pos >= len) {
			cout << "超出范围" << endl;
			abort();
		} 
		if (n < 0) {
			cout << "长度小于0" << endl;
			abort();
		}
		n = n > len - pos ? len - pos : n;
		int sl = s.len; //String类型的长度不能用strlen测量
		char* p = new char[len+sl-n+1]{};
		memcpy(p, ptr, pos);
		memcpy(p+pos, s.ptr, sl);
		memcpy(p+pos+sl, ptr+pos+n, len-pos-n);
		len = sl + len - n;
		delete [] ptr;
		ptr = p;
	}
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
	String substr(int pos, int n) {
		//	ptr[pos] ~ ptr[pos+n-1]
		if (pos < 0 || pos >= len) {
			cout << "超出范围" << endl;
			abort();
		} 
		if (n < 0) {
			cout << "长度小于0" << endl;
			abort();
		}
		n = n > len - pos ? len - pos : n;
		char* p = new char[n+1];
		memcpy(p, ptr+pos, n);
		String t;
		delete [] t.ptr;
		t.ptr = p;
		t.len = n;
		return t;
	}
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
	//析构函数
	~String() {
		cout << "delete " << ptr <<endl;
		delete [] ptr;
	}
};
int main()
{
	String s3('s', 4);
	String s4("hello"); //String(String)
	s4.insert(1, s3);

	s3.show();
	s4.show();
	String s5 = s4.substr(2, 20);
	s5.show();

	return 0;
}
