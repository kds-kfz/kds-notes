#include <iostream>
using namespace std;
struct Person
{
	int id;
	string name;
	string sex;
	void init(int id, string name, string sex) {
		this->id = id;
		this->name = name;
		this->sex = sex;

	}
	void show() {
		cout << id << " " << name << " " << sex << endl;
	}
};

int main()
{
	Person p1;
	p1.init(1001, "Tom", "male");
	p1.show();
	p1.id = 1002;

	return 0;
}
