#include<stdio.h>
//3.c
//优先级
struct st{
    int num;
};

int main(){
    struct st a[5]={100,200,300,400,500};
    struct st *p=a;
    
    // () . [] -> * ++ --
    //++或者--出现在变量右边，优先级最低，出现在左边，从右往左计算

    printf("%d %d %d %d\n\n",a[2].num,p[2].num,(*(p+2)).num,(p+2)->num);
    
    printf("p : %p\n\n",p);

    int num=(*p++).num;//先取值，地址加1
    printf("num=%d\n",num);
    printf("p : %p,a[1]_addr=%p\n\n",p,&a[1].num);

    num=++p->num;//先取值，值加1
    printf("num=%d,a[1].num=%d\n",num,a[1].num);
    printf("p : %p,a[1]_addr=%p\n\n",p,&a[1]);

    num=(*++p).num;//地址先加1,再取值
    printf("num=%d,a[2].num=%d\n",num,a[2].num);
    printf("p : %p,a[2]_addr=%p\n\n",p,&a[2]);

    num=++(*p).num;//先取值，值加1
    printf("num=%d\n",num);
    printf("p : %p\n",p);

    return 0;
}
