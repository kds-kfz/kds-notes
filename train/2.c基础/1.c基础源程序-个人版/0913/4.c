#include"stdio.h"
//4.c
struct st{
int num;
char name[10];
char sex;
};

void copy_st(struct st a[],struct st b[]){
int i;
for(i=0;i<3;i++)
    b[i]=a[i];

printf("printf struct b[3]-----------\n");

for(i=0;i<3;i++)
    printf("%d %s %c\n",b[i].num,b[i].name,b[i].sex);
}

int main(){

struct st a[3]={0};
struct st b[3]={0};

int i;
for(i=0;i<3;i++)//注意输入格式，字符串和字符scanf遇到空格或回车才可以分开
    scanf("%d%s\t%c",&a[i].num,a[i].name,&a[i].sex);
    copy_st(a,b);

return 0;
}
