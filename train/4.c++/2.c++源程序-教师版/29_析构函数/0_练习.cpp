
写一个双向链表;
struct Student
{
	int id;
	string name;
	int age;
};
struct Node
{
	Student stu;
	Node* prev;
	Node* next;
};
class List
{
	Node* head;
	Node* tail;
public:
	List() : head(nullptr), tail(nullptr) { }
	List(int n) { }
	List(const List& l) { }
	insert_back()
	insert_front()
	delete_back()
	delete_front()
	remove_by_id(int id) 
	search_by_id(int id)
	update_by_id(int id)
	show()
	~List() 
};
{
	List l1;
	l1.show();
}
List l2(2);
List l3(l2);


