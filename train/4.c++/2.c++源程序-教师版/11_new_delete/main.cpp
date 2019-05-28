#include <iostream>
#include <stdlib.h>
using namespace std;

int main()
{
	//malloc free是c语言的函数
	int* p = (int*)malloc(sizeof(int));//申请空间
	*p = 32;
	free(p);
/*	
	int* arr = (int*)malloc(4*4);
	free(arr);只释放4个字节
*/
//	new delete;
	int n(45);

	//new 和 delete 是c++的运算符	
	int* q = new int(45);	//申请空间, 创造对象
	delete q;				//释放空间, 删除对象
	
	int* arr = new int[4]{1, 2, 3, 4};	//申请连续空间
	
	for (int i=0; i<4; i++)
		cout << arr[i] << endl;
	delete [] arr;			//释放连续空间
	return 0;
}
练习:
	定义一个结构体 : Student {id, name};
	在堆区申请四个连续的空间，存储四个学生信息
	显示学生信息
	最后释放空间
