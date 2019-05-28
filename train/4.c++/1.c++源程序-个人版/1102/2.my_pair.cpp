#include<iostream>
#include<vector>
//2.my_pair.cpp
using namespace std;
//重写关联容器
template <typename T1,typename T2>
struct Pair{
    T1 first;
    T2 second;
    public:
    Pair(){}
    Pair(const T1 &a,const T2 &b) : first(a),second(b){}
    bool operator==(const Pair &p)const{
	return first==p.first&&second==p.second;
    }
    bool operator!=(const Pair &p){
	return !(*this==p);
    }
    bool operator<(const Pair &p)const{
//	return first<p.first||second<p.second;
	if(first<p.first)
	    return true;
	else if(first==p.first&&second<p.second)
	    return true;
	return false;
    }
    bool operator>(const Pair &p)const{
//	return first>p.first||second>p.second;
	return !(p<*this||p==*this);
    }
    bool operator<=(const Pair &p)const{
//	return first>p.first||second>p.second;
	return *this<p||*this==p;
    }
};

int main(){
    Pair<int,string> p1(1000,"keke");
    Pair<int,string> p2;
    p2.first=1000;
    p2.second="lisi";
    if(p1==p2)
	cout<<"p1=p2"<<endl;
    if(p1!=p2)
	cout<<"p1!=p2"<<endl;
    if(p1>p2)
	cout<<"p1>p2"<<endl;
    if(p1<p2)
	cout<<"p1<p2"<<endl;

    Pair<int,Pair<int,string> >p3(100,Pair<int,string>(200,"world"));
    cout<<p3.first<<" "<<p3.second.first<<" "<<p3.second.second<<endl;

    #if 1
    Pair<int,vector<string>> p4;
    p4.first=2017;
    p4.second.push_back("yellow");
    p4.second.push_back("red");
    p4.second.push_back("blue");
    #endif

    #if 1
    Pair<int,string[3]> p5;
    p5.first=2018;
    p5.second[0]="yellow";
    p5.second[1]="red";
    p5.second[2]="blue";
    #endif
    cout<<p4.first<<" ";
    for(auto k:p4.second)
	cout<<k<<" ";
    cout<<endl;
    cout<<p5.first<<" ";
    for(auto k:p5.second)
	cout<<k<<" ";
    cout<<endl;
    return 0;
}
