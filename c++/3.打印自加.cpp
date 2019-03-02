#include<iostream>

using namespace std;

int main(){
#if 0
    int i = 10;
    cout<<2+6<<endl;
    cout<<(i++,i++,i++)<<endl;//12
    cout<<i<<endl;//13

    //()里的运算从左到右
    i = 10;
    cout<<(++i,++i,i++)<<endl;//12
    i = 10;
    cout<<(++i,i++,i++)<<endl;//12
    i = 10;
    cout<<(i++,++i,++i)<<endl;//13
    i = 10;
    //先右计算到左，()里的运算是从左到右
    cout<<(++i,++i,i++)<<","<<(++i,i++,i++)<<endl;//15,12
    cout<<i<<endl;
#endif
    char str[20];
    char *p = str;
    cin>>str;
    cout<<str<<endl;
    cout<<p<<endl;
    cout<<(void *)(p+1)<<endl;
    cout<<*(p+1)<<endl;
    return 0;
}
