#include<iostream>
//5.cpp
using namespace std;

struct Stu{
    int id;
    string name;
    int math;
    int english;
    int c;
    int total;
};

bool cmpC(Stu s1,Stu s2){
    return s1.c > s2.c; 
}

void Swap(Stu &s1,Stu &s2){
    Stu d1=s1;
    s1=s2;
    s2=d1;
}

void sort(Stu *arr,int n,bool (*p)(Stu,Stu)){
    for(int i=0;i<n-1;i++){
	for(int j=0;j<n-i-1;j++){
	    if(p(arr[j],arr[j+1]))
		Swap(arr[j],arr[j+1]);
	}
    }
}

void show(Stu *arr,int n){
    for(int i=0;i<n;i++){
	cout<<arr[i].id<<" ";
	cout<<arr[i].name<<" ";
	cout<<arr[i].math<<" ";
	cout<<arr[i].english<<" ";
	cout<<arr[i].c<<" ";
	cout<<arr[i].total<<"\n";
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
    sort(s,5,cmpC);
    show(s,5);
    return 0;
}
