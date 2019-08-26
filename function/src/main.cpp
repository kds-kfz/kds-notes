#include"klog.hpp"

# ifndef LOG_MODULE
# define LOG_MODULE "LOG "
# endif

using namespace std;
using namespace kfz;

int main(int argc, char *argv[]){
#if 0
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
    cout<<"转换结果:"<<str.StringMateMoreLast(str_2, " ")<<endl;
    
    //Date time1;
    //cout<<time1.GetTime("%Y%m%d-%H%M%S.log")<<endl;

    //初始化，后续得放到对象池中，同一初始化
    Log glog("../log/mount-service");
    long sec = 0;
    while(1){
        sleep(1);
        ERROR_LOG("测试[%ld]\n", sec++);
    }
    return 0;
}
