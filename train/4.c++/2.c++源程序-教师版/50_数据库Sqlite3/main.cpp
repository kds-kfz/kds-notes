

SQL 语言操作数据库
.database 显示当前数据库
.table	显示当前数据库里的表格
.q .exit 退出
数据类型:
integer 整数 : int, short, long
text	文本 : string
real	浮点型: float, double
blob	二进制
//每个语句按;结束
//创建数据库在当前目录下
sqlite3 usr.db
//创建表格
create table stu(id int, name text, age integer, score int);
//查看表格所有内容:
select * from stu;
//查找表格里某个内容
select * from stu where id = 1001; //id为1001的一行
select id from stu where name = 'Tom';//名字为Tom的id
//向表格里增加内容:
insert into stu(id, name, age, score) values(1001, 'Tom', 12, 90);
//修改表格的内容
update stu set name = "Cindy" where id = 1002 and name = 'Bob';
//删除表格中的数据
delete from stu where id = 1001 and name = 'Bob';
//删除表格
drop table xiaoming;
		
id integer primary key autoincrement
            ----------  -------------
		    主键		自动增加
