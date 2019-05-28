#include<iostream>
#include<vector>
//4.work.cpp
using namespace std;
//练习1：使用迭代其遍历整个数组，把其中的偶数删除掉
//vector<int>v1={1,2,3,4,5,6,7,8,9,10};
int main(){
#if 1
    vector<int>v1={2,2,2,2,4,5,6,7,8,9};
#if 0
    cout<<&v1[0]<<endl;
    auto p=v1.end();
//==    vector<int>::iterator p=v1.end();
    p=v1.erase(p-1);
    for(auto it=v1.begin();it!=v1.end();it++)
	cout<<*it<<" ";
    cout<<endl;
    p=v1.erase(p-1);
    for(auto it=v1.begin();it!=v1.end();it++)
	cout<<*it<<" ";
    cout<<endl;
    p=v1.erase(p-1);
    p=v1.erase(p-1);
    p=v1.erase(p-1);
    p=v1.erase(p-1);
    p=v1.erase(p-1);
    for(auto it=v1.begin();it!=v1.end();it++)
	cout<<*it<<" ";
    cout<<endl;
    cout<<&v1[0]<<endl;
#endif
    
    cout<<v1.size()<<endl;
    for(auto it=v1.begin();it!=v1.end();){
	if(*it%2==0)
//	    it=v1.erase(it);
	    v1.erase(it);
	else
	    it++;
    }
    /*
    for(auto it=v1.begin();it!=v1.end();it++)
	cout<<*it<<" ";
    cout<<endl;
    */
    for(auto m:v1)
	cout<<m<<" ";
    cout<<endl;
#endif
    return 0;
}
