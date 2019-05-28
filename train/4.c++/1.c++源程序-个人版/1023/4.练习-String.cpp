#include<iostream>
#include<cstring>
//4.cpp
using namespace std;

//写个自己的String
class String{
    char *ptr;
    int len;
    public:
    String(){
	ptr=new char[1]{'\0'};
	len=0;
    }
    String(const char *s){
	len=strlen(s);
	ptr=new char[len+1];
	memcpy(ptr,s,len+1);//可以拷贝\0
	//strcpy(ptr,s);//不拷贝\0，结束后自动加\0
    }
#if 0
    //个人版
    String(int n,const char s){
	len=n;
	ptr=new char[len];
	for(int i=0;i<len;i++)
	    ptr[i]=s;
    }
#endif
    String(int n,const char s){
	len=n;
	ptr=new char[n+1]{};
	memset(ptr,s,n);
    }
//    String s4(s3);
    //const String & 拷贝构造函数的原型
    //1.引用可以改变实参，不应该改变实参的内容
    //2.const & 可以引用右值(匿名变量)
    String(const String &s){
	cout<<"copy"<<endl;
	len=s.len;
	ptr=new char[s.len+1]{};
	memcpy(ptr,s.ptr,len);
    }
    int size(){
	return len;
    }
#if 0
    //个人版
    bool empty(){
	if(len)
	    return false;
	return true;
    }
#endif
    //教师版
    //len 为0则返回逻辑1,不等于0返回逻辑0
    bool empty(){
	return len==0;
    }
    const char *c_str(){
	return ptr;
    }
#if 0
    //个人版
    void resize(int n){
	char *news=new char[n];
	memcpy(news,ptr,n);//可以拷贝\0
	delete []ptr;
	ptr=new char[n];
	memcpy(ptr,news,n);//可以拷贝\0
	delete []news;
	len=n;
    }
#endif
    //教师版
    void resize(int n){
	char *news=new char[n+1]{};
	memcpy(news,ptr,n);
	//p[n]='\0';
	delete []ptr;
	ptr=news;
	len=n;
    }
#if 0
    //个人版
    void erase(int pos,int n){
	if(pos>=0&&pos<len){
	int num=pos+n;
	if(n>0)
	for(int i=pos;i<len-n;)
	    ptr[i++]=ptr[num++];
	resize(len-n);
	}
	else
	    abort();
    }
#endif
    //教师版
    void erase(int pos,int n){
	if(pos<0||pos>=len){
	    cout<<"超出范围"<<endl;
	    abort();
	}
	if(n<0){
	    cout<<"长度小于0"<<endl;
	    abort();
	}
	n=n>len-pos?len-pos:n;
	char *p=new char[len-n+1]{};
	memcpy(p,ptr,pos);
	memcpy(p+pos,ptr+pos+n,len-pos-n);
	delete []ptr;
	ptr=p;
	len=len-n;
    }
#if 0
    //个人版
    void insert(int pos,const char *arr){
	if(pos>=0&&pos<len){
	int n=strlen(arr);
	int n1=len;
	char *news=new char[len+n];
	memcpy(news,ptr,len);
	delete []ptr;
	ptr=new char[len+n];
	memcpy(ptr,news,len);
	delete []news;
	len+=n;

	//插入在末尾
	if(pos==n1-1){
	    for(int i=n1,j=0;i<len;i++,j++)
	    ptr[i]=arr[j];
	}
	else{//插入在中间
	    for(int i=len;i>=pos;i--)
		ptr[i]=ptr[n1--];
	    for(int i=pos,j=0;i<pos+n;i++,j++)
		ptr[i]=arr[j];
	}
	}
	else
	    abort();
    }
#endif
    //教师版
    void insert(int pos,const char *s){
	replace(pos,0,s);
    }

