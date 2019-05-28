#include<iostream>
//7.work-提取string中字符与数字.cpp
using namespace std;
#if 0
练习：
    80个长度以内的字符串，
    string s="hello12world34red56friend90oppo345";
    使用范围for循环，分成2个数组
    string a1[40]=""hello", ..."
    string a2[40]="12,34, ..."
    使用模板对这两个数组排序，使用选择排序
    输出2个数组，使用函数重载
#endif

template <typename T>
void Sort(T *a,int n){
    int i,j;
    for(i=0;i<n-1;i++){
	T temp=a[0];
	int m=0;
	for(j=1;j<n-i;j++){
	    if(temp<a[j]){
		temp=a[j];
		m=j;
	    }
	}
	if(m!=n-1-i){
	    a[m]=a[n-1-i];
	    a[n-i-1]=temp;
	}
    }
}

//数字string转int
int StrToInt1(string a){
    int num=0;
    for(int j=0;j<a.size();j++){
	num*=10;
	num+=a[j]-'0';
    }
    return num;
}

//数字string数组转int数组,a->b
void StrToInt(string *a,int *b,int n) {
//    int b[n]={0};
    string *p=a;
    int k=0;
    for(int i=0;i<n;i++){
	int num=StrToInt1(*p++);
	b[k++]=num;
    }
}
template <typename T>
void show(T *a,int n){
    for(int i=0;i<n;i++)
	cout<<a[i]<<" ";
    cout<<endl;
}
int main(){
#if 1
    string s="345hello39world22red56friend90oppo0a";
    string a1[40];//数字
    string a2[40];//字符
    string a3;
    string a4;
    int flag1=0;
    int flag2=0;

    int i=0,j=0;
    for(auto n:s){
	if(n>='0'&&n<='9'){
	    a4+=n;
	    if(flag2==1){
	    a2[j++]=a3;
//	    a3.resize(0);//清空数组
	    a3="";	//清空数组
	    flag2=0;
	    }
	    flag1=1;
	}
	else{
	    a3+=n;
	    if(flag1==1){
	    a1[i++]=a4;
//	    a4.resize(0);//清空数组
	    a4="";	//清空数组
	    flag1=0;
	    }
	    flag2=1;
	}
    }
    //加最后一个字符串
    a1[i]=a4;
    a2[j]=a3;
    //打印数字
    /*
    for(auto n:a1)
	cout<<n<<",";
    cout<<endl;
*/
    show(a1,6);
    //打印字母
    /*
    for(auto n:a2)
	cout<<n<<" ";
    cout<<endl;
*/
    show(a2,6);
    //string数字字符数组转int数组
    int x[6];
    StrToInt(a1,x,6);
    //排序后打印数字
    Sort(x,6);
    show(x,6);

    //排序后打印字母
    Sort(a2,6);
    show(a2,6);
#endif
    return 0;
}
