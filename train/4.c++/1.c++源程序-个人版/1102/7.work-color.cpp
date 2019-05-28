#include<iostream>
#include<set>
//7.work-color.cpp
using namespace std;
#if 0
练习：
用集合记录c++的关键字
this,friend,new,delete,for,while,case,
defalut,if,public,protected,private,const,
final,override,template,typename,using,auto

输出一个cpp的文件，相应的关键字带有颜色
#endif

#define Green "\033[32m"
#define Defalut "\033[0m"

enum Color{
    zero=0,
    black=30,	    //黑
    red=31,	    //红
    green=32,	    //绿
    yellow=33,	    //黄
    bule=34,	    //蓝
    purple=35,	    //紫
    darlbule=36,    //深绿
    wihte=37	    //白
    /*//错误
    public:
    Color operator++(int){
	enum Color t=this;
	*this++;
	return t;
    }
    //enum Color i;
    //i+1,i++,i+=1;c++中是错误，c语言中正确
    */
};
void message(Color c){
    cout<<"\033["<<c<<"m"<<flush;
}
//enum Color *p;
//int main(int argc,char *argv[])
//输入了几个命令
//字符串数组，每个元素都是char *类型,指向常量字符串

int main(int argc,char *argv[]){
    //如果传入的参数不是两个则结束本程序
    if(argc!=2){
	cout<<"参数错误"<<endl;
	return 0;
    }   
    //argv[0]表示字符串，这里是可执行文件"./a.out"
    //argv[1]表示字符串，这里是打开的文件路径，只有文件名默认在当前路径
    FILE *fp=fopen(argv[1],"r");
    set<string> s1={"int","double","char","FILE","void",
		     "enum","namespace","const","float"};
    set<string> s2={"while","if","else","return","using",
		    "case","new","delete","defalut"};
//    char buf[40];
    string s;
    char c;
//    while(fscanf(fp,"%s",buf)==1){//遇到空格结束，不好用
    while((c=fgetc(fp))!=EOF){
	if(c!=' '&&c!='\t'&&c!='\n')
	    s+=c;//追加' '&&'\n'&&'\t'前的字符组成字符串
	else if(c==' '||c=='\t'||c=='\n'){
	    if(s1.find(s)!=s1.end())
		message(green);//绿色字体
	    else if(s2.find(s)!=s2.end())
		message(yellow);//黄色字体
	    else
		message(zero);//关闭属性
	    cout<<s<<c;//输出字符串后输出' '或'\n'或'\t'
	    s.resize(0);
	}
    }
    message(zero);
    cout<<s<<endl;//输出最后一个字符0,return 0
    return 0;
}
