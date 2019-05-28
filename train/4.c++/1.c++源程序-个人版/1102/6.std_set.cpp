#include<iostream>
#include<set>
#include<algorithm>
//算法头文件
//6.std_set.cpp
using namespace std;
//集合，容器
#if 0
    交集
    set_intersection(s1,begin(),s1.end(),
	    s2.begin(),s2.end(),inserter(s4,s4.begin()))
    并集
    set_union()
    差集
    set_difference()
    对称差集
    set_symmetric_difference()
#endif

int main(){
    //有序，唯一，内部自动从小到大排序
    set<int> s1={1,2,5,9,56,4,10};
    set<int> s2(s1.begin(),s1.end());
    set<int> s3={2,55,56,4,100,3};
    #if 0
    //即使原来集合里有内容，求交集...之后
    //原内容与结果合在一起，有序，从小到大
    set<int> s4;
    set<int> s5;
    set<int> s6;
    set<int> s7;
    //求交集
    set_intersection(s1.begin(),s1.end(),s3.begin(),s3.end(),
	    inserter(s4,s4.begin()));
    //求并集
    set_union(s1.begin(),s1.end(),s3.begin(),s3.end(),
	    inserter(s5,s5.begin()));
    //求差集
    set_difference(s1.begin(),s1.end(),s3.begin(),s3.end(),
	    inserter(s6,s6.begin()));
    //求对称差集
    set_symmetric_difference(s1.begin(),s1.end(),s3.begin(),s3.end(),
	    inserter(s7,s7.begin()));
    cout<<"s1集合 ";//1 2 4 5 9 10 56
    for(auto m:s1)
	cout<<m<<" ";
    cout<<endl;
    cout<<"s3集合 ";//2 3 4 55 56 100
    for(auto m:s3)
	cout<<m<<" ";
    cout<<endl;
    cout<<"-----------------------------------"<<endl;
    cout<<"s1与s3 交集 ";//2 4 56
    for(auto m:s4)
	cout<<m<<" ";
    cout<<endl;
    cout<<"s1与s3 并集 ";//1 2 3 4 5 9 10 55 56 100
    for(auto m:s5)
	cout<<m<<" ";
    cout<<endl;
    //s1-s1与s3的交集
    cout<<"s1与s3 差集 ";//1 5 9 10 
    for(auto m:s6)
	cout<<m<<" ";
    cout<<endl;
    //(s1-s1与s3的交集)+(s2-s1与s3的交集)
    cout<<"s1与s3 对称差集 ";//1 3 5 9 10 55 100 
    for(auto m:s7)
	cout<<m<<" ";
    cout<<endl;
    #endif 
    #if 1
    cout<<s1.size()<<endl;
    for(auto m:s1)
	cout<<m<<" ";
    cout<<endl;

    for(auto it=s2.begin();it!=s2.end();it++)
	cout<<*it<<",";
    cout<<endl;

    s1.insert(45);
    s1.erase(1);
    auto it=s1.find(9);
    if(it!=s1.end())
	cout<<"it "<<*it<<endl;
    else
	cout<<"没找到"<<endl;
    #endif 
    return 0;
}
