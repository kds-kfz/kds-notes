//文件复制
#include <stdio.h>
int main()
{
    FILE* fp = fopen("3.txt","r");
    FILE* op = fopen("4.txt","w");
    char ch = 0;
    if (fp != NULL)
    {
	while((ch = fgetc(fp)) != -1)
	    fputc(ch,op);
	printf ("复制成功\n");
	fclose(fp);
	fclose(op);
    }
    else
    {
	printf ("打开失败\n");
	return 0;
    }
}
