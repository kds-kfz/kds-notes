#include<stdio.h>
//3.c
//文件操作，学习3

//文件描述符
//标准输入 0 STDIN_FILENO
//标准输出 1 STDOUT_FILENO
//标准错误 2 STDERR_FILENO
//文件指针指向V节点，不同文件指针V节点不同，在文件描述符中3代表文件指针
//(目前认识)

typedef struct{
    int num;
    char sex;
    char name[20];
    float score;
}ST;
int main(){
    ST a[10]={0};
    FILE *fp=fopen("1.doc","r");
    int i=0;
    int ret=0;
    if(fp!=NULL){
	printf("read 1.doc success\n");
	while(1){
	ret=fscanf(fp,"%d %c %s %f",
		&a[i].num,&a[i].sex,a[i].name,&a[i].score);
	if(ret!=4)
	    break;
	i++;
	}
	fclose(fp);
    }
//    long int len=ftell(fp);//此时文件关闭，文件指针不指向文件，返回-1
//    printf("len=%ld\n",len);
    FILE *fp1=fopen("2.doc","w");//不同的文件指针v节点不同，文件描述符相同
    int j=0;
    for(;j<i;j++)
	fprintf(fp,"%d %c %s %.2f\n",
		a[j].num,a[j].sex,a[j].name,a[j].score);
//    long int len1=ftell(fp1);
//    printf("len1=%ld\n",len1);
    fclose(fp1);
    
    FILE* fp2 = fopen("3.doc","w");
    j=0;
    for(;j<i;j++)
	fprintf(fp,"%d %c %s %.2f\n",
		a[j].num,a[j].sex,a[j].name,a[j].score);
    fclose(fp2);
    return 0;
}
