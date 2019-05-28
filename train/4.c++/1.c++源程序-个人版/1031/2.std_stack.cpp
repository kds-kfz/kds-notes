#include<iostream>
#include<stack>
//2.std_stack.cpp
using namespace std;
//STL容器
#if 0
//栈:先进后出,存数据
    1 2 3 4	<--进
    1 2 3 4	-->出
-----------------------------
#endif
int main(){
    stack<int> s1;
    s1.push(2);
    s1.push(20);
    cout<<s1.top()<<endl;
    s1.top()=30;
    
    cout<<"top = "<<s1.top()<<endl;
    cout<<"size = "<<s1.size()<<endl;
    cout<<s1.empty()<<endl;
    
    s1.pop();
    cout<<"top = "<<s1.top()<<endl;
    cout<<"size = "<<s1.size()<<endl;
    s1.pop();
    
    //这时栈没有数据，不能够访问，段错误
    //error reading variable,读变量错误
//    cout<<"top = "<<s1.top()<<endl;
    cout<<"size = "<<s1.size()<<endl;
    return 0;
}
