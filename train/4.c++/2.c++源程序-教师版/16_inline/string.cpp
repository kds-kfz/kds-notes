#include <iostream>
using namespace std;

using pFun = bool (*)(string, string);
bool cmpStr(string a, string b)
{
	return a > b;
}
bool cmpSize(string a, string b)
{
	return a.size() > b.size();
}

void sort(string* arr, int n, pFun p)
{
	string t;
	for (int i=0; i<n-1; i++) {
		for (int j=0; j<n-1-i; j++) {
			if (p(arr[j], arr[j+1])) {
				t = arr[j];
				arr[j] = arr[j+1];
				arr[j+1] = t;
			}
		}
	}
}
int main()
{
	string arr[5] = {"hello", "world", 
		"an", "apple", "banana"};
	sort(arr, 5, cmpSize);
	
	for (auto m : arr) {
		cout << m << endl;
	}

	return 0;
}
