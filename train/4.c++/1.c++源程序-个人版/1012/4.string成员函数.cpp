#include<iostream>
//4.cpp
using namespace std;

int main (){
    string s1("hello");
    string s2;
    //.empty()判断是否是空的字符串,不空是1,空是0
    cout<<"s1 is empty ?"<<s1.empty()<<endl;
    s1[2]='A';//合法
//    s1[5]='G';//非法
    cout<<"is empty ?"<<s1.empty()<<endl;
    cout<<s1[1]<<endl;
#if 1
//    s2[0]='B';//非法
//    cout<<s2[0]<<endl;

//    s2=s1;//拷贝赋值
    //.resize()重置大小
    s2.resize(5);//申请5个字节空间给s2
    cout<<"s2_size="<<s2.size()<<endl;
    cout<<"s2 is empty ?"<<s2.empty()<<endl;
    for(int i=0;i<s2.size();i++)
	s2[i]=s1[i];
    cout<<"s2 is empty ?"<<s2.empty()<<endl;
    cout<<"s2="<<s2<<endl;
    
    s1="hello";
    s2="word";
//    s2=s1+s2;
    s2=s2+s1;//s2+=s1;
    cout<<"s2="<<s2<<endl;
    
    if(s1>s2)
	cout<<"s1>s2"<<endl;
    else
	cout<<"s1<s2"<<endl;
    
    //如果string连续加法，有可能会出错，尽量妹子只执行依次加法
    s1.resize(8);
    //多余的空间存在
    //cout打印string类型,长度是.size()
    for(int i=0;i<s1.size();i++)
	cout<<s1[i]<<",";
    cout<<endl;

    s2=s1+s2;
    s2=s2+'w';
    s2=s2+"SD";
    //电脑编译器不一样，有可能不支持加数字
    //s2=s2+97;
    cout<<"s2="<<s2<<endl;
    
    //.erase(pos,len)
    //橡皮檫，删除
    //参数pos,不能超数组范围，参数len,从参数pos开始删除len个字符
    s1.erase(1,1);
    cout<<"s1="<<s1<<endl;

    //.insert(pos,args) args:const char *,string
    //参数pos,不能超数组范围，参数args,从参数pos开始增加字符
    //字符类型可是const char *或者string
    s1.insert(1,"e");
    cout<<"s1="<<s1<<endl;
    
    //.replace(pos,len,args)
    //替换
    //参数pos,不能超数组范围，参数len,从参数pos开始len个字符被args替换
    s1.replace(1,3,"www");
    cout<<"s1="<<s1<<endl;
    
    //.substr(pos,len)
    //提取一段字符串返回
    //参数pos,不能超数组范围，参数len,从参数pos开始提取len个字符并将其返回
    string st=s1.substr(1,4);
    cout<<"s1="<<s1<<endl;
    cout<<"st="<<st<<endl;
#endif
    return 0;
}
