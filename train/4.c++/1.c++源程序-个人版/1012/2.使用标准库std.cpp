#include<iostream>
//2.cpp
//using namespace std;
//在std空间里不要定义重名变量和函数

using std::cout;
using std::cin;
using std::endl;

int main(){
#if 0
    int i=10;
    cout<<"hello world\n";
    cout<<2+3<<endl;//5
    cout<<(i++,i++,i++)<<endl;//12
    i=10;
    cout<<(++i,++i,i++)<<","<<(++i,i++,i++)<<endl;//15,12
    i=10;
    cout<<(++i,++i,i++)<<","<<(i++,i++,++i)<<endl;//15,16
    cout<<(2,3)<<"\n";//()从左到右，打印结果3
#endif
#if 1    
    int a,b;
    char arr[20];
    char *p=arr;
    cout<<"输入2个数"<<endl;
    cin>>a>>b;
    cout<<"a="<<a<<","<<"b="<<b<<endl;

    cout<<"输入1个字符串"<<endl;
    cin>>arr;
    cout<<arr<<endl;
    cout<<(void *)arr<<endl;
    cout<<p<<endl;
    cout<<(void *)(p+1)<<endl;
    cout<<*(p+1)<<endl;

//#endif
    struct Student{
	int id;
	char name[20];
    };
//    struct Student d1={0};
//    struct Student *p1=&d1;

//    Student==struct Student
    Student d1={0};
    Student *p1=&d1;
    cout<<"输入学生学号,名字\n";
//    cin>>d1.id>>d1.name;
//    cout<<d1.id<<" "<<d1.name<<endl;
    cin>>p1->id>>p1->name;
    cout<<p1->id<<" "<<p1->name<<endl;
#endif
    return 0;
}
