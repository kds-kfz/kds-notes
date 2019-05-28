/*
   80个长度以内的字符串
   string s = "hello12world19apple76friend345";
   使用范围for循环，分成两个数组
   string a1[40] = "hello" .....
   int	a2[40] = 12, 19, ...

   使用模板对这两个数组排序, 使用选择排序

   输出两个数组, 使用函数重载
   show()
 */

#include <iostream>
using namespace std;
inline bool isNum(char c)
{
    return c >= '0' && c <= '9';
}
inline bool isChar(char c)
{
    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z';
}
/*
   4, 1, 5, 3, 2
   i应该找到的最大值下标 i = n-1; i > 0; i--
   maxIndex = 2
   4, 1, 2, 3, 5
   i
   int j=0; j<=i; j++
   maxIndex = 0;
   3, 1, 2, 4, 5
   i
   maxIndex = 0;
   2, 1, 3, 4, 5
   i
   maxIndex = 0;
   1, 2, 3, 4, 5
   i
 */
template <typename T>
void Swap(T& a, T& b)
{
    T temp = a;
    a = b;
    b = temp;
}

template <typename T>
void select(T* arr, int n)
{
    int maxIndex = 0;
    for (int i=n-1; i>0; i--) {
	maxIndex = 0;
	for (int j=1; j<=i; j++) {
	    if (arr[maxIndex] < arr[j]) 
		maxIndex = j;
	}
	Swap(arr[i], arr[maxIndex]);
    }
}
void show(string* a, int n) {
    for (int i=0; i<n; i++)
	cout << a[i] << " ";
    cout << endl;
}
void show(int* a, int n) {
    for (int i=0; i<n; i++)
	cout << a[i] << " ";
    cout << endl;
}

int main()
{
    string s = "hello123world119apple76friend345ww0oppo";
    //	f1 = 1	字母
    //	f2 = 1  数字
    string a1[40];
    int a2[40] = {};
    int i=0, j=0, f1=0, f2=0;
    string t;
    int num = 0;
    for (auto m : s) {
	if (isChar(m)) {
	    f1 = 1;
	    t += m;
	} else if (f1 == 1) {
	    a1[i++] = t;
	    t.resize(0);// t = "";
	    f1 = 0;
	}
	if (isNum(m)) {
	    f2 = 1;
	    num *= 10;
	    num += m - '0';
	} else if (f2 == 1) {
	    a2[j++] = num;
	    num = 0;
	    f2 = 0;
	}
    }
    if (t.size() > 0) {
	a1[i] = t;
    } else {
	a2[j] = num;
    }

    show(a1, i);
    show(a2, j);
    select(a1, i);
    select(a2, j);
    show(a1, i);
    show(a2, j);
    return 0;
}
