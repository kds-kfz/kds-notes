#include<iostream>
#include<map>
//3.std_map.cpp
using namespace std;
//数据类型pair,映射，树状

int main(){
    pair<int,string> p;
    //key是唯一的，有序
    //key value
    map<int,string> m;
    //make_pair()函数是制作一个pair
    m.insert(pair<int,string>(1001,"lisi"));
    m.insert({1002,"lisi"});

    //如果key有相同的，后面的插入会失败
    //pair<iterator,bool>,此时需要打印second,判断是否插入成功
    //成功bool为1,失败为0
    auto a=m.insert({1002,"li"});
    cout<<"a = "<<a.second<<endl;

    //erase删除成功返回1
    int x=m.erase(1002);
    cout<<"x = "<<x<<endl;

    //m[key],查找，是相应关键字对应的值
    //是key对应的value
    m[1001]="keke";
    cout<<m[1001]<<endl;

    //如果没有key，会创造1个pair，
    //key=2001,value是默认值
    cout<<m[2001]<<endl;

    //创造1个pair
    m[-99]="xixi";

    //关键字key相同, 则覆盖之前的value
    m[-99]="lol";

    //m.find(key),如果找到key，返回相应的位置
    //如果没找到，返回m.end()位置
    map<int,string>::iterator i=m.find(1091);
    if(i!=m.end()){//找到输出
	cout<<"找到了"<<endl;
	cout<<i->first<<","<<i->second<<endl;
    }
    else{
	cout<<"没有找到"<<endl;
	/*
	//可以访问最后1个pair 
	i--;
	cout<<i->first<<","<<i->second<<endl;
	*/
    }

    //upper_bound(key),返回大于key的位置,>,返回的是迭代器的位置
    auto it1=m.upper_bound(1001);
    //lower_bound(key),返回不小于key的位置,>=
    auto it2=m.lower_bound(1001);
    cout<<"it1 upper "<<it1->first<<" "<<it1->second<<endl;
    cout<<"it2 lower "<<it2->first<<" "<<it2->second<<endl;

    //pair<map<>::iterator,map<>::iterator>返回上面两个位置
    auto it3=m.equal_range(1001);
    cout<<"it3 upper "<<it3.first->first<<" "<<it3.first->second<<endl;
    cout<<"it3 lower "<<it3.second->first<<" "<<it3.second->second<<endl;

    m.insert(make_pair(1003,"lisi"));

    //自带有序，key按从小到大自动排序
    for(auto i:m)
	cout<<i.first<<" "<<i.second<<endl;
    for(auto it=m.begin();it!=m.end();it++)
	cout<<it->first<<","<<it->second<<endl;

    map<string,string> k;
    k.insert({"lol","20171102"});
    for(auto i:k)
	cout<<i.first<<" "<<i.second<<endl;

    return 0;
}
