#include<iostream>
#include<string>

using namespace std;

string trim(string& str, const string& trimStr /* = " \r\n\t" */)
{
    size_t len = str.length();
    size_t begin = 0,end=len;

    while(begin < len)
    {
        if (trimStr.find(str[begin]) == string::npos)
            break;
        ++begin;
    }
    while(end - 1 > 0 && end - 1 > begin)
    {
        if (trimStr.find(str[end - 1]) == string::npos)
            break;
        --end;
    }
    cout<<"begin = "<<begin<<" end = "<<end<<endl;
    str = str.substr(begin,end - begin);
    return str;
}

int main(){
    string tmp = "   hello   =world   ";
    cout<<trim(tmp, " \r\n\t")<<endl;
    return 0;
}
