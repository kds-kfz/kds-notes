#include <stdio.h>
int main()
{
   // FILE *fopen(const char *path, const char *mode);
   // int fclose(FILE *fp);
   // lo0ng ftell(FILE *stream);//文件指针距离文件开头的长度
//    EOF end of file (-1)
  //  EOF ctrl d

    FILE* fp = fopen("5.txt","r");
    if(fp == NULL)
	printf ("文件打开失败\n");
    else
    {
	printf ("成功\n");
	long int len = ftell(fp);
	printf ("len = %ld\n",len);
	 fclose(fp);
    }
    return 0;
}
