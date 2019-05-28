#include <stdio.h>
int main()
{
    size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
    size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
    //1.写入/读取数据的地址　
    //2.写入/读取的数据类型的字节个数
    //3.写入/读取的个数
    int a = 30;
    float f = 3.14f;
    int a1[5] = {1,2,3,4,5};
    struct st{
	int num;
	char name[20];
    }s[2]={{1001,"zhang"},{1002,"wang"}};
    FILE* fp = fopen("1.txt","wb+");
    fwrite(&a,sizeof(int),1,fp);
    fwrite(&f,4,1,fp);
    //fwrite(a1,sizeof(int),5,fp);
    fwrite(a1,sizeof(a1),1,fp);
    //fwrite(s,sizeof(struct st),2,fp);
    fwrite(s,sizeof(s),1,fp);
    fseek(fp,-sizeof(s),SEEK_CUR);

    struct st  s2 = {0};
    fread(&s2,sizeof(struct st),1,fp);
    printf ("%d %s\n",s2.num,s2.name);

    fseek(fp,-(sizeof(struct st)+sizeof(a1)),SEEK_CUR);

    int b[5] = {0};
 //   fread(b,sizeof(int),5,fp);
    fread(b,sizeof(b),1,fp);
    printf ("%d %d %d %d %d\n",b[0],b[1],b[2],b[3],b[4]);
    fclose(fp);
    return 0;

}
