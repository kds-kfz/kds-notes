#include<iostream>
#include<typeinfo>
#include<vector>
#include<map>
//1.typeid.cpp
using namespace std;

struct Stu{
    int id;
    string name;
};

int main(){
    char c='a';
    short s=7;
    int i=9;
    float f=1.2;
    long l=56;
    double d=3.14;
    int arr[4]={1,2,3,4};
    double arr2[4]={1.2,1.3,1.4,1.5};
    string str="abc";
    Stu s1={1101,"lisi"};
    int *p1;
    int **p2;
    const int *p3=&i;
    int const *p4=&i;
    int &t=i;
    vector<int>v;
    vector<string>v2;
    map<int,string>m;

    auto name=typeid(s1).name();

    //const char * typeid只能识别底层const
    //不能呢个识别顶层const和引用
//    if(typeid(i).name()==typeid(s).name())
    //比较两个数据的类型是否相同
//    if(typeid(i)==typeid(short))//判断i数据类型是否是short
    if(typeid(i)==typeid(s))
	cout<<"i==s"<<endl;
    else
	cout<<"i!=s"<<endl;
    //xx的类型名字
    cout<<typeid(c).name()<<endl;
    cout<<typeid(s).name()<<endl;
    cout<<typeid(i).name()<<endl;
    cout<<typeid(f).name()<<endl;
    cout<<typeid(l).name()<<endl;
    cout<<typeid(d).name()<<endl;
    cout<<typeid(arr).name()<<endl;
    cout<<typeid(arr2).name()<<endl;
    cout<<typeid(str).name()<<endl;
    cout<<typeid(s1).name()<<endl;
    cout<<typeid(p1).name()<<endl;
    cout<<typeid(p2).name()<<endl;
    cout<<typeid(p3).name()<<endl;
    cout<<typeid(p4).name()<<endl;
    cout<<typeid(t).name()<<endl;
    cout<<typeid(v).name()<<endl;
    cout<<typeid(v2).name()<<endl;
    cout<<typeid(m).name()<<endl;
    cout<<typeid(name).name()<<endl;

    return 0;
}
