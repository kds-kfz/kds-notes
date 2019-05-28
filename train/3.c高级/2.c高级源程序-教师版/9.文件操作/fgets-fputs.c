#include <stdio.h>
int main()
{
//    stdout
  //  stdin
   //  int fputs(const char *s, FILE *stream);
   //   char *fgets(char *s, int size, FILE *stream);
     FILE* fp = fopen("1.txt","w+");
     char* s = "hello";
     char a[1024] = "hello\nworld";
     fputs(a,fp);
     printf ("%ld\n",ftell(fp));
     rewind(fp);
//长度n　读到文件结束或者遇见\n结束　读取\n自动加\0
//长度n 　只能读取n-1个字符加\0
//想读取n个字符　长度写n+1
     s = fgets(a,100,fp);
     printf ("a = %s\n",a);
     printf ("s = %s\n",s);
     printf ("%ld\n",ftell(fp));
     fclose(fp);

    return 0;
}
