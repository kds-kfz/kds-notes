#include<iostream>
//双向链表,容器
#include<list>
//2.std_list.cpp
using namespace std;
int main(){
    //0个结点
    list<int> l0;
    //n个结点，每个都是默认值
    list<int> l1(4);
    //n个节点都是value
    list<int> l2(4,10);
    list<int> l3(l2);
    list<int> l4={1,2,3,4};

    l2.empty();
    l2.size();
    l2.pop_back();
    l2.pop_front();
    l2.push_front(2);
    l2.push_back(5);
#if 1
    //删除所有值为value的节点
    l2.remove(10);
    //list链表内的成员方法，从小到大排序
    l2.sort();

    //没有+n,+=,-=
    list<int>::iterator p=l2.begin();
    p++;
    l2.insert(p,45);
//    l2.erase(p);
    
    for(auto m:l2)
	cout<<m<<" ";
    cout<<endl;
#endif
    cout<<"front = "<<l2.front()<<endl;
    //头
    l2.front()=100;
    //尾
    l2.back()=200;

    for(list<int>::iterator it=l2.begin();it!=l2.end();it++)
	cout<<*it<<",";
    cout<<endl;
    return 0;
}
