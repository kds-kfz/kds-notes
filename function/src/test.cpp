#include"pubfun.h"

using namespace std;
using namespace kfz;

int main(int argc, char *argv[]){
    Files fd;
    fd.Open("1.txt", O_RDONLY);
    cout<<"size:"<<fd.Size()<<endl;
    cout<<"date:"<<fd.Date();
    fd.Write("./","2.txt","hello");
    return 0;
}
