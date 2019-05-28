/*
   作业:
   写一个类, 文件操作的类
   FILE* fp = fopen("a.txt", "w");
   if (fp == NULL) 
   ...
   char c = fgetc(fp);
   char str[10] ;
   fputc(c, fp);
   fread(str, 3, 1, fp);
   fwrite(str, 10, 1, fp);
 */
#include <iostream>
#include <cstdio>
#include <cstdlib>
using namespace std;
struct Student
{
	int id;
	char name[20];
};
class File 
{
	FILE* fp;
	public:
	bool open(const char* fileName, const char* mod) {
		fp = fopen(fileName, mod);
		if (fp == NULL) {
			cout << "open fail" << endl;
		//	abort();
			return false;
		}
		return true;
	}
	int get() {
		if (fp)
			return fgetc(fp);
		else 
			return -1;
	}
	char* get(char* s, int n) {
		return fgets(s, n, fp);
	}
	int put(char c) {
		return fputc(c, fp);
	}
	int put(const char* s) {
		return fputs(s, fp);
	}
	size_t read(Student& s) {
		return fread(&s, sizeof(Student), 1, fp);
	}
	size_t write(const Student& s) {
		return fwrite(&s, sizeof(Student), 1, fp);
	}
	bool close() {
		if (fp != NULL)
			return !fclose(fp);
		else
			return false;
	}
};

int main()
{
	File p;
	p.open("b.txt", "r");
//	p.get();
//	p.put('a');
//	p.put("hello");
	char c = p.get();
	cout << "c = " << c << endl;
	char str[10]; //"hello\0"
	p.get(str, 6);
	cout << str << endl;
	Student s1;// = {1001, "Tom"};
	p.read(s1);
//	p.write(s1);
	cout << s1.id << "\n" << s1.name << endl;
	p.close();
	return 0;
}

