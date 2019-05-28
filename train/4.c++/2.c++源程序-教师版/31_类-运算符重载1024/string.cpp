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
//	"a"			"hello"
	String& operator=(const String& s) {
	//	ptr = s.ptr;
		//自拷贝
		if (this == &s)
			return *this;
		delete [] ptr;
		ptr = new char[s.len+1]{};
	//	*ptr = *s.ptr;
		memcpy(ptr, s.ptr, s.len);
		len = s.len;
		return *this;
	}
	String& operator=(const char* s) {
		delete [] ptr;
		len = strlen(s);
		ptr = new char[len+1]{};
		memcpy(ptr, s, len);
		return *this;
	}
	//		   const char*
	//		   const char
	String& operator+=(const String& s) {
		//申请更大空间
		char* p = new char[len+s.len+1]{};
		//向新的空间里拷贝内容
		memcpy(p, ptr, len);
		memcpy(p+len, s.ptr, s.len);
		//释放旧的空间
		delete [] ptr;
		//ptr指向新的空间
		ptr = p;
		//长度改变
		len += s.len;
		return *this;
	}
	String& operator+=(const char* s) {
		int sl = strlen(s);
		char* p = new char[len+sl+1]{};
		memcpy(p, ptr, len);
		memcpy(p+len, s, sl);
		delete [] ptr;
		ptr = p;
		len += sl;
		return *this;
	}
	String& operator+=(char c) {
		char* p = new char[len+2]{};
	//	strcpy(p, ptr);	//拷贝ptr的内容到p里, ptr到\0停止，并且会在p的最后增加一个\0
		memcpy(p, ptr, len);//拷贝len个字符
	//	memcpy(p+len, &c, 1);
		p[len++] = c;
		delete [] ptr;
		ptr = p;
		return *this;
	}
/*
	string a;	a = b; 深拷贝
	string b = "hello";
	//浅拷贝 , 
	memcpy(&a, &b, sizeof(string));
*/
	String operator+(const String& s) {
		String t;
		delete [] t.ptr;
		t.ptr = new char[len+s.len+1]{};
		//把ptr和s.ptr的内容拷贝到新的String里
		memcpy(t.ptr, ptr, len);
		memcpy(t.ptr+len, s.ptr, s.len);
		t.len = len + s.len;
		return t;
	}
//	"hel\0lo" "hel\0zlo"
//	"hello\0"\0
//	"hello\0\0abc"
	bool operator==(const String& s) {
		int n = len < s.len ? len : s.len;
		if (memcmp(ptr, s.ptr, n+1) == 0 )
			return len == s.len;
		else 
			return false;
	}
	bool operator!=(const String& s) {
		int n = len < s.len ? len : s.len;
		return memcmp(ptr, s.ptr, n+1) != 0;
	}
	bool operator>(const String& s) {
		int n = len < s.len ? len : s.len;
		if (memcmp(ptr, s.ptr, n+1) > 0)
			return true;
		else if (memcmp(ptr, s.ptr, n+1) == 0) 
			return len > s.len;
		else
			return false;
	}
	bool operator<(const String& s) {
		int n = len < s.len ? len : s.len;
		if (memcmp(ptr, s.ptr, n+1) < 0)
			return true;
		else if (memcmp(ptr, s.ptr, n+1) == 0)
			return len < s.len;
		else
			return false;
	}
	friend ostream& operator<<(ostream& o, const String& s);
	friend istream& operator>>(istream& in, String& s);
	
	char& operator[](int n) {
		//可以增加判断n的范围的方法
		return *(ptr+n); //ptr[n]
	}
};
//仿造std::string 的用法,　定义一个myString 
构造函数，拷贝，析构，重载运算符 + = += == != < > <= >=
							[] >> <<
relpace, erase, ... 

ostream& operator<<(ostream& o, const String& s) 
{
	for (int i=0; i<s.len; i++)
		o << s.ptr[i];
	return o;
}
istream& operator>>(istream& in, String& s)
{
	char buf[2048];
	in >> buf;
	s = buf;
	return in;
	/*
	int l = strlen(buf);
	delete [] s.ptr;
	s.ptr = new char[l+1]{};
	memcpy(s.ptr, buf, l);
	s.len = l;
*/
}

int main()
{
	std::string s;
	cout << s.max_size() << endl;
	String s3("hel");
	String s4("hel"); //String(String)
//	cin >> s3;
	cout << "s3 = " << s3 << endl;
	s3[2] = 'a';
	cout << "s3 = " << s3 << endl;
	
//	s3.show();
//	s4.show();
//	String s5 = s4.substr(2, 20);
//	s5.show();
//	String& r = s3;
//	s3 = r;
//	s4 = s3;	//调用默认的=
//	s5 = s3;
	s3 += '\0';
	s3 += '\0';
	s3 += "banana";
	s4 += '\0';
//	s4 += "ba";
	s3.show();
	s4.show();
//	s5 = s3 + s4;
//	cout << "s5 : ";
//	s5.show();	
	if (s3 == s4) 
		cout << "s3 == s4" << endl;
	else if (s3 < s4) 
		cout << "s3 < s4" << endl;
	else if (s3 > s4)
		cout << "s3 > s4" << endl;

	return 0;
}
