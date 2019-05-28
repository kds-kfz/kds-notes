//str.c
//从键盘输入两个字符串，从mian输入
//fun函数返回两个字符串中较长的一个字符串，相等随便

//1.算1个字符串长度
int my_strlen(const char *arr){
    int i=0;
    for(;arr[i];i++);
    return i;
}
/*
void str_input(char arr[],int n)
{
}
*/
//2.返回一个较长的字符串地址
const char * compare_str(const char *p1,const char *p2){
//    return my_strlen(p1)>my_strlen(p2)?p1:p2;
    if(my_strlen(p1)>my_strlen(p2))
	return p1;
    else if(my_strlen(p1)==my_strlen(p2))
	return p1;
    else
	return p2;
}

//3.复制字符串p2-->p1
char *copy_str(char *p1,const char *p2){
/*
    int i=0;
    for(;*(p2+i)!='\0';i++)
	*(p1+i)=*(p2+i);
    *(p1+i)='\0';
    return p1;
    */
    char *p=p1;
    while(*p1++=*p2++);
    return p;
}

//4.字符串拼接p2-->p1
char *cat_str(char *p1,char *p2){
/*
    int i=0,j=0;
    for(;*(p1+i)!='\0';i++);
    for(;*(p2+j)!='\0';j++)
	*(p1+i+j)=*(p2+j);
    *(p1+i+j)='\0';
    return p1;
    */
    char *p=p1;
    while(*p1++);
    p1--;
    while(*p1++=*p2++);
    return p;

}



