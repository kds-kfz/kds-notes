#include<iostream>
//1.std_pair.cpp
using namespace std;
//关联容器
int main(){
    pair<int,string> p;
    pair<int,string> p3;
    pair<int,string> p2(1002,"lisi");
    p.first=1001;
    p.second="lisi";
    cout<<p.first<<" "<<p.second<<endl;
    cout<<p2.first<<" "<<p2.second<<endl;
    
    //==比较first与second全相等
    //先比较第一个，如果第一个相等，再比较第二个
    if(p==p3)
	cout<<"p=p2"<<endl;
    if(p!=p3)
	cout<<"p!=p2"<<endl;
    if(p>p3)
	cout<<"p>p2"<<endl;
    if(p<p3)
	cout<<"p<p2"<<endl;
    return 0;
}
