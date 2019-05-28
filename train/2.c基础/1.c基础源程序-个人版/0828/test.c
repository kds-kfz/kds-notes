#include"stdio.h"
#include"string.h"

char str[50],c;
int num=0,word=0,i=0;
//输入句子，判断单词个数
int main(){
//scanf("%s",str);
gets(str);    
printf("%s\n",str);
for(i=0;(c=str[i])!='\0';i++){
if(c==' ') word=0;
if(word==0){word=1;num++;}
}
printf("this sentence has %d word\n",num);

return 0;
}














