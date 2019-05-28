#include<iostream>
#include<vector>
#include<algorithm>
//5.work.cpp
using namespace std;

//练习2.struct Stu{string name,int age}
//vector<Stu> v
//增加不定个数的Stu,排序,打印

struct Stu{
    int id;
    string name;
    bool operator<(const Stu &s)const{
	return id<s.id;
    }
};

bool CmpStu(const Stu &s1,const Stu &s2){
    return s1.name>s2.name;
}
bool CmpInt(const int &a,const int &b){
    return a<b;
}

template <typename T>
void Swap(T &a,T &b){
    T t=a;
    a=b;
    b=t;
}
template <typename T>
void Swap(T &s,int a,int b){
    Swap(s[a],s[b]);
}

void sort1(vector<Stu> &v){
//    auto p=v.begin();
    int n=0;//记录最大下标
//    int len=0;//统计动态数组长度
    int len=v.size();//保存动态数组长度
//    for(vector<Stu>::iterator it=v.begin();it!=v.end();it++,len++);
    for(int i=0;i<len-1;i++){
//    int maxid=p->id;//每次默认最大id是第一个
    int maxid=v[0].id;
    int n1=0;//记录最大id
    for(vector<Stu>::iterator it=v.begin();it!=v.end()-i-1;it++){
	if(maxid<(*it).id){
	    n1++;
	    maxid=(*it).id;
	    n=n1;
	}
    }
    if(n!=len-i-1)
    Swap(v,n,len-i-1);    
    }
}

int main(){
#if 1
    vector<Stu> v;
    v.push_back({1001,"lisi"});
    v.push_back({1003,"lisi"});
    v.push_back({1005,"lisi"});
    v.push_back({1004,"lisi"});
    v.push_back({1002,"lisi"});
    sort1(v);
    for(auto m:v)
	cout<<m.id<<" "<<m.name<<endl;
#endif
#if 0
    vector<Stu> v;
//    v.push_back({1000,"lisi"});
    Stu s;
    string id;
    while(1){
	cout<<"请输入学生名字(输入:q结束):\n";
	cin>>s.name;
	if(s.name==":q")break;
	cout<<"请输入学生id:\n";
	cin>>id;
	s.id=atoi(id.c_str());
	v.push_back(s);
    }
    for(auto m:v)
	cout<<m.id<<" "<<m.name<<endl;
    cout<<"------------------------------"<<endl;

    //模板排序，默认两个参数，从小到大，必须重载<运算符
    //前两个参数是要排序的位置，不包含末位，
    //第三个参数是一个函数指针，bool(*p)(Stu,Stu)
    //bool(*)(int,int)
//    sort(v.begin(),v.end(),CmpStu);
    sort(v.begin(),v.end());
    for(auto it=v.begin();it!=v.end();it++)
	cout<<it->id<<" "<<it->name<<endl;
#endif
#if 0
    vector<int>arr={3,5,1,2,4,-4,45,22,88,-3};
    sort(arr.begin(),arr.end(),CmpInt);
//    sort(arr,arr+10,CmpInt);//错误，没有+
    for(auto it=arr.begin();it!=arr.end();it++)
	cout<<*it<<" ";
    cout<<endl;
#endif
    return 0;
}
