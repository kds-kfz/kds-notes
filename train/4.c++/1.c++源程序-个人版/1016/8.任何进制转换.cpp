#include<iostream>
#include<cmath>
//8.cpp
using namespace std;
#if 0
//练习1:
//把是字符数组分解称小的字符串和数字，分别存到数组里
//string s="sfds34dfa12sfs45sds";
//string a1[40];
//int a2[40];
//练习2:
//定义1个string数组，能对string进行排序，也能对string的长度排序
//sortStr(string *arr,int n,P);
//
"apple friend hello"
" "
//5 5 6
//练习3：
//任意进制相互转换
八进制十    ->	十六进制
0101	 十进制	
64+1	   65	41
//anyToDec();任意进制转换成10进制
//decToAny();10进制转换成任意进制
#endif
using pFun=bool (*)(string,string);

bool fun1(string a,string b){
    return a>b;
}

bool fun2(string a,string b){
    return a.size()>b.size();
}

void sortStr(string *arr,int n,pFun p){
    string t;
    for(int i=0;i<n-1;i++){
	for(int j=0;j<n-1-i;j++){
	    if(p(arr[j],arr[j+1])){
		t=arr[j];
		arr[j]=arr[j+1];
		arr[j+1]=t;
	    }
	}
    }
}

//任意进制转换成10进制
//123 (1*10+2)*10+3
//123 3*1+2*10+1*10*10
int anyToDec(string s,int n){
    int num=0;
    int a=0;
    for(int i=s.size()-1,j=0;i>=0;i--,j++){
	if(s[i]<='9'&&s[i]>='0')
	    a=s[i]-'0';
	else if(s[i]<='Z'&&s[i]>='A')
	    a=s[i]-'A'+10;
	else if(s[i]<='z'&&s[i]>='a')
	    a=s[i]-'a'+10;
	num+=a*pow(n,j);
    }
    return num;
}
//10进制转换成任意进制
string DecToAny(int num,int n){
    string r;
    int t=0;
    string s("0123456789ABCDEFGHIJKLMNOPQRSTUVWSYZ");
    while(num){
	t=num%n;
	r=s[t]+r;
	num/=n;
    }
    return r;
}
//任意进制转换成任意进制
string AnyToAny(string a,int n1,int n2){
    int num=anyToDec(a,n1);//先转10进制
    string r=DecToAny(num,n2);
    return r;
}
int main(){
#if 0
    string s[5]={"hello","world","yellow","red","blue"};
    sortStr(s,5,fun1);
    for(auto &n:s)
	cout<<n<<endl;

    sortStr(s,5,fun2);
    for(auto &n:s)
	cout<<n<<endl;
#endif
    string s("1011");
    auto k=anyToDec(s,2);
    cout<<k<<endl;

    int d=43981;
    auto h=DecToAny(d,16);
    cout<<h<<endl;

    auto f=AnyToAny(s,8,16);
    cout<<f<<endl;
    return 0;
}
