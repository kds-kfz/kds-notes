#include"kfile.hpp"
#include"pubfun.hpp"

using namespace std;
using namespace kfz;

int main(int argc, char *argv[]){
#if 1
    //文件测试
    Files fd;
    fd.Open("1.txt", O_RDONLY);
    cout<<"size:"<<fd.Size()<<endl;
    cout<<"date:"<<fd.Date();
    fd.Write("./","2.txt","hello");
    
    //字符串测试
    String str;
    char str_1[128] = {0};
    char str_2[128] = "hello world pp";
    cout<<"转换结果:"<<str.StringMateMore(str_2, " ")<<endl;
#endif
    return 0;
}
