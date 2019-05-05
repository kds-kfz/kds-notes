#include<iostream>
#include<cstring>
#include<stdio.h>
#include<stdlib.h>
using namespace std;
#if 0
    华为软件中心--面试题目2
    现在和外星人打交道，发现外星人使用的并非10进制/16进制等，
    有些星球居然使用n进制(根据统计n都在2-35之间)，一下的1001个题目中我们将首先
    给您一个数字表示n进制，然后给出两个数字的字符串，请计算其求和结果并输出
    数字的字符串其中包括0-9和a-z(表示10-35)
    -------------------------------------------------------------------
    详情描述如下：
    int Calc(int n, const cahr *num1, const char *num2, char *result);
    输入参数：
    n：n进制
    num1：第一个数字，加号左边的数字
    num2：第二个数字，加号右边的数字
    result：得数
    返回值：
    int：如果计算成功返回0，否则返回-1
    -------------------------------------------------------------------
    注明：
    1、契约1：所有的输入参数和输出都只会是小写字母或数字，不存在其他字符
    2、契约2：result的内存外部申请和释放
    3、契约3：如果任何一个字符串为空或者空字符串都是非法，不得在计算result
#endif

#define  max 100
int trans(char a){
    if(a >= 'a' && a <= 'z')
        return a - 87;
    else if(a >= '0' && a <= '9')
        return a - 48;
    else
        return -1;
}

char rev_trans(int num){
    if(num >= 0 && num <= 9)
        return char(num + 48);
    else if(num >= 10 && num <= 36)
        return char(num + 87);
    else
        return '#';
}

int bigger(int a, int b){
    return a > b ? a : b;
}

int main(){
    int N,position;
    bool flag = false;
    char str1[max], str2[max];
    int num1[max], num2[max];
    cin >> N;
    getchar();
    cin >> str1;
    cin >> str2;
    position = 0; flag = false;
    for(int i = 0; i < max; i++)
        num1[i] = num2[i] = 0;
    for(int i = 0; i < strlen(str1); i++)
        num1[i] = trans(str1[strlen(str1) - 1 - i]);
    for(int i = 0; i < strlen(str2); i++)
        num2[i] = trans(str2[strlen(str2) - 1 - i]);
    if(N < 2 || N > 35){
        cout << -1 <<endl;
        exit(0);
    }
    for(int k1 = 0; k1 < strlen(str1); k1++){
        if(num1[k1] >= N){
            cout << -1 <<endl;
            flag = true;
        }
    }
    for(int k2 = 0; k2 < strlen(str2); k2++){
        if(num1[k2] >= N){
            cout << -1 <<endl;
            flag = true;
        }
    }
    if(!flag){
        for(int i = 0; i < bigger(strlen(str1), strlen(str2)); i++){
            num1[i] = num1[i] + num2[i];
            if(num1[i] >= N){
                num1[i] = num1[i] % N;
                num1[i + 1]++;
            }
        }
        for(int i = bigger(strlen(str1), strlen(str2)); i >= 0; i--){
            if(num1[i] != 0){
                position = i;
                break;
            }
        }
        for(int i = position; i >= 0; i--)
            cout << rev_trans(num1[i]);
        cout << endl;
    }
    return 0;
}
