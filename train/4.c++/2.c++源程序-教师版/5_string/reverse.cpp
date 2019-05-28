#include <iostream>

using namespace std;

int main()
{
//	string t = s.substr(i, 3); r = t-->你 + r;
	
//s	//97 98 99 100 32 | -28 -67 -96 |-27 -91 -67
//	                     i  i+1  i+2
//				     j-2 j-1 j
//r	//-27 -91 -67 | -28 -67 -96 | 32 100 99 98 97

	string s = "abcd 你好 1234xyz 今天很凉爽";
	string n;
/*	n += -28;
	n += -67;
	n += -96;
	cout << n << endl;
*/
	string r;

	int len = s.size();
	for (int i=0; i<len; i++) {
		 if (s[i] >= 0 && s[i] <= 127) {
			r = s[i] + r;
		 } else {
			n = s.substr(i, 3);
			//将n拼接到r的前面
			r = n + r;
			i += 2;
		 }
	}
/*	
	r.resize(len);
	for (int i=0, j=len-1; i<len; i++, j--) {
		if (s[i] >= 0 && s[i] <= 127) {
			r[j] = s[i];
		} else {
			r[j-2] = s[i];
			r[j-1] = s[i+1];
			r[j] = s[i+2];

			i += 2;
			j -= 2;
		}
	}
*/
	cout << r << endl;

	return 0;
}

