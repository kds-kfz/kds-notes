#include<iostream>
#include<fstream>
#include<cstring>
//1.c++文件操作.cpp
using namespace std;
//c++,文件操作
int main(){
    //初始化
    //无参，没有绑定任何文件的对象
    fstream f1;
    
    //绑定文件file，默认是读写方式
    fstream f2("5.file.txt");
    
    //以mode方式打开
//    fstream f3("5.file.txt",mode);
    #if 0
    //mode如下：
    ios::in 读
    ios::out 写,若没文件就创建，有就清空
    ios::app 以读/写方式打开，追加，内容不清空
    ios::binary 二进制
    ios::trunc 打开文件并清空
    ios::ate 打开一个已存在的文件并定位到文件末尾
    //ios::in|ios::inary 以多种方式打开，按位或
    #endif
    
    //打开，关闭
    f1.open("file");
//==    f1.open("file",mode);

    //不手动关闭，默认有析构函数关闭
    f1.close();

    //判断是否打开文件
    f1.is_open();

    #if 0
    /***************单字节IO操作**************/
    //获取1个字符 
    char ch=f2.get();
//==    f2.get(ch);
    cout<<"get "<<ch<<endl;

    //输出1个字符
    f2.put(ch);
    cout<<"get(ch) "<<ch<<endl;

    //向文件写1个字符
    f2.put('W');
    
    //不常用
    //把ch字符放回流里,待用
//    f2.putback('A');

    //不获得1个字符，指针向前移动1个字符，跳过
//    f2.unget();

    //把一个字节以整型方式返回，不从流中删除
//    ch=f2.peek();

    cout<<"peek "<<ch<<endl;

    cout<<"while : ";
    while((ch=f2.get())!=EOF){
	cout<<ch<<",";
    }
    cout<<endl;
#endif
#if 1
    /***************多字节IO操作*************/
    char buf[1024];
    char ch;
    //最多读n-1个字符，第n个是\0,或者遇到ch结束,ch还保存在流里
    //	       n   ch
    f2.get(buf,10,'\n');
    cout<<"get "<<buf<<endl;

    //最多读n-1个字符，第n个是\0,或者遇到ch结束,ch丢弃,不在流里
    f2.getline(buf,10,'\n');
    cout<<"getline "<<buf<<endl;
    memset(buf,'\0',1024);

    //读n个字节,后面没有\0
    f2.read(buf,10);
    cout<<"read "<<buf<<endl;

    //返回上次读取的字节个数
    int num=f2.gcount();
    cout<<"上一次读取字节个数 num = "<<num<<endl;

    //从buf首字符开始写n个字符到文件里
//    f2.write(buf,10);

    //不常用
    //读取并忽略最多n个字符，或者是到ch(包括ch)结束
    f2.ignore(10,' ');

    /***************流随机访问*************/
    //返回输入流的位置
    int a=f2.tellg();

    //返回输出i流的位置
    int b=f2.tellp();
    cout<<"a = "<<a<<",b = "<<b<<endl;

    //fseek(fp,SEEK_CUR);SEEK_SET,SEEK_END
    //直接定位到距离开始位置12的位置 
    f2.seekg(12);
    f2.seekp(43);

    f2.seekg(12,ios::beg);
    f2.seekp(-12,ios::end);
    //ios::cur
    //ios::end
    cout<<f2.tellg()<<endl;

    cout<<"while : ";
    while((ch=f2.get())!=EOF){
	cout<<ch<<",";
    }
    cout<<endl;
#endif
    return 0;
}
