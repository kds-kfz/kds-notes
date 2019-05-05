#include<iostream>

using namespace std;

void spwd_1(int &x, int &y){
    cout<<"前 : x = "<<x<<", y = "<<y<<endl;
    x += y;
    y = x - y;
    x = x - y;
    cout<<"后 : x = "<<x<<", y = "<<y<<endl;
}
#if 0
    6: 0000 0000 0000 0000 0000 0000 0000 0110
    9: 0000 0000 0000 0000 0000 0000 0000 1001
    6^9: 0000 0000 0000 0000 0000 0000 0000 1111
#endif
void spwd_2(int &x, int &y){
    cout<<"前 : x = "<<x<<", y = "<<y<<endl;
    x ^= y;
    y ^= x;
    x ^= y;
    cout<<"后 : x = "<<x<<", y = "<<y<<endl;
}

int main(){
    int a = 6, b = 9;
    spwd_1(a, b);
    spwd_2(a, b);
    return 0;
}
