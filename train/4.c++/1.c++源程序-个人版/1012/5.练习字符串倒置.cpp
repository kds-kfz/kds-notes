#include<iostream>
#include<stdio.h>
//5.cpp
using namespace std;

int main(){
#if 0
    string s1("hello\0world");
    cout<<s1<<endl;
    printf("s1=%s\n",&s1[0]);
    
    s1.resize(8);
    s1=s1+"world";
    //.c_str()
    //返回内部字符串的首地址
    printf("s1=%s\n",s1.c_str());

    const char *p=s1.c_str();
    printf("s1=%s\n",p);

    cout<<s1<<endl;
    for(int i=0;i<s1.size();i++){
	cout<<s1[i]<<" ";
    if(s1[i]=='\0')
	cout<<"# ";
    }
    cout<<endl;
//    *p='A';//错误,内容不可改变
//    cin>>s1;//s1大小可变,遇到空格或回车结束
    getline(cin,s1);//可接收空格,遇见回车结束

    cout<<s1<<endl;
#endif

//练习：字符串倒置
//hello 今天很热-->热很天金 olleh
//方法1:
#if 0
    string s2("hello 今天很热");
    string s3(s2.size(),'0');
    cout<<s2<<endl;
    int g=s2.size();
    for(int i=0;i<s2.size();i++){
    if(s2[i]>=0&&s2[i]<=127)
	s3[s2.size()-i-1]=s2[i];
    else{
	string s4=s2.substr(i,3);
	cout<<s4<<endl;
	i+=2;
	int k=s2.size()-i-1;
	s3[k] = s4[0];
	s3[k+1]=s4[1];
	s3[k+2]=s4[2];
	}
    }
    cout<<s3<<endl;
#endif
//方法2:
#if 0
    string s2("hello 今天很热");
    string s3(s2.size(),'0');
    string s4("000");
    cout<<s2<<endl;
    int i=0;
    int count=0;
    int g=s2.size();
    for(;i<g;i++){
	if(s2[i]>=0&&s2[i]<=127)
	    s3[g-i-1]=s2[i];
	else{
	    int j=i;
	    j+=2;
	    for(;i<=j;)
		s4[count++]=s2[i++];
	    count=0;
	    i--;
	    int k=g-i-1;
	    s3[k]=s4[0];
	    s3[k+1]=s4[1];
	    s3[k+2]=s4[2];
	}
    }
    cout<<s3<<endl;
#endif
//方法3:
#if 0
    string s2("hello 今天很热");
    string s3;
    cout<<s2<<endl;
    int g=s2.size();
    for(int i=g;i>=0;i--){
	if(s2[i]>=0&&s2[i]<=127)
	    s3+=s2[i];
	else{
	    int j=i-2;
	    string s4=s2.substr(j,3);
	    s3+=s4;
	    i-=2;
	}
    }
    cout<<s3<<endl;
#endif
//方法4:
#if 0
    string s2("hello 今天很热");
    string s3;
    cout<<s2<<endl;
    int g=s2.size();
    int k=0;
    for(int i=g;i>=0;i--){
	k=i;
	if(s2[i]>=0&&s2[i]<=127){
	    s3+=s2[i];
	}
	else{
	    k-=2;
	    string s4=s2.substr(k,3);
	    s3.insert(g-k-2,s4);
	    i-=2;
	}
    }
    cout<<s3<<endl;
#endif
//方法5:
#if 0
    string s2("hello 今天很热");
    string s3;
    int len=s2.size();
    s3.resize(len);
    cout<<s2<<endl;
    for(int i=0,j=len-1;i<len;i++,j--){
	if(s2[i]>=0&&s2[i]<=127){
//	    cout<<s2[i]<<" ascii "<<endl;
	    s3[j]=s2[i];
	}
	else{
//	    cout<<s2[i]<<s[i+1]<<s2[i+2]<<endl;
	    s3[j-2]=s2[i];
	    s3[j-1]=s2[i+1];
	    s3[j]=s2[i+2];
	    i+=2;
	    j-=2;
	}
    }
    cout<<s3<<endl;
#endif
//方法6:
#if 1
    string s2("hello 今天很热");
    string s3;
    string s4;
    int len=s2.size();
    s3.resize(len);
    cout<<s2<<endl;
    for(int i=0;i<len;i++){
	if(s2[i]>=0&&s2[i]<=127){
	    s3=s2[i]+s3;
	}
	else{
	    s4=s2.substr(i,3);
	    s3=s4+s3;
	    i+=2;
	}
    }
    cout<<s3<<endl;
#endif
    return 0;
}
