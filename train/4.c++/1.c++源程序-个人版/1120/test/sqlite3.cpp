#include<iostream>
#include<cstdio>
#include"sqlite3.h"
using namespace std;
/*
class Sqlite3(){
    sqlite3 *pdb;
    char **buf;
    int r;
    int c;
    public:
    Sqlite3(){}
    int Get_c(){
	return c;
    }
    int Get_r(){
	return r;
    }
};
*/
int main1(){

    sqlite3 *pdb;
    int ok;
    ok=sqlite3_open("usr.db",&pdb);
    if(ok!=SQLITE_OK){
	cout<<"数据库打开失败"<<endl;
	return 0;
    }
    else{
	cout<<"打开数据库成功"<<endl;	
    }
    string sql;
    sql="insert into stu values(1010,'lili',12,78);";

    //增，删，查 都可以用sqlite3_exec()
    //
    ok=sqlite3_exec(pdb,sql.c_str(),NULL,NULL,NULL);
    
    char **buf=NULL;
    int row;
    int col;
    sql="select *from stu";
    //数据库指针，字符串，二级指针，行，列，回调函数地址
    ok=sqlite3_get_table(pdb,sql.c_str(),&buf,&row,&col,NULL);

    if(ok==SQLITE_OK){
	cout<<"操作成功"<<endl;
    }
    else{
	cout<<"操作失败"<<endl;
    }

    /*
    for(int i=0;i<(row+1)*col;i++)
	cout<<buf[i]<<endl;
	*/
    /*
       buf == char *[];
       [0]    [1]     [2]     [3]      [4]
       "id"  "name"  "age"   "score"  "1001"  ...
       */
    for(int i=0;i<(row+1)*col;i++){
	cout<<buf[i]<<" ";
	if((i+1)%col==0)
	    cout<<endl;
    }
    if(buf!=NULL)
	sqlite3_free_table(buf);//释放buf指向的表格
    sqlite3_close(pdb);
    return 0;
}
