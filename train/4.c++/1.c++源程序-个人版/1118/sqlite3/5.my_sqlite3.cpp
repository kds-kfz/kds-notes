#include<iostream>
#include<cstdio>
#include"sqlite3.h"
//5.my_sqlite3.cpp
using namespace std;
class Sqlite3{
    protected:
    sqlite3 *pdb;
    char **buf;
    int r;//行row
    int c;//列col
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
struct Stu{
    int id;
    string name;
    int age;
    int score;
};
struct Ste{
    int id;
    string name;
    int age;
    string subject;
};
//is-a
class StuSqlite3 : public Sqlite3{
    char sql_buf[1024];
    public:
    StuSqlite3(const char *db) : Sqlite3(db){}
    void insert(const Stu &s){
	sprintf(sql_buf,"insert into stu values(%d,'%s',%d,%d);",
	    s.id,s.name.c_str(),s.age,s.score);
	if(exec(sql_buf)!=SQLITE_OK)
	    cout<<"学生数据插入失败"<<endl;
    }
};
//has-a
class SteSqlite3{
    char sql_buf[1024];
    Sqlite3 sqlite;
    public:
    SteSqlite3(const char *db) : sqlite(db){}
    void insert(const Ste &s){
	sprintf(sql_buf,"insert into ste values(%d,'%s',%d,\'%s\');",
	    s.id,s.name.c_str(),s.age,s.subject.c_str());
	if(sqlite.exec(sql_buf)!=SQLITE_OK)
	    cout<<"教师数据插入失败"<<endl;
    }
    void show_table(){
	sqlite.exec("select *from ste");	
	sqlite.show_table();
    }
};
int main(){
#if 0
    Ste t1={2001,"lisi",24,"语文"};
    SteSqlite3 te("usr.db");
    te.insert(t1);
    te.show_table();
#endif
#if 1
    Sqlite3 sq("usr.db");
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
    cout<<sq[2][0]<<endl;
    cout<<*sq[1]<<endl;
    //	    n
    //sq[1]
    //sq.operator[](1)[0]
#endif
    return 0;
}
