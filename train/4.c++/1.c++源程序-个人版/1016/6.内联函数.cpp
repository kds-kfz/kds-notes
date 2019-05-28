#include<iostream>
//6.cpp
using namespace std;

struct Stu{
    int id;
    string name;
    int math;
    int english;
    int c;
    int total;
};
//inline内联函数，实在调用的位置内敛的展开，在编译的时候执行
//适用于函数比较简单，逻辑不复杂，代码简短情况，不能用递归，inline只是对编译的一个建议
//在调用之前要有整个函数的具体实现，所以会把内联函数写在头文件里
//优点：提高效率
//缺点：增加里代码的长度(编译到了二进制文件可以看出)

//与宏比较有区别
//宏是简单替换，在预处理阶段处理
//
inline bool cmpMath(Stu s1,Stu s2){
    return s1.math > s2.math; 
}

bool cmpC(Stu s1,Stu s2){
    return s1.c > s2.c; 
}

void Swap(Stu &s1,Stu &s2){
    Stu d1=s1;
    s1=s2;
    s2=d1;
}

void sort(Stu *arr,int n){
    for(int i=0;i<n-1;i++){
	for(int j=0;j<n-i-1;j++){
	    if(cmpMath(arr[j],arr[j+1]))
		//if(arr[j].c>arr[j+1].c)//时间复杂度相对来说要小
		Swap(arr[j],arr[j+1]);
	}
    }
}
void show(Stu *arr,int n){
    for(int i=0;i<n;i++){
	cout<<arr[i].id<<" "<<arr[i].name<<" "<<arr[i].math
	    <<" "<<arr[i].english<<" "<<arr[i].c<<" "<<arr[i].total<<"\n";
    }

}
int main(){
    Stu s[5]={
	{1004,"lisi",25,25,25,75},
	{1005,"lisi",30,30,30,90},
	{1002,"lisi",15,15,15,45},
	{1001,"lisi",10,10,10,30},
	{1003,"lisi",20,20,20,60},
    };
    show(s,5);
    cout<<endl;
    sort(s,5);
    show(s,5);
    return 0;
}
