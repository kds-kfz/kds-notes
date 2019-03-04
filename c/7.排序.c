#include<stdio.h>

#define Size(x) sizeof(x)/sizeof(*x)

static void bubble_sort(int *p, const int n);
static void select_sort(int *p, const int n);
static void insert_sort(int *p, const int n);
static void output(int *p, const int n);
static void Rand(int *p,int n);

int main(){
    int arr[10] = {0};
    Rand(arr, Size(arr));
    //bubble_sort(arr, Size(arr));
    //select_sort(arr, Size(arr));
    insert_sort(arr, Size(arr));
    output(arr, Size(arr));
    return 0;
}

static void bubble_sort(int *p, const int n){
    int i,j,flag,temp;
    for(i = 0; i < n - 1; i++){
        flag = 0;
        for(j = 0; j < n-1-i; j++){
            if(*(p+j) > *(p+j+1)){
                flag = 1;
                temp = *(p+j);
                *(p+j) = *(p+j+1);
                *(p+j+1) = temp;
            }
        }
        if(!flag) break;
    }
}

static void select_sort(int *p, const int n){
    int i,j;
    for(i = 0; i < n - 1; i++){
        int max = *p;
        int mid = 0;
        for(j = 1; j < n - i; j++){
            if(max < *(p+j)){
                max = *(p+j);
                mid = j;
            }
        }
        if(mid != n-1-i){
            *(p+mid) = *(p+n-1-i);
            *(p+n-1-i) = max;
        }
    }
}

static void insert_sort(int *p, const int n){
    int i,j,temp;
    for(i = 1; i < n; i++){
        temp = *(p+i);
        for(j = i; j > 0 && temp < *(p+j-1); j--){
            *(p+j) = *(p+j-1);
            *(p+j-1) = temp;
        }
    }
}

static void output(int *p, const int n){
    int i = 0;
    for(i = 0; i < n; i++)
        printf("%d ", *(p+i));
    putchar(10);
}

static void Rand(int *p,int n){
    int i = 0;
    //unsigned)time(NULL) 伪随机数起始值
    //若没有srand，则从1开始，以为着每次伪随机数都一样
    srand((unsigned)time(NULL));
    for(;i < n; i++){
        //*(p+i) = rand()%6+1;
        *(p+i) = rand()%1000;
    }
}
