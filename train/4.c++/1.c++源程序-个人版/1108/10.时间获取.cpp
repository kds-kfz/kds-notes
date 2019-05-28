#include<iostream>
#include<unistd.h>
//10.获取时间.cpp
using namespace std;
int main(){
    time_t t;//long int
    time(&t);//会获得1个从1900-1-1 00：00：00
	    //到现在的所有秒数
    cout<<t<<endl;
    sleep(2);
    time_t t2;//long int
    time(&t2);
    cout<<t2<<endl;

    //计算两个时间差t-t2
    cout<<difftime(t,t2)<<endl;

    struct tm *showTime;
/*
    {
	tm_year;//年+1900
	tm_mon;//月+1
	tm_yday;//本年到现在的天数
	tm_mday;//本月
	tm_wday;//本周
	tm_hour;//小时
	tm_min;//分
	tm_sec;//秒
    }
 */
    showTime =localtime(&t);//取本地时间
//    showTime =gmtime(&t);//取世界时间

    cout<<showTime->tm_year+1900<<"/"
	<<showTime->tm_mon+1<<"/"
	<<showTime->tm_mday<<" "
	<<showTime->tm_hour<<":"
	<<showTime->tm_min<<":"
	<<showTime->tm_sec
	<<endl;

    return 0;
}
