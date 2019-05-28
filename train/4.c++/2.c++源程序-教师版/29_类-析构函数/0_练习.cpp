
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
	int len;
public:
	List() : head(nullptr), tail(nullptr) { }
	List(int n) { }	// n个结点
	List(const List& l) { } //拷贝整条链表
	insert_back()	//尾插
	insert_front()	//头插
	delete_back()	//删除尾
	delete_front()	//删除头
	remove_by_id(int id) //通过id删除一个结点
	search_by_id(int id) //查找结点
	update_by_id(int id) //改变结点的内容
	show()				 //显示整个链表
	~List()		//释放整个链表
};
	List l1;	//每个对象都是一个链表
	2->5->6->8
	l1.show();

	List l2(2);
	List l3(l1);
	2->5->6->8
	


