#include<iostream>
#include<cstring>
//6.my_stack.cpp
using namespace std;
//mystack
template <typename T>
class Stack{
    T *ptr;
    int len;
    int max;
    public:
    Stack() : ptr(NULL),len(0),max(4){}
//    Stack()
    ~Stack(){
	delete []ptr;
    }
    int size(){
	return len;
    }
    bool empty(){
	return len==0;
    }
    void push(const T &t){
	if(ptr==NULL)
	    ptr=new T[max];
	else if(len>=max){
	    max*=2;
	    T *p=new T[max];
	    for(int i=0;i<len;i++)
		p[i]=ptr[i];
	    delete []ptr;
	    ptr=p;
	    }
	ptr[len++]=t;
    }
    T &top(){
	if(len==0)
	    abort();
//	return ptr[0];
	return ptr[len-1];
    }
    void pop(){
//	delete []ptr[len];
//	ptr[len]=ptr[len-1];
	if(len>0)
	len--;
    }

};

int main(){
    Stack<int> a1;
    a1.push(22);
    a1.push(32);
    a1.push(42);
    cout<<a1.top()<<endl;
    cout<<a1.size()<<endl;
    cout<<a1.empty()<<endl;
    a1.pop();
    a1.pop();
    a1.pop();
    cout<<a1.top()<<endl;
    cout<<a1.size()<<endl;

    return 0;
}
