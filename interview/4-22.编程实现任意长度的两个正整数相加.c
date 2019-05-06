#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>

char *addBigInt(char *num1, char *num2){
    int c = 0;                  //进位，开始最低进位为0
    int i = strlen(num1) - 1;   //第一个加数最低位
    int j = strlen(num2) - 1;   //低二个加数最低位
    int maxLen = strlen(num1) > strlen(num2) ? i + 2 : j + 2; //得到两个位数最大的位数
    char *rst = (char *)malloc(maxLen + 1);
    if(rst == NULL){
        printf("malloc fail\n");
        exit(0);
    }

    rst[maxLen] = '\0';
    //int k = strlen(rst) - 1;//这里有问题，malloc向堆空间申请的内存，初始化全是\0
    int k = maxLen - 1;     //指向结果数组最低位
    while((i >= 0) && (j >= 0)){
        rst[k] =( (num1[i] - '0') + (num2[j] - '0') + c ) % 10 + '0';   //计算本位的值
        c = ( (num1[i] - '0') + (num2[j] - '0') + c ) / 10;             //向高位进位值
        --i;
        --j;
        --k;
    }
    while(i >= 0){  //若加数1位数大于加数2的位数
        rst[k] = ( (num1[i] - '0') + c ) % 10 + '0';
        c = ( (num1[i] - '0') + c ) / 10;
        --j;
        --k;
    }
    while(j >= 0){  //若加数2位数大于加数1的位数
        rst[k] = ( (num2[j] - '0') + c ) % 10 + '0';
        c = ( (num2[j] - '0') + c ) / 10;
        --j;
        --k;
    }
    rst[0] = c + '0';       //开头两位是预留位，若没有进位则第一位为0
    if(rst[0] != '0'){      //若结果最高位不是0，即最高位有进位，则输出结果
        return rst;
    }else{
        return rst + 1;
    }
}

int main(){
    char num1[] = "123456789323";
    char num2[] = "45671254563123";
    char *result = NULL;
    result = addBigInt(num1, num2);
    printf("%s + %s = %s\n", num1, num2, result);
    return 0;
}
