#include <iostream>
#include <cstdio>
#include "sqlite3.h"
using namespace std;

//把sqlite3数据库的c语言操作形式改成一个c++的类

//fstream f1("a.txt");
//f1.put('x');
//f1.get();
Sqlite sq("usr.db");
sq.exec("insert into stu values(200,'a', 32,33);");
sq.exec("select * from stu;");

class Sqlite
{
	sqlite3* pdb;
	char** buf;
	int r;
	int c;

public:
	Sqlite() {
	
	}
	~Sqlite() { }
	int exec(char* sql) {
		
	}
	int row() {
		return r;
	}

};
int main()
{
	sqlite3* pdb;
	int ok;
	ok = sqlite3_open("usr.db", &pdb);
	if (ok == SQLITE_OK) {
		cout << "打开数据库成功" << endl;
	} else {
		cout << "打开数据库失败" << endl;
		return 0;
	}
	//char sql[1024];
	string sql;
	char** buf = NULL;
	int row = 0;
	int col = 0;
	sql = "insert into stu values(1005, 'Lisa', 12, 89);";
	//增删改
	ok = sqlite3_exec(pdb, sql.c_str(), NULL, NULL, NULL);
	sql = "select * from stu;";
	//查
	ok = sqlite3_get_table(pdb, sql.c_str(), &buf, &row, &col, NULL);
	if (ok == SQLITE_OK) {
		cout << "操作成功" << endl;
	} else {
		cout << "操作失败" << endl;
		return 0;
	}
//	buf == char* []; buf指向一个一维char*数组
//	[0]  [1]    [2]   [3]     [4] ...
//	"id" "name" "age" "score" 
//	"1001" "Tom" "12"  "90"
//	"1002" "Tom" "12"  "90"
//	row = 2; col = 4; row不包含表头
	for (int i=0; i<(row+1)*col; i++) {
		cout << buf[i] << " ";
		if ((i+1) % col == 0)
			cout << endl;
	}
	if (buf != NULL)
		sqlite3_free_table(buf); //释放buf指向的表格

	sqlite3_close(pdb);
	return 0;
}
