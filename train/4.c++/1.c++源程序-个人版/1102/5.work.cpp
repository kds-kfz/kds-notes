#include<iostream>
#include<map>
//5.work.cpp
using namespace std;
#if 0
练习2
统计从终端输入的字符串和相应的次数
hello apple apple id name id id
hello :1
apple :2
id :3
name :1
map<string,int> m;
#endif

int main(){
#if 0
    map<string,int> m;
    string buf;
    while(1){
    cout<<"请输入字符串(输入:q结束)"<<endl;
    cin>>buf;
    if(buf==":q")break;
    map<string,int>::iterator i=m.find(buf);
    if(i!=m.end())
	i->second++;
    m.insert({buf,1});
    buf.resize(0);
    }
    for(auto k:m)
	cout<<k.first<<" : "<<k.second<<endl;
#endif
#if 1
    string s;
    map<string,int>m;
    cout<<"请输入字符串(输入:EOF结束)"<<endl;
    while(cin>>s){
	/*
	if(m.find(s)==m.end())
	    m.insert({s,1});
	else
	    m[s]++;
	    */
	m[s]++;
    }
    for(auto k:m)
	cout<<k.first<<" : "<<k.second<<endl;
#endif
    return 0;
}
