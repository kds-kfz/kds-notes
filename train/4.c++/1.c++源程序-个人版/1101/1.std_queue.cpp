#include<iostream>
#include<queue>
//1.std_queue.cpp
//单向队列
/*
#include<deque>
//双向队列
//有迭代器，可以双向删除和插入，遍历
*/
//2.cpp
using namespace std;
//队列
#if 0
//先进先出
<--  1 2 3  <--
front		back
先删除1,2,3
//没有迭代器,不能遍历，使用范围for循环
#endif

int main(){

    queue<int> q;
    q.push(21);
    q.push(31);
    q.push(41);
    cout<<q.size()<<endl;
    cout<<q.front()<<endl;
    cout<<q.back()<<endl;
    q.pop();

    cout<<q.size()<<endl;
    cout<<q.front()<<endl;
    cout<<q.back()<<endl;
    return 0;
}
