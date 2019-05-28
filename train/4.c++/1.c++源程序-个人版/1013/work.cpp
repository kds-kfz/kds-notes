/*
   作业:

   使用 new 和 delete 创建删除　链表　：　实现增删查改	
 */
#include <iostream>
//#include <string>	
using namespace std;

struct Data
{
    int id;
    string name;
    int score;
};
struct Student
{
    Data data;
    Student* next;
};

//Student* 
void insert_front(Student*& h, Data d)
{
    //	if (h == nullptr) 
    //		h = new Student{d, nullptr};
    //	else
    h = new Student{d, h};
    // Student* p = new Student{d, h};
    // h = p;
    //	return head;
}
Student* search_by_id(Student* h, int id)
{
    while(h) {
	if (h->data.id == id)
	    return h;
	h = h->next;
    }
    return nullptr;
}
//"91"  -- 91
//string s = to_string(int a);
int toInt(string str)
{
    int num = 0;
    for (int i=0; i<str.size(); i++) {
	num *= 10;
	num += str[i] - '0';
    }
    return num;
}

bool update_by_id(Student* h, int id)
{
    auto p = search_by_id(h, id);
    if (p == nullptr) {
	cout << "没有这个id" << endl; 
	return false;
    }
    cout << "请输入更改后的 name, score :" << endl;
    cin >> p->data.name;
    //p->data.score 
    string score;
    cin >> score;
    for (int i=0; i<score.size(); i++) {
	if (score[i] < '0' || score[i] > '9') {
	    cout << "分数输入错误" << endl;
	    return false;
	}
    }
    p->data.score = toInt(score);
    return true;
}
bool delete_by_id(Student*& head, int id)
{
    Student* prev = nullptr;
    Student* h = head;
    while(h) {
	if (h->data.id == id)
	    break;
	prev = h;
	h = h->next;
    }
    if (h == nullptr)
	return false;
    if (prev == nullptr) {
	head = head->next;
	delete h;
    } else {
	prev->next = h->next;
	delete h;
    }
    return true;
}

void free(Student* h) 
{
    Student* p = h;
    while(h)
    {
	p = h->next;
	cout << "delete " << h->data.id << endl;
	delete h;
	h = p;
    }
}

void show(Student* h) 
{
    while(h) {
	cout << h->data.id << " " << h->data.name << " "
	    << h->data.score << endl;
	h = h->next;
    }
}

int main()
{	
    //c++11　nullptr 空指针 0x0
    Student* head = nullptr; //NULL == 0
    Data stu1 = {1001, "Tom", 90};
    insert_front(head, stu1);
    insert_front(head, {1002, "Mike", 89});
    insert_front(head, {1003, "Bob", 79});
    insert_front(head, {1004, "John", 69});

    show(head);
    cout << "----------------" << endl;
    bool b = update_by_id(head, 1002);
    if (!b)
	cout << "更改失败" << endl;
    else
	cout << "更改成功" << endl;
    show(head);
    cout << "----------------" << endl;

    delete_by_id(head, 1003);
    show(head);
    cout << "----------------" << endl;
    delete_by_id(head, 1004);
    show(head);
    cout << "----------------" << endl;

    free(head);
    return 0;
}

