#include<iostream>
//6.work-模板函数.cpp
using namespace std;

//练习：Swap特例,交换两个student对象的name
struct Stu{
    int id;
    string name;
};
//模板
template <typename T>
void Swap(T &a,T &b){
    Stu t=a;
    a=b;
    b=t;
}
//特例
//(有模板时，是特例，没模板时,是普通函数)
void Swap(Stu &a,Stu &b){
    Stu t;
    t.name=a.name;
    a.name=b.name;
    b.name=t.name;
}

int main(){
    Stu d1={1001,"yellow"};
    Stu d2={1002,"bule"};
    cout<<d1.id<<","<<d1.name<<endl;
    cout<<d2.id<<","<<d2.name<<endl<<endl;
    
    //交换name,使用特例
    Swap(d1,d2);
    cout<<d1.id<<","<<d1.name<<endl;
    cout<<d2.id<<","<<d2.name<<endl<<endl;

    //交换结构体，使用模板
    Swap<Stu>(d1,d2);
    cout<<d1.id<<","<<d1.name<<endl;
    cout<<d2.id<<","<<d2.name<<endl;
    return 0;
}
