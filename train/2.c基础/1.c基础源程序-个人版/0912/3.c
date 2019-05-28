#include"stdio.h"
#include<string.h>
//4.c
//比较两个字符串大小(个人版)
int cmp_str(const char *p1,const char *p2){
    int i=0;
    for(;p1[i]==p2[i]&&p1[i]!='\0'&&p2[i]!='\0';i++);
    //不相等或只要有1个为\0就结束
    if(p1[i]>p2[i])
	//	return p1[i]-p2[i];
	return 1;
    else if(p1[i]<p2[i])
	//	return p1[i]-p2[i];
	return -1;
    else
	return 0;
}
//比较两个字符串大小(教师版)
int cmp_str1(const char *p1,const char *p2){
    while(*p1==*p2&&*p2!='\0'){//只要有一个为\0或同时相等结束
	p1++;
	p2++;
    }
    if(*p1=='\0'&&*p2=='\0')
	return 0;
    else if(*p1>*p2)
	return 1;
    else
	return -1;
}

/*
   double fun(char *p){
   int i=0,j=0;
   int arr[10]={0};
   double data=0;
   for(;*p!='\0';p++,i++)
   arr[i]=*p-48;
   arr[i]='\0';
   data=arr[0]*100+arr[1]*10+arr[2]+arr[4]*0.1+arr[5]*0.01;

   return data;
   }
 */
//递归函数，返回a^b的值
double fun2(float a,int b){
    if(b>0){
    if(b==1)
	return a;
    else
	return a*fun2(a,b-1);
    }
    else 
	return 0;
}
//个人版
//形参：一个指向字符串数字常量的指针，
//返回：double data
double fun(char *p){
    int i=0,j=0,k=0;
    char arr[30]={0};
    double data=0;
    for(;*p!='\0';p++,i++)
	arr[i]=*p;
    arr[i]='\0';
    
    i=0;
    if(arr[i]!='-'){//字符位正数
    //统计字符.前整数部分位数
    for(i=0;arr[i]<='9'&&arr[i]>='0'&&arr[i]!='.';i++,k=i);
    //先算个位
    data+=arr[i-1]-48;
    //个位前求和
    for(i-=1;i>0;i--,j++)
	data+=(arr[j]-48)*fun2(10,i);
    //小数部分求和
    for(k+=1,j=k;arr[k]!='\0';k++)
	data+=(arr[k]-48)*fun2(0.1,k-j+1);
    return data;
    }
    else{//字符为负数
    for(i=1;arr[i]<='9'&&arr[i]>='0'&&arr[i]!='.';i++,k=i);
    data+=arr[i-1]-48;
    for(i-=2,j=1;i>0;i--,j++)
	data+=(arr[j]-48)*fun2(10,i);
    for(k+=1,j=k;arr[k]!='\0';k++)
	data+=(arr[k]-48)*fun2(0.1,k-j+1);
    return -data;
    }
}
//个人版
double fun1(char *p){
    double data=0;
    int i=0,j=0,k=0;
    char *p1=p;
    while(*p!='\0'){
	i++;
	if(*p=='.'){
	    j=i;
	    i+=1;
	}
	p++;
    }
    
    j-=2;
    i=j;
    data+=*(p1+j)-48;
    for(k=j,j-=1;j>=0;j--)
	data+=(*(p1+j)-48)*fun2(10,k-j);
    for(i+=2;*(p1+i)!='\0';i++)
	data+=(*(p1+i)-48)*fun2(0.1,i-3);

    return data;
}
//教师版
double fun3(const char *p){
    int i=0;
    int flag=0;
    double data=0;
    double d=0;
    double m=0.1;
    
    for(;p[i]!='\0';i++){
	if(p[i]>='0'&&p[i]<='9'&&flag==0){
	    data*=10;
	    data+=p[i]-'0';
	}
	else if(p[i]=='.')
	    flag=1;
	else if{
	    d=(p[i]-'0')*m;
	    data+=d;
	    m*=0.1;
	}
	}
    i=0;
    if(p[i]!='-')
	return data;
    else
	return -data;
}

//字符串比较，相等返回0,不等返回插值，a>b正，a<b负
int main(){
    /*   
	 int value=0;

	 char a[100];
	 char b[100];

	 scanf("%s",a);
	 scanf("%s",b);

	 value=cmp_str1(a,b);
	 printf("value=%d\n",value);
	 if(value>0)
	 printf("a>b\n");
	 else if(value<0)
	 printf("a<b\n");
	 else
	 printf("a==b\n");
     */
    char *p="-12565463.633456";

    //    printf("%lf\n",fun2(0.1,2));

    printf("data=%lf\n",fun3(p));
    
//    printf("double=%ld\n",sizeof(long double));
    return 0;
}
