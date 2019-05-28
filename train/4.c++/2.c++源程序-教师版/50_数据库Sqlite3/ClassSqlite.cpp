#include <iostream>
#include "sqlite3.h"
#include <cstdio>
using namespace std;
struct Student {
	int id;
	string name;
	int age;
	int score;
};
struct Teacher {
	int id;
	string name;
	int age;
	string subject;
};
class Sqlite
{
protected:
	sqlite3* pdb;
	int r;
	int c;
	int ok;
	char** buf;
public:
	Sqlite(const char* database) : 
		r(0), c(0), buf(NULL), ok(SQLITE_ERROR) 
	{
		ok = sqlite3_open(database, &pdb);
		if (ok == SQLITE_OK) 
			cout << "打开成功" << endl;
		else
			cout << "打开失败" << endl;
	}
	~Sqlite() {
		if (buf != NULL) {
			cout << "释放表格" <<endl;
			sqlite3_free_table(buf);
		}
		if (is_open()) {
			sqlite3_close(pdb);
			cout << "关闭数据库" << endl;
		}
	}
	int exec(const string& s) {
		if (! is_open())
			return SQLITE_ERROR;
		if (s[0] == 's') {
			if (buf != NULL)
				sqlite3_free_table(buf);
			return sqlite3_get_table(pdb, s.c_str(), &buf, &r, &c, NULL); 
		} else {
			return sqlite3_exec(pdb, s.c_str(), NULL, NULL, NULL);
		}
	}
	void show_table() {
		if (buf == NULL)
			return;
		for (int i=0; i<(r+1)*c; i++)
		{
			cout << buf[i] << " ";
			if ((i+1) %c == 0)
				cout << endl;
		}
	}
	bool is_open() {
		return ok == SQLITE_OK;
	}
	char** operator[](int n) {
		return buf + c*(n);
	}
};
//is-a
class StudentSqlite : public Sqlite
{
	char sql[1024];
public:
	StudentSqlite(const char* db) : Sqlite(db) { }
	void insert(const Student& s) {
		sprintf(sql, "insert into stu values(%d, '%s', %d, %d);", s.id, s.name.c_str(), s.age, s.score);
		if (exec(sql) != SQLITE_OK) {
			cout << "增加学生失败" << endl;
		}
	}
};
//has-a
class TeacherSqlite 
{
	char sql[1024];
	Sqlite sqlite;
public:	
	TeacherSqlite(const char* db) : sqlite(db) { }
	void insert(const Teacher& t) {
		sprintf(sql, "insert into tea values(%d, '%s', %d, \'%s\');", t.id, t.name.c_str(), t.age, t.subject.c_str());
		if (sqlite.exec(sql) != SQLITE_OK) {
			cout << "增加教师失败" << endl;
		}
	}
	void show_table() {
		sqlite.exec("select * from tea;");
		sqlite.show_table();
	}
};
int main()
{ 
//Student : id, name, age, score
//Teacher : id, name, age, subject
	Student s1 = {1003, "Cindy", 23, 89};
	Teacher t1 = {2001, "Black", 45, "math"};
//	Sqlite sq("usr.db");
	//继承
//	StudentSqlite st("usr.db");
//	cout << "insert " << endl;
//	st.insert(s1);//增删查改
	TeacherSqlite te("usr.db");
	te.insert(t1);
	te.show_table();

//	sq.insert(t1);
//	char sql[1024];
//	sprintf(sql, "insert into stu values(%d, '%s', %d, %d);", s1.id, s1.name.c_str(), s1.age, s1.score);
//	sq.exec(sql);
	
//	int ok = sq.exec("insert into stu values(1023, 'Mark', 23, 89);");
//	if (ok != SQLITE_OK) {
//		cout << "操作失败" << endl;
//		return 0;
//	}
/*	sq.exec("select * from stu;");
	if (ok != SQLITE_OK) {
		cout << "操作失败" << endl;
		return 0;
	}
	sq.show_table();
*/
//	buf == char* [] 
//	cout << sq[1] [0] << endl; //== "1001" char*
//       n
//	sq[1]
//	sq.operator[](1) [0]
	return 0;
}
