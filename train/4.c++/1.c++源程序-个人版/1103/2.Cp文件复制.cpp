#include<iostream>
#include<fstream>
#include<cstring>
//2.Cp文件复制.cpp
using namespace std;

//练习：实现cp命令
//./Cp file1 file2

int main(int argc,char *argv[]){
    if(argc!=3){
	cout<<"参数错误"<<endl;
	return 0;
    }
    if(strcmp(argv[2],".")==0){
	return 0;
    }
    if(strcmp(argv[1],argv[2])==0){
	cout<<"两文件同名"<<endl;
	return 0;
    }
    fstream fp1;
    fstream fp2;
    fp1.open(argv[1],ios::in);
    fp2.open(argv[2],ios::out);
    if(!fp1.is_open()){
	cout<<"没有这个文件 "<<argv[1]<<endl;
	fp1.close();
	fp2.close();
	return 0;
    }
    #if 0
    //方式一：每次读一个字节
    char c;
    while((c=fp1.get())!=EOF)//若没读到EOF文件末尾
	fp2.put(c);//成功从文件1读取一个字节写入文件2
    #endif
    //方法二：读取连续字节
    char buf[13];
    int len=0;
    while(1){
	fp1.read(buf,13);//从文件1连续读取13字节，不含\0
	len=fp1.gcount();//统计上一次读取个数
	fp2.write(buf,len);//将读取成功的13文件2
	//若已经到达EOF文件末尾，则结束读取
	//len不一定都是13,而是实际读取的字节数，
	//若文件有80字节，80/13=6.15...表示工读取7次，
	//前6次每次读取13字节，第7次读取2个字节
	//第一个字节是文件最后一个字节，第二个是EOF
	if(len<13){
	    cout<<"len = "<<len<<endl;
	    break;
	}
    }
    //不写也会调用默认析构，关闭文件
    fp1.close();
    fp2.close();
    return 0;
}

