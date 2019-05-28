#include <stdio.h>
int main()
{
/*
   ^ 相同为0　不同为1
    任意数跟本身异或是0
    任意数跟0异或是本身
    异或没有顺序
*/
    printf ("%d\n",4^5^6);
    printf ("%d\n",6^4^5);
  //数字加密
    int n = 0;
    printf ("输入密码\n");
    scanf("%d",&n);

    int m = n ^ '#';
    printf ("加密后存储是%d\n",m);

    n = m ^ '#';
    printf ("n = %d\n",n);
//数据交换
    int a = 10;
    int b = 20;
    printf ("a =%d b=%d\n",a,b);
    int c = a;
    a = b;
    b = c;
    printf ("a =%d b=%d\n",a,b);
//加法
    a = a+b;  
    b = a-b;
    a = a-b;  
    printf ("a =%d b=%d\n",a,b);
//异或
    a = a^b;
    b = a^b;
    a = a^b;
    printf ("a =%d b=%d\n",a,b);

    








}        


	    

