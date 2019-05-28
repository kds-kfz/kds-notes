#include<iostream>
#include<vector>
//4.std_vector.cpp
using namespace std;
//动态数组
void print(vector<int> &v){
    for(auto m:v)
	cout<<m<<" ";
    cout<<endl;
}
int main(){
    int a[4];
    //size()为0
    vector<int> v0;
    //初始化n个默认值
    vector<int>v1(4);
    vector<string>v2(4,"hello");
    //拷贝初始化
    auto v3=v2;
    //列表初始化
    vector<int> v4{1,2,3,4};
    vector<int> v5={11,12,13,14};
    for(int i=0;i<v2.size();i++)
	cout<<v2[i]<<endl;
    for(auto m:v4)
	cout<<m<<" ";
    cout<<endl;
    //使用迭代器访问动态数组的成员
    //it相当于一个指针，指向数组某个位置
    for(vector<int>::iterator it = v4.begin();it!=v4.end();it++)
	cout<<*it<<",";
    cout<<endl;
    for(auto it = v2.begin();it!=v2.end();it++)
	cout<<*it<<"-";
    cout<<endl;
    //从后面增加
    //当增加到一定空间大小，则申请新空间，这时首地址改变
    cout<<&v5[0]<<endl;
    v5.push_back(33);
    v5.push_back(34);
    v5.push_back(35);
    cout<<v5.size()<<endl;

    //插入在it的位置，it后的数向后移动
    //此时插入后，动态数组地址改变
    auto it=v5.begin();
    it++;
    //插入后，返回值是新的地址
    //获得返回值，返回值是新的迭代器位置 
    it=v5.insert(it,88);
    it=v5.insert(it,0);
        
    print(v5);
    cout<<&v5[0]<<endl;
    vector<int>::iterator p=v5.begin();
//    it=v5.erase(it);
    //删除中间后，后面的数据向前移动一个
    p=v5.erase(p+3);
    cout<<&v5[0]<<endl;
    //数组的首地址不变

    print(v5);
    //从后面删除,只是简单的size()减减,空间依然存在，数据也存在
    //此时首地址不变
    v5.pop_back();
    print(v5);
    v5.pop_back();
    v5.pop_back();
    v5.pop_back();
    v5.pop_back();
    v5.pop_back();
    v5.pop_back();
    cout<<v5.size()<<endl;
    //size()为0,空间没有删除,还可以访问原来的值
    //如果访问到数组外，则是非法，随机值
    for(int i=0;i<6;i++)
	cout<<v5[i]<<" ";
    cout<<endl;
    return 0;
}