    //教师版
    //"hello\0world"
    void insert(int pos,const String &s){
	replace(pos,0,s);
    }
#if 0
    //个人版
    void replace(int pos,int n,const char *arr){
	if(pos>=0&&pos<len){
	    int n=strlen(arr);
	    int n1=strlen(arr);
	    char *news=new char[len-n+n1+1]{};
	    memcpy(news,ptr,pos);
	    memcpy(news+pos,arr,n1);
	    memcpy(news+pos+n1,ptr+pos,len-pos);
	    delete []ptr;
	    ptr=news;
	    len=len-n+n1;
	}
	else
	    abort();
    }
#endif
    //教师版
    void replace(int pos,int n,const char *arr){
	if(pos<0||pos>=len){
	    cout<<"超出范围"<<endl;
	    abort();
	}
	if(n<0){
	    cout<<"长度小于0"<<endl;
	    abort();
	}
	n=n>len-pos?len-pos:n;
	int sl=strlen(arr);
	char *news=new char[len+sl-n+1]{};
	memcpy(news,ptr,pos);
	memcpy(news+pos,arr,sl);
	memcpy(news+pos+sl,ptr+pos+n,len-pos-n);
	len=sl+len-n;
	delete []ptr;
	ptr=news;
    }

    //教师版
    void replace(int pos,int n,const String &arr){
	if(pos<0||pos>=len){
	    cout<<"超出范围"<<endl;
	    abort();
	}
	if(n<0){
	    cout<<"长度小于0"<<endl;
	    abort();
	}
	n=n>len-pos?len-pos:n;
	int sl=arr.len;//String类型长度不能用strlen
	char *news=new char[len+sl-n+1]{};
	memcpy(news,ptr,pos);
	memcpy(news+pos,arr.ptr,sl);
	memcpy(news+pos+sl,ptr+pos+n,len-pos-n);
	len=sl+len-n;
	delete []ptr;
	ptr=news;
    }
#if 0
    //个人版
    char *substr(int pos,int n){
	if(pos>=0&&pos<len){
	    char *news=new char[n+1]{};
	    memcpy(news,ptr+pos,n);
	    return news;
	}
	else
	    abort();
    }
#endif
    //教师版
    String substr(int pos,int n){
	if(pos<0||pos>=len){
	    cout<<"超出范围"<<endl;
	    abort();
	}
	if(n<0){
	    cout<<"长度小于0"<<endl;
	    abort();
	}
	n=n>len-pos?len-pos:n;
#if 0
    //个人版
	String news;
	//需要释放原来的空间，否则news一直占用空间，造成内存泄漏
	delete []news.ptr;
	news.ptr=new char[n+1]{};
	memcpy(news.ptr,ptr+pos,n);
	news.len=n;
	return news;
#endif
    //教师版
	char *p=new char[n+1]{};
	memcpy(p,ptr+pos,n);
	String news;
	//需要释放原来的空间，否则news一直占用空间，造成内存泄漏
	delete []news.ptr;
	news.ptr=p;
	news.len=n;
	return news;
    }
    //String s(s2)
    String show(String s){
	s.show();
	String t("apple");//匿名变量t,函数结束自动销毁，局部变量
	return t;
    }
    void show(){
//	cout<<len<<endl;
	for(int i=0;i<len;i++)
	    cout<<ptr[i];
	cout<<endl;
    }
#if 0
    //个人版
    void free(){
	cout<<"delete :"<<(void *)ptr<<endl;
	delete []ptr;
    }
#endif
    //析构函数释放,需要手动释放手动申请的空间
    ~String(){
//	cout<<"delete :"<<ptr<<endl;
	delete []ptr;
    }
#if 1
//  2017/10/23
    String &operator=(const String &s){
	if(this==&s)//当发生自己赋自己的情况,自拷贝
	    return *this;
	delete []ptr;
	ptr=new char[s.len+1]{};
	memcpy(ptr,s.ptr,s.len);
	len=s.len;
	return *this;
    }
    String operator+(const String &s){
	String news;
	delete []news.ptr;
	news.ptr=new char[len+s.len+1]{};
	memcpy(news.ptr,ptr,len);
	memcpy(news.ptr+len,s.ptr,s.len);
	news.len=len+s.len;
	return news;
    }
    String &operator+=(const String &s){
	/*
	if(this->len==0){
	    delete []ptr;
	    ptr=new char[s.len+1]{};
	    len=s.len;
	    memcpy(ptr,s.ptr,len);
	    return *this;
	}*/
	char *p=new char[len+s.len+1]{};
	memcpy(p,ptr,len);
	memcpy(p+len,s.ptr,s.len);
	/*
	int n=len+s.len;
	delete []ptr;
	ptr=p;
	len=n;
	*/
	delete []ptr;
	len+=s.len;
	ptr=p;
	return *this;
    }
    String &operator+=(const char *s){//const char &s
	int sl=strlen(s);
	char *p=new char[len+sl+1]{};
	memcpy(p,ptr,len);
	memcpy(p+len,s,sl);
	delete []ptr;
	len+=sl;
	ptr=p;
	return *this;

    }
    String &operator+=(const char c){
	char *p=new char[len+2]{};
	memcpy(p,ptr,len);
	/*
	memcpy(p+len,&c,1);
	delete []ptr;
	len++;
	*/
	delete []ptr;
	p[len++]=c;
	ptr=p;
	return *this;
    }
    //"hell\0\0ab"
    //"hell\0\0az"
    bool operator==(const String &s)const{
	/*
	if(len!=s.len)
	    return false;
	int i=0;
	while(i<len){
	    if(ptr[i]!=s.ptr[i])
		return false;
	    i++;
	}
	return true;
	*/
	int n=len<s.len?len:s.len;
	if(memcmp(ptr,s.ptr,n+1)==0)
	    return len==s.len;
	else
	    return false;
    }
    bool operator!=(const String &s)const{
//	return !(ptr==s.ptr)&&len!=s.len;
	int n=len<s.len?len:s.len;
	return memcmp(ptr,s.ptr,n+1)!=0;
    }
    bool operator>(const String &s)const{
	/*
	int n=len>s.len?len:s.len;
	int i=0;
	while(i<n){
	    if(ptr[i]!=s.ptr[i]){
		if(ptr[i]>s.ptr[i])
		    return true;
	    }
	    i++;
	}
	return false;//相等
    }
	*/
	int n=len<s.len?len:s.len;
	if(memcmp(ptr,s.ptr,n+1)>0)
	    return true;
	else if(memcmp(ptr,s.ptr,n+1)==0)
	    return len>s.len;
	else
	    return false;
    }
    bool operator<(const String &s)const{
	/*
	int n=len>s.len?len:s.len;
	int i=0;
	while(i<n){
	    if(ptr[i]!=s.ptr[i]){
		if(ptr[i]<s.ptr[i])
		    return true;
	    }
	    i++;
	}
	return false;//相等
    }
	*/
	int n=len<s.len?len:s.len;
	if(memcmp(ptr,s.ptr,n+1)<0)
	    return true;
	else if(memcmp(ptr,s.ptr,n+1)==0)
	    return len<s.len;
	else
	    return false;
    }
#endif
};

