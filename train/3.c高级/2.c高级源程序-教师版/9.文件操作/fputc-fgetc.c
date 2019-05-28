#include <stdio.h>
int main()
{
    int fgetc(FILE *stream);
    int fputc(int c, FILE *stream);

     FILE* fp = fopen("3.txt","w+");
     fputc(49,fp);
     fputc('\n',fp);
     fputc('2',fp);
     long int len = ftell(fp);
     printf ("len = %ld\n",len);
     rewind(fp);//移动文件指针到文件开头
     len = ftell(fp);
     printf ("len = %ld\n",len);

     char ch = fgetc(fp);
     printf ("ch = %d\n",ch);
     len = ftell(fp);
     printf ("len = %ld\n",len);
     ch = fgetc(fp);
     printf ("ch = %d\n",ch);

     len = ftell(fp);
     printf ("len = %ld\n",len);
     ch = fgetc(fp);
     printf ("ch = %d\n",ch);
     len = ftell(fp);
     printf ("len = %ld\n",len);

    fclose(fp);
}
