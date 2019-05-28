#include<iostream>
//3.cpp
using namespace std;

int main (){
#if 0
    bool ok=true;
    cout<<"ok="<<ok<<endl;
    ok=false;
    cout<<"ok="<<ok<<endl;
    cout<<"bool="<<sizeof(bool)<<endl;
#endif

#if 1
    cout<<"string="<<sizeof(string)<<endl;
    string s1;
    string s2="abcd";	//拷贝初始化
    string s3("world"); //直接初始化
    string s4=s3;	//拷贝初始化
    string s5(10,'a');	//直接初始化,向栈空间申请10字节，初始化为a

    const char *p="holle";	 //指向的内容不可变，"holle"存在代码区
    const char *const p1="world";//指向不可改变
    p="1234";

    cout<<p<<endl;
    
    //sizeof测string类型的大小
    cout<<"s1="<<s1<<endl;
    cout<<"s1_size="<<sizeof(s1)<<endl;
    cout<<"s2="<<s2<<endl;
    cout<<"s2_size="<<sizeof(s2)<<endl;

    //.size()测某个具体对象的字符串长度
    cout<<"s3="<<s3<<endl;
    cout<<"s3_size="<<s3.size()<<endl;
    cout<<"s4="<<s4<<endl;
    cout<<"s6_size="<<s4.size()<<endl;
    cout<<"s5="<<s5<<endl;
    cout<<"s5_size="<<s5.size()<<endl;
#endif
    return 0;
}
