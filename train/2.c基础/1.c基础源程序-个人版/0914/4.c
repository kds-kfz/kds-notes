#include"stdio.h"
//4.c
//共用体
union gong{
    char a;
    char b;
}a1;

union gong1{
    int a;
    char b;
}a2;

union gong2{
    char a[2];
    int b;
};

union yong{
    char a;
    int b;
    float c;
    double d;
};

int main(){
    union gong s1={'A'};
    printf("s1.a=%c,s1.b=%c\n\n",s1.a,s1.b);

    a1.a='a';
    printf("a1.a=%c,a1.b=%d\n\n",a1.a,a1.b);

//0000 0000 0000 0000 0000 0000 0000 0000
//0000 0000 0000 0000 0000 0001 0010 1100
    //300=256+32+8+4
    //1字符低8位，44,字符','
    a2.a=300;
    printf("a2.a=%c,a2.b=%d\n\n",a2.a,a2.b);


//0000 0000 0000 0000 0011 0000 0000 1101
    //48=32+16
    //13=8+4+1
    //10进制:12301
    union gong2 a3={0};
    a3.a[0]=13;
    a3.a[1]='0';
    printf("a3.b=%d\n\n",a3.b);

//    union yong s2={65.89};
    union yong s2={0};
    s2.c=65;
    s2.d=65.89;
    printf("s2.a=%c,s2.b=%d,s2.c=%f,s2.d=%lf\n\n",s2.a,s2.b,s2.c,s2.d);

    return 0;
}
