#include <iostream>
using namespace std;

template <typename T>
T Max(T a, T b)
{
	return a > b ? a : b;
}
template <typename T>
T Max(T* a, int n)
{
	T m = a[0];
	for (int i=1; i<n; i++) {
		m = m > a[i] ? m : a[i];
	}
	return m;
}
//练习: 写一个交换函数模板 Swap
//		再写一个Swap能交换一个数组中两个位置的值

//int* const p

template <typename T>
void Swap(const T & a, const T & b) // T=int*
{
	cout << "& " << endl;
	auto t = *a;
	*a = *b;
	*b = t;
}

template <typename T>
void Swap(T* a, T* b)
{
	cout << "* " << endl;
	T t = *a;
	*a = *b;
	*b = t;
}


template <typename Type>
void Swap(Type* arr, int n1, int n2)
{
	Type t = arr[n1];
	arr[n1] = arr[n2];
	arr[n2] = t;
}

int main()
{
	int a = 10, b = 20;
	int arr[4] = {4, 2, 1, 6};
	cout << Max(2, 3) << endl;

	cout << Max(arr, 4) << endl;
	
	Swap(&a, &b);	// int* const& p = &a;

	Swap(arr, 1, 2);
	cout << "a = " << a << 
		", b = " << b << endl;
	for (auto m : arr) 
		cout << m << " ";
	cout << endl;
	return 0;
}
