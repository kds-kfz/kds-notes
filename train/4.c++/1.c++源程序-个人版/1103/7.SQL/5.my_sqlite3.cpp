#include<iostream>
#include<cstdio>
#include"sqlite3.h"
//5.my_sqlite3.cpp
using namespace std;
class Sqlite3{
    sqlite3 *pdb;
    char **buf;
    int r;
    int c;
    int ok;
    public:
    Sqlite3(const char *database) : r(0),c(0),buf(NULL),ok(SQLITE_ERROR){
	ok=sqlite3_open(database,&pdb);
	if(ok==SQLITE_OK)
	    cout<<"打开成功"<<endl;
	else
	    cout<<"打开失败"<<endl;
    }
    int exec(const string &s){
	if(!is_open())
	    return SQLITE_ERROR;
	if(s[0]=='s'){
	    if(buf!=NULL)
		sqlite3_free_table(buf);
	    return sqlite3_get_table(pdb,s.c_str(),&buf,&r,&c,NULL);
	}
	else{
	    sqlite3_exec(pdb,s.c_str(),NULL,NULL,NULL);
	}
    }
    void show_table(){
	if(buf==NULL)
	    return;
	for(int i=0;i<(r+1)*c;i++){
	    cout<<buf[i]<<" ";
	    if((i+1)%c==0)
		cout<<endl;
	}
    }
    bool is_open() {
	return ok == SQLITE_OK;
    }
    ~Sqlite3(){
	if(is_open()){
	    cout<<"关闭数据库"<<endl;
	    sqlite3_close(pdb);
	}
	if(buf!=NULL){
	    cout<<"释放数据"<<endl;
	    sqlite3_free_table(buf);
	}
    }
    int Get_c(){
	return c;
    }
    int Get_r(){
	return r;
    }
    char **operator[](int n){
	return buf+c*(n);
    }
};

int main(){
    Sqlite3 sq("usr.db");
//    int ok=sq.exec("insert into stu values(2017,'oppo',12,90);");
    int ok=sq.exec("delete from stu where id=1010;");
    if(ok!=SQLITE_OK){
	cout<<"操作失败"<<endl;
	return 0;
    }
    sq.exec("select *from stu");
    if(ok!=SQLITE_OK){
	cout<<"操作失败"<<endl;
	return 0;
    }
    sq.show_table();

    //buf==char *[]
    cout<<sq[1][0]<<endl;
//    cout<<*sq[1]<<endl;
    //	    n
    //sq[1]
    //sq.operator[](1)[0]

    return 0;
}
