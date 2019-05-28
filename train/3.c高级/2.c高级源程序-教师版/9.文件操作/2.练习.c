#include <stdio.h>
typedef struct{
    int num;
    char sex;
    char name[20];
}ST;
int main()
{
    ST a[3] = {{1001,'M',"zhangsan"},
		{1002,'N',"lisi"},
		{1003,'M',"wangwu"}};
    FILE* fp = fopen("1.txt","w+");
    int i = 0;
    for(;i<3;i++)
	fprintf(fp,"%d %c %s\n",a[i].num,a[i].sex,a[i].name);
    
    ST s1 = {0};
//    fseek(fp,-26 ,SEEK_CUR);
    rewind(fp);
    fseek(fp,15,SEEK_CUR);
    fscanf(fp,"%d %c %s",&s1.num,&s1.sex,s1.name);
    printf ("%d %c %s\n",s1.num,s1.sex,s1.name);
    rewind(fp);
   //fseek(fp,0,SEEK_SET);
    ST b[3] = {0};
    for(i=0;i<3;i++)
	fscanf(fp,"%d %c %s",&b[i].num,&b[i].sex,b[i].name);

    for(i = 0;i<3;i++)
	printf("%d %c %s\n",b[i].num,b[i].sex,b[i].name);
    fclose(fp);
    return 0;
}
