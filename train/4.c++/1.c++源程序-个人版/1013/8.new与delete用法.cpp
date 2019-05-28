#include<iostream>
#include<stdlib.h>
//8.cpp
using namespace std;

int main(){
#if 0
    int *p=(int *)malloc(sizeof(int));
    *p=32;
    free(p);
#if 0
    int *arr=(int *)malloc(4*4);
//    free(arr);//只释放4字节
    for(int i=0;i<4;i++)
        free(&arr[i]);
#endif
    //定义变量n并初始化为45
    int n(45);

    // 申请空间，创造对象
    int *q=new int(45);
    //释放空间，删除对象
    delete q;

    //new和delete是c++的运算符，不需要添加其它头文件
    //申请连续空间,并初始化
    int *arr=new int[4]{1, 2, 3, 4};
    /*
    for(int i=0;i<4;i++)
	cout<<arr[i]<<" ";
    cout<<endl;
    */
    for(int i=0;i<4;i++)
	cout<<arr[i]<<endl;
    //saipan连续空间
    delete []arr;
#endif
#if 1
    //练习：
    //定义一个结构体：Student{id,name}
    //在堆区申请4个连续的空间，存储4个学生信息
    //显示学生信息
    //最后释放 
    struct Student{
	int id;
	string name;
    };
   
    //第3个结构体中name是10个\0
    Student *d1=new Student[4]{{1001,"lisi"},{1002,"lisi"},
			       {1003},{1004,"lisi"}};
    //如果申请空间失败，(申请空间大于内存)直接中断
    //若申请空间大小合理，失败则返回空指针
    //申请成功返回指向空间首地址

    //打印1个Student结构体大小
    //不能打印申请的4个Student连续空加大小
    cout<<"Student_size="<<sizeof(Student)<<endl;

    for(int i=0;i<4;i++)
	cout<<d1[i].id<<","<<d1[i].name<<endl;

    delete []d1;
#endif
    return 0;
}
