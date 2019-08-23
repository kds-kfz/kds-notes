#include"klog.hpp"

using namespace std;
using namespace kfz;

int main(int argc, char *argv[]){
#if 1
    //文件测试
    Files fd;
    fd.Open("3.txt", O_RDONLY);
    cout<<"size:"<<fd.Size()<<endl;
    cout<<"date:"<<fd.Date();
    fd.Write("./","4.txt");
    
    fd.Open("5.txt", O_APPEND);
    fd.Write("world!");


#endif
    
    //字符串测试
    String str;
    char str_1[128] = {0};
    char str_2[128] = "hello world pp";
    cout<<"转换结果:"<<str.StringMateMore(str_2, " ")<<endl;
    
    //Date time1;
    //cout<<time1.GetTime("%Y%m%d-%H%M%S.log")<<endl;

    Log log1("../log/mount-service");
    log1.logMsg(TC, INFO, "%s:%d", __FILE__, __LINE__);
    return 0;
}
