#include<stdio.h>
#include<string.h>
//2.c
struct student{
    char name[20];
    int age;
    char sex;
    int number;
    float score;
};

struct student s1={0};
struct student s2={"zhangsan",24,'m',1001,0};
struct student s3;

int main(){

    strcpy(s3.name,"lisi");
    s3.age=23;
    s3.sex='n';
    s3.number=1002;
    s3.score=98.78;

    printf("s1:%s\t%d\t%c\t%d\t%.2f\t\n",
	    s1.name,s1.age,s1.sex,s1.number,s1.score);
    printf("s2:%s\t%d\t%c\t%d\t%.2f\t\n",
	    s2.name,s2.age,s2.sex,s2.number,s2.score);
    printf("s3:%s\t%d\t%c\t%d\t%.2f\t\n",
	    s3.name,s3.age,s3.sex,s3.number,s3.score);

    return 0;
}
