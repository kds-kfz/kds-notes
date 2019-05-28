#include<stdio.h>
//4.c
//二进制文件操作, 学习4 
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb,FILE *stream);
//ptr：任意类型
//size：任意数据类型的字节个数
//nmemb：写入的个数 

int main(){
    FILE *fp=fopen("1.dat","wb+");
    int a[10]={1,2,3,4,5,6,7,8,9,10};
    fwrite(a,4,10,fp);

    rewind(fp);
    int b[10]={0};
    fread(b,40,1,fp);
    int i=0;
    for(;i<10;i++)
	printf("%d ",b[i]);
    putchar(012);
    return 0;   
}
