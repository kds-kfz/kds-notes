#include<iostream>
#include<map>
#include<unistd.h>
//4.work.cpp
using namespace std;
#if 0
练习
创建1个文件，存储单词和相应的释义
写一个程序，读取文件的内容，存储到map里
输入单词，打印出释义
#endif

char buf1[30]="0";
char buf2[30]="0";
string buf3;
string buf4;
void Change(char *arr,string &buf){
/*
    for(;*arr!='\0';){
	buf+=*arr;
	arr++;
    }
    */
    for(int i=0;arr[i]!='\0';i++){
	buf+=arr[i];
    }
}

int main(){
    map<string,string> m;
    string word;
    FILE *fp=fopen("9.word.txt","r");
    if(fp){
	cout<<"文件打开成功"<<endl;
	int ret=0;
	while(1){
	    ret=fscanf(fp,"%s %s",buf1,buf2);
	    if(ret==2){
		Change(buf1,buf3);
		Change(buf2,buf4);
		m.insert({buf3,buf4});
		buf3.resize(0);
		buf4.resize(0);
		for(int i=0;i<30;i++){
		    buf1[i]='\0';
		    buf2[i]='\0';
		}
	    }
	    else
		break;
	}
    fclose(fp);
    }
    else
	cout<<"文件打开失败"<<endl;
//    for(auto i:m)
//	cout<<i.first<<" "<<i.second<<endl;

    while(1){
    system("clear");
    cout<<"请输入单词"<<endl;
    cin>>word;
    map<string,string>::iterator i=m.find(word);
    if(i!=m.end()){
	cout<<i->first<<" "<<i->second<<endl;
    }
    else
	cout<<"没找到"<<endl;
    sleep(3);
    }
    return 0;
}
