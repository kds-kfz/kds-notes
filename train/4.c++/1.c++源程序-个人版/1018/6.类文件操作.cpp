#include<iostream>
//6.cpp
using namespace std;

//文件操作类
struct Stu{
    int id;
    char name[20];
    char sex[4];
    /*
    public:
    void set_stu(int id,string name,string sex){
	this->id=id;
	this->name=name;
	this->sex=sex;
    }
    void show(){
	cout<<"id :"<<id<<endl;
	cout<<"name :"<<name<<endl;
	cout<<"sex :"<<sex<<endl;
    }
    */
};
class File{
    FILE *fp;
    public:
    bool open(const char *filename, const char *mode){
	fp=fopen(filename,mode);
	if(fp==nullptr){
	    cout<<"open error\n";
//	    abort();
	    return false;
	}
	return true;
    }
    int getc(){
	return fgetc(fp);
    }
    int putc(int c){
	return fputc(c,fp);
    }
    char *gets(char *str,int size){
	return fgets(str,size,fp);
    }
    int puts(const char *s){
	return fputs(s,fp);
    }
    /*
    int print(const char *format,char *args){
	return fprintf(fp,format,args);
    }
    long tell(){
	return ftell(fp);
    }	
    */
    size_t write(const Stu &d){
	return fwrite(&d,sizeof(Stu),1,fp);
    }
    size_t read(Stu &s){
	return fread(&s,sizeof(Stu),1,fp);
    }
    bool close(){
	if(fp)
	    return !fclose(fp);
	return false;
    }
};

int main(){
    Stu d1={1001,"lisi","m"};
    Stu d2;
    File fd;
#if 0
    fd.open("stu.txt","w");
    fd.putc('A');
    fd.puts("hello");
//    d1.set_stu(1001,"lisi","m");
    fd.write(d1);
    fd.close();
#endif
#if 1
    fd.open("stu.txt","r");
    char c=fd.getc();
    cout<<c<<endl;

    char str[10];
    fd.gets(str,6);
    cout<<str<<endl;

    fd.read(d2);
    cout<<"id :"<<d2.id<<"\n";
    cout<<"name :"<<d2.name<<"\n";
    cout<<"sex :"<<d2.sex<<"\n";
    fd.close();
#endif
    return 0;
}
