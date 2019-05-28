#include<iostream>
#include<cstring>
//7.my_vector.cpp
using namespace std;
template <typename T>
class Vector
{
    T *ptr;
    unsigned int len;
    unsigned int max;
    public:
    //嵌套类和外部的类没有任何关系，只有位置有关系
    //Vector<> ::Iterator it;
    
    //自己的迭带器
    class Iterator
    {
	T* pi;
	public:
	Iterator() : pi(nullptr){}
	Iterator(T* p) : pi(p){}
	bool operator==(const Iterator &it){
	    return pi==it.pi;
	}
	bool operator!=(const Iterator &it){
	    return pi!=it.pi;
	}
	Iterator operator++(){
	    pi++;
	    return *this;
	}
	Iterator operator++(int){
	    Iterator t=*this;
	    pi++;
	    return t;
	}
	T &operator*(){
	    return *pi;
	}
	T *operator->(){
	    return pi;
	}
	/*
	==
	!=
	<
	>
	++
	--
	*
	->
	*/
    };
    Vector() : ptr(nullptr),len(0),max(4){
//	ptr=new T[max]{};
    }
    Vector(int n){
//抛出异常
#ifdef DEBUG
	if(n<0) abort();
#else
	if(n<0) n=0;
#endif
	max=n>4?n:4;
	len=n;
	//{} 初始化会把每个基本数据类型的成员初始化成0
	//把自定义类型初始化成默认值
	ptr=new T[max]{};
    }
    void show(){
	for(int i=0;i<len;i++)
	    cout<<ptr[i].id<<" "<<ptr[i].name<<" "<<" ";
	cout<<endl;
    }
    Vector(unsigned int n, const T& value) : len(n),max(n){
	//申请空间类型是无符号整型
	//-1,或100000000000000,申请空间失败
	ptr=new T[max];
	for(int i=0;i<len;i++)
	    ptr[i]=value;
    }
    ~Vector(){
	cout<<"delete :"<<ptr->id<<endl;
	delete []ptr;
    }
    Vector(const Vector& v) : len(v.len),max(v.max){
	ptr=new T[max];
	for(int i=0;i<len;i++)
	    ptr[i]=v.ptr[i];
    }
    int size(){
	return len;
    }
    bool empty(){
	return len==0;
    }
    void push_back(const T &v){
	if(len>=max){
	max*=2;
	T *p=new T[max]{};
	int i=0;
	for(;i<len;i++)
	    p[i]=ptr[i];
	len+=1;
	p[i]=v;
	delete []ptr;
	ptr=p;
	}
    }
    T &operator[](int n){
	return ptr[n];
	//*(ptr+n);
    }
    Vector operator=(const Vector &v){//=delete;
	if(ptr==&v)
	    return *this;
	delete []ptr;
	len=v.len;
	max=v.max;
	ptr=new T[max];
	for(int i=0;i<len;i++)
	    ptr[i]=v.ptr[i];
	return *this;
    }
    void pop_back(){
	if(len>0)
	    len--;
    }
    Iterator begin() {
	return Iterator(ptr);
    }
    Iterator end() {
	return Iterator(ptr+len);
    }
    /*
    //resize()
    end()
    */
};
struct Stu{
    int id;
    string name;
};
/*
Vector<Student> v1;
Vector<Student> ::Iterator it = v1.begin();

it = v1.begin();
it == != ++
*it ---> Student
(*it).id
it->id
*/
int main(){
    Vector <Stu>s1(5,{1001,"lisi"});
    Vector <Stu>s2(s1);
//    s1.show();
//    s2.show();
    s1.push_back({1002,"lisi"});

    for(int i=0;i<s1.size();i++)
	cout<<s1[i].id<<" "<<s1[i].name<<" ";
    cout<<endl;

    for(auto m:s1)
	cout<<m.id<<","<<m.name<<",";
    cout<<endl;

    for(auto it=s1.begin();it!=s1.end();it++)
	cout<<it->id<<"-"<<it->name<<"-";
    cout<<endl;

    return 0;
}
