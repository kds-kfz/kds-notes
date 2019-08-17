#include"pubfun.hpp"

using namespace std;
using namespace kfz;

int main(int argc, char *argv[]){
    Files fd;
    fd.Open("1.txt", O_RDONLY);
    cout<<"size:"<<fd.Size()<<endl;
    cout<<"date:"<<fd.Date();
    fd.Write("./","2.txt","hello");
    
    String str;
    char str_1[128] = {0};
    char str_2[128] = "hello world pp";
    cout<<"转换结果:"<<str.StringMateMore(str_2, " ")<<endl;
    return 0;
}