int main(){
#if 0
    String s1("abc");
    String s2;
    s2=s1;
    s1.show();
    s2.show();
    cout<<"\n/******change s2******/\n"<<endl;
    s2.insert(0,"123");
    s1.show();
    s2.show();
    cout<<endl;
#endif
#if 0
    cout<<"s1:"<<endl;
    String s1("123");
    s1.show();
    cout<<"s2:"<<endl;
    String s2("abc");
    s2.show();
    String s3;
    cout<<"s3=s1+s2"<<endl;
    s3=s1+s2;
    s3.show();
    cout<<"s3+=s1"<<endl;
    s3+=s1;
    s3.show();
    cout<<"s3+='A'"<<endl;
    s3+='A';
    s3.show();
    cout<<"s3+='QWE'"<<endl;
    s3+="QWE";
    s3.show();
#endif
#if 1
    String s1("ab");
    String s2("abc");
    String s3("abc");
    String s4("abcd");
    String s5("abcz");
    s1.show();
    s2.show();
    s3.show();
    s4.show();
    s5.show();
    cout<<endl;
    if(s2==s3)
	cout<<"s2==s3"<<endl;
    if(s1!=s2)
	cout<<"s1!=s2"<<endl;
    if(s5>s4)
	cout<<"s5>s4"<<endl;
    if(s1<s5)
	cout<<"s1<s5"<<endl;
    cout<<endl;
#endif
#if 0
    //测试String成员函数:show(),size(),初始化2种方式
    String s1("hello");
    s1.show();
    String s2(10,'a');
    s2.show();
    cout<<s2.size()<<endl;
    s1.resize(3);
    s1.show();
    cout<<s1.size()<<endl;
#endif
#if 0
    //测试String成员函数:erase()
    String s3("hello");
    s3.erase(3,1);
    s3.show();
    cout<<s3.size()<<endl;
#endif
#if 0
    //测试String成员函数:insert()
    String s4("yellow");
    s4.insert(2,"red");
    s4.show();
    cout<<s4.size()<<endl;
#endif
#if 0
    //测试String成员函数:replace()
    String s5("hello");
    s5.replace(0,0,"you 22");
    s5.show();
    cout<<s5.size()<<endl;
#endif
#if 0
    //测试String成员函数:.substr()
    String s6("helloworld");
    s6.show();
    cout<<s6.size()<<endl;

    String s7=s6.substr(4,8);
    s7.show();
    cout<<s7.size()<<endl;
#endif
#if 0
    //同类型参数的构造函数，叫做拷贝构造函数
    //如果没有手动写拷贝构造函数，会合成1个默认的,浅拷贝
    String s1(8,'A');
    String s2(s1);//String(String)
    s1.show();
    s2.show();

    //编译时加-fno
    //-fno-elide-constructots 关闭返回值优化
    String s3=s2.show(s1);
    s3.show();
    //s3 = 匿名对象 = s2.show(s1);
    //s3 = "hello";
    //s3 = String("hello");
    //默认的拷贝构造只是简单的变量赋值
#endif
#if 0
    //浅拷贝，只是指针的简单赋值
    //s2.ptr = s1.ptr; s2.len = s1.len
    //例如：
    int *p1=new int(4);
    int *p2=p1;
    *p1=10;
    cout<<*p1<<","<<*p2<<endl;
    delete p1;
    delete p2;
    //通过同类型的对象，构造本对象的时候，调用拷贝构造函数
    //1.定义一个对象
    //2.函数传参，传的类型是非引用类型
    //3.函数返回值，返回类型是非引用类型
#endif
#if 0
    //深拷贝，内容相同，每个指针都有自己的空间地址
    //例如：
    int *p3=new int(4);
    int *p4=new int(*p3);
    *p3=20;
    cout<<*p3<<","<<*p4<<endl;
    delete p3;
    delete p4;
#endif
#if 0
    //测试string
    string s1;
    string s2('a',10);
    string s3("hello");
    cout<<&s3<<endl;
    cout<<(void *)&s3[0]<<endl;
    s3="abcdefghijklmnopqrstuvwsyz";
    cout<<(void *)&s3[0]<<endl;
#endif
#if 0
    //右值引用，引用的是一个常量，不能修改内容
    const int &n1=10;
//    n1=20;//错误
    cout<<n1<<endl;//10
    //左值引用，引用的是一个变量，可以修改内容
    int a=20;
    int &n2=a;
    n2=30;
    cout<<n2<<endl;//30
#endif
#if 0
    String s1("hello");
    String s2("red");
    s2.insert(0,s1);
    s2.insert(0,"bulue");
    s2.show();
    String s3=s2.substr(2,7);
    s3.show();
    s2.show();
#endif
    /*
    int b;//
    String(volatile String &s){}
    volatile int a;//
    register int i;// 
    const int c;//
    */
    return 0;
}
