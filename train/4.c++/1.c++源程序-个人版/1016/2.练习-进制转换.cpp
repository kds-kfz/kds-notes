#include<iostream>
//2.cpp
using namespace std;

void fun(int *arr,int n){
    //数组传入之后退化成指针，不能使用范围for循环
//    for(int n:arr){}//错误
}

//1.string小写转大写，
void toUpper(string &s){
    for(char &n:s)
	if(n>='a'&&n<='z')
	    n-=32;
} 
//2.移除string里的标点字符，
string removeSpot(string s){
#if 0
    for(int i=0;i<s.size();){
	//如果第一个是标点符号
	while(!(s[i]>='a'&&s[i]<='z'||s[i]>='A'
		    &&s[i]<='Z'||s[i]>='0'&&s[i]<='9')&&i<s.size()){
		s.erase(i,1);//此时第一个字符不存在
	}
	//如果下一个不是标点符号
	if((s[i+1]>='a'&&s[i+1]<='z'||s[i+1]>='A'
		    &&s[i+1]<='Z'||s[i+1]>='0'&&s[i+1]<='9')&&i<s.size())
	    continue;
	else
	    i++;
    }
    return s;
#endif
    string res;
    for(auto c:s){
	if(c<='z'&&c>='a'||c<='Z'&&c>='A'||c<='9'&&c>='0')
	    res+=c;
    }
    return res;
}
//3.数字转16进制string，
string tiHex(int n){
#if 0
    //只限2位
    int count=0;
    int i;
    int j;
    static string s;
    i=n/16;
    n%=16;
    if(n>=0&&n<10&&i==0)
	s+=n+'0';
    else if(n>9&&n<16&&i==0){
	switch(n){
	    case 10:s+='A';break;
	    case 11:s+='B';break;
	    case 12:s+='C';break;
	    case 13:s+='D';break;
	    case 14:s+='E';break;
	    case 15:s+='F';break;
	}
    }
    else{
	if(i>0&&i<10)
	    s+=i+'0';
	if(n>9&&n<16){
	    switch(n){
		case 10:s+='A';break;
		case 11:s+='B';break;
		case 12:s+='C';break;
		case 13:s+='D';break;
		case 14:s+='E';break;
		case 15:s+='F';break;
	    }
	}
	else
	    s+=n+'0';
//	tiHex(n);
    }	
    return s;
#endif
    int a=0;
    string s="0123456789ABCDEF";
    string res;
    while(n){
	a=n%16;
	res=s[a]+res;
	n/=16;
    }
    return res;
}

int main(){
#if 0
    int arr[4]={1,2,3,4};
    //c++11标准中，范围for循环语句：只能从结构(数组，string)的开始到末尾，从头到位打印
    //编译需加std=c++11
    //int n;//在外面定义n是c++14标准
    for(int n : arr)//int n=arr[...]
	cout<<n<<" ";
    cout<<endl;

    for(int &n : arr){//定义引用n 可以修改数组里的值
	n=10;	      //int &n=arr[...]
	cout<<n<<" ";
    }
    cout<<endl;

    for(int i=0;i<4;i++)
	cout<<arr[i]<<" ";
    cout<<endl;

    string s("hello");
    //auto自动推倒成员类型
    for(auto c:s)
	cout<<c<<" ";
    cout<<endl;
#endif
    //练习
    //1.string小写转大写，
    //2.移除string里的标点字符，
    //3.数字转16进制string，

    string s1(",a,b.c/d;e'f!1@2#3$%^&*()_+-=");
    cout<<s1<<endl;
    toUpper(s1);
    cout<<s1<<endl;

    s1=removeSpot(s1);
    cout<<s1<<endl;

    cout<<"输入1个数字"<<endl;
    int num=0;
    cin>>num;
    string s2=tiHex(num);
    cout<<s2<<endl;
    return 0;
}
