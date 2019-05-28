#include<iostream>
//3.cpp
using namespace std;

//函数模板
//Max是模板函数，类型是T
template <typename T> T Max(T a, T b){
    return a>b?a:b;
}
//4 7 2 -3//temp=7,i=1

//4 7 2 -3//temp=2,i=2,j=2
//4 7 7 -3//temp=2,i=2,j=1
//4 4 7 -3//temp=2,i=2,j=0
//2 4 7 -3//temp=2,i=3

//2 4 7 -3//temp=-3,i=3,j=3
//2 4 7 7 //temp=-3,i=3,j=2
//2 4 4 7 //temp=-3,i=3,j=1
//2 2 4 7 //temp=-3,i=3,j=0
//-3 2 4 7//temp=-3,i=4
template <typename T> //T==int
void Sort(T *a,int n){//T==string
#if 0
    T temp;
    int i,j;
    for(i=1;i<n;i++){
	temp=a[i];
	for(j=i;j>0&&temp<a[j-1];j--)
	    a[j]=a[j-1];
	a[j]=temp;
    }
#endif
#if 1
    T temp;
    int i,j;
    for(i=1;i<n;i++){
	temp=a[i];
	for(j=i-1;j>=0;j--){
	    if(temp<a[j])
		a[j+1]=a[j];
	    else{
		break;
	    }
	}
	a[j+1]=temp;
    }
#endif
}
/*
template <typename T> 
void Sort(T a,int n){ //T==int *
    auto m=a[0];      //T==string *
}
*/
int main(){
#if 0
    cout<<Max(1,2)<<endl;
    cout<<Max(1.3,2.6)<<endl;
    cout<<Max(-1.3f,-2.9f)<<endl;
    //const char *s1="auwe",&s1[0]
    //const char *s2="axw",&s1[0]
    //推断是const char类型,比较的是常量字符串首地址大小
    cout<<Max("auwe","axw")<<endl<<endl;

    cout<<Max(string("auwe"),string("axw"))<<endl;
    //<string>表示模板的类型T==string
    cout<<Max<string>("auwe","axw")<<endl;
#endif
#if 1
    //练习：写一个排序函数模板，使用插入排序法
    int a[4]={4,7,2,-3};
    string s[4]={"hello","World","yellow","apple"};
    Sort(a,4);
    for(auto &n:a)
	cout<<n<<" ";
    cout<<endl<<endl;

    Sort(s,4);
    for(auto &n:s)
	cout<<n<<" ";
    cout<<endl<<endl;
#endif
    return 0;
}
