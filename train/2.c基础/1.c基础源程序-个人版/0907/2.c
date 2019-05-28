#include"stdio.h"
//2.c
int main(){
char ch[127];
char a;
char *p=NULL,*q=NULL;
int i=0;
while((ch[i]=getchar())!='\n')
    i++;
for(p=ch,q=&ch[i-1];p<q;p++,q--){
    a=*p;*p=*q;*q=a;}
for(p=ch;p<ch+i;p++)
    printf("%3c",*p);
printf("\n");
return 0;
}

int main3(){
char ch[127];
char a;
char *p=NULL,*q=NULL;
int i=0;
while((a=getchar())!='\n')
    ch[i++]=a;
for(p=ch,q=&ch[i-1];p<q;p++,q--)
    if(*p!=*q)break;
if(p<q)printf("no\n");
else printf("yes\n");
return 0;
}

//定义字符数组长度，输入字符，并输出倒序，12345---->54321
int main2(){
int i;
char c;
int n;
printf("input n bit charater:\n");
scanf("%d",&n);
char arr[n];
char *p1=arr;
//char *p2=&arr[n-1];
getchar();
for(i=0;i<n;i++)
    scanf("%c",p1+i);
for(i=0;i<n/2;i++){
//    c=*(p2-i);*(p2-i)=*(p1+i);*(p1+i)=c;}
    c=*(p1+n-1-i);*(p1+n-1-i)=*(p1+i);*(p1+i)=c;}
for(i=0;i<n;i++)
    printf("%c",arr[i]);
printf("\n");
return 0;
}

//输入5个字符，判断是否是回数，asdsa:yes  asdas:no
int main1(){
char arr[5]={'b','a','b','a','a'};
int i,j;
char *p1=arr;
j=0;
printf("input 5bit charater:\n");
for(i=0;i<5;i++)
    scanf("%c",p1+i);
for(i=0;i<5/2;i++){
    if(*(p1+i)==*(p1+5-1-i))j=1;
    else j=0;break;}
if(j==1)printf("yes\n");
else printf("no\n");
return 0;
}

