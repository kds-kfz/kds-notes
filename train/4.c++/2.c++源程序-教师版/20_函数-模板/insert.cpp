#include <iostream>
using namespace std;
//练习:写一个排序的模板，使用直接插入法排序
/*
   void Sort(int* a, int n);
   void Sort(string* a, int n);
1 | 5 2 3
j   i
1 5 | 0 3
j     i
1  2 3 |5
     j  i
1  2 3	5
t = 3
 */
/*
   template <typename T>	// T == int*
   void Sort(T a, int n)	// T == string*
   {
//	auto m = a[0];
}
 */
	template <typename T>	// T == int / string
void Sort(T* a, int n)	
{
	T t;
	int j;
	for (int i=1; i<n; i++)
	{
		t = a[i];
	//	t<a[j] && j>=0 如果j=-1,可能会有段错误
		for (j=i-1; j>=0 && t<a[j]; j--)
		{
	//		if (t < a[j]) {
				a[j+1] = a[j];
	//		} else 
	//			break;		
		}
		a[j+1] = t;
	}
}

int main()
{
	int a1[4] = {1, 5, 2, 3};
	string a2[4] = {"hello", "my", "fiend", "Tom"};
	Sort(a1, 4);
	for (auto m: a1)
		cout << m << " ";
	cout << endl;
	Sort(a2, 4);
	for (auto m: a2)
		cout << m << " ";
	cout << endl;

	return 0;
}
