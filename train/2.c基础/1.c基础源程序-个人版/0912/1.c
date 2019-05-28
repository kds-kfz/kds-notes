#include"stdio.h"
//1.c
int main(){
    int i;
//    char ch[10];
//    setbuf(stdin,ch);
    
    char a[]={'a','b','c','\0'};
    char b[10]={"abc"};
    char c[10]="abc";
    char d[]="abc";
    char e[10]={'a','b','c'};
    char f[]={"abc"};

    char g[]={'a','b','c'};//不是字符串

    char h[]={'a','b','\0','c'};//

    for(i=0;i<10;i++)
	printf("%d ",e[i]);
    putchar('\n');

    printf("a=%ld\n",sizeof(a));
    printf("b=%ld\n",sizeof(b));
    printf("c=%ld\n",sizeof(c));
    printf("d=%ld\n",sizeof(d));
    printf("e=%ld\n",sizeof(e));
    printf("f=%ld\n",sizeof(f));
    printf("g=%ld\n",sizeof(g));
    printf("h=%ld\n",sizeof(h));

    printf("a= %s\n",a);
    printf("b= %s\n",b);
    printf("c= %s\n",c);
    printf("d= %s\n",d);
    printf("e= %s\n",e);
    printf("f= %s\n",f);//字符数组自身不带\0，打印遇到内存\0结束
    printf("g= %s\n",g);
    printf("h= %s\n",h);

//    putchar(65);
//    putchar('\101');
//    putchar(0x41);
//    putchar(0101);
    return 0;
}
