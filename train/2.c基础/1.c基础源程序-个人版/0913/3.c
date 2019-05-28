#include<stdio.h>
//3.c
//结构体字节大小，字节对齐，按成员变量最大的对齐
//成员变量有double，32位机double占8字节，其余成员占4字节
struct stu{//2
    char a;
    char b;
};
struct stu1{//8
    char a;
    int b;
};
struct stu2{//12
    char a;
    int b;
    char c;
};
struct stu3{//8
    int a;
    char b;
    char c;
};
struct stu4{//12
    int a;
    char b[7];
    char c;
};
struct stu5{//16
    double a;
    int b;
    char c;
};
struct stu6{//16
    double a;
    int  b;
    char c[3];
    char d;
};
struct stu7{//16
    int  b;
    char c[3];
    char d;
    double a;
};
struct stu8{//16
    char *a;
    char b[3];
    char c;
};
struct stu9{//16
    int  *a;
    char b[3];
    char c;
};
struct stu10{//6
    short  a;
    char b[3];
    char c;
};
int main(){
    
    printf("stu : %ld\n",sizeof(struct stu));
    printf("stu1 : %ld\n",sizeof(struct stu1));
    printf("stu2 : %ld\n",sizeof(struct stu2));
    printf("stu3 : %ld\n",sizeof(struct stu3));
    printf("stu4 : %ld\n",sizeof(struct stu4));
    printf("stu5 : %ld\n",sizeof(struct stu5));
    printf("stu6 : %ld\n",sizeof(struct stu6));
    printf("stu7 : %ld\n",sizeof(struct stu7));
    printf("stu8 : %ld\n",sizeof(struct stu8));
    printf("stu9 : %ld\n",sizeof(struct stu9));
    printf("stu10 : %ld\n",sizeof(struct stu10));

    return 0;
}
