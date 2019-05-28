#include <stdio.h>
int main()
{
   // int  fprintf(FILE  *stream,  const  char  *format, ...);
   // int fscanf(FILE *stream, const char *format, ...);

    int a[50] = {3,4,5,6,7};
    char ch = 'A';
    float f = 3.14f;
    FILE* fp = fopen("1.txt","r+");
//    fprintf(fp,"%c %.3f\n",ch,f);
 //   fprintf(fp,"%d %d %d %d %d\n",a[0],a[1],a[2],a[3],a[4]);
    rewind(fp);
    fseek(fp,0,SEEK_SET);
    SEEK_SET  0
    SEEK_CUR  1
    SEEK_END  2
    fscanf(fp,"%c%f",&ch,&f);
    printf ("%c %.3f\n",ch,f);
    printf("%ld\n",ftell(fp));
    fseek(fp,5,SEEK_CUR);
    int n = 0;
    fscanf(fp,"%d",&n);
    printf ("n=%d\n",n);
    fseek(fp,-5,SEEK_CUR);
    int i = 0;
    int ret = 0;
    while(1)
    {
	ret = fscanf(fp,"%d",&a[i]);
	if (ret == 1)
	    i++;
	else
	    break;
    }
    int j = 0;
    for(;j<i;j++)
	printf ("%d ",a[j]);
    printf ("\n");
    fclose(fp);




/*    int i = printf("%d %d %d\n",1,2,3);
    printf ("i = %d\n",i);
    int a = 0;
    int b = 0;
    int c = 0;
    i = scanf("%d %d %d",&a,&b,&c);
    printf ("i = %d\n",i);
*/
    return 0;
}
