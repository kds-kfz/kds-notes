#include <iostream>
using namespace std;
struct Data
{
	int id;
	string name;
	int age;
	Data() : id(0), age(0) { }
	Data(int i, string n, int a) : id(i), name(n), age(a) {}
};

struct Node
{
	Data data;
	Node* prev;
	Node* next;
	Node() : prev(nullptr), next(nullptr) { }
	Node(Data d, Node* p, Node* n) : data(d), prev(p), next(n) {}
};

class List
{
	Node* head;
	Node* tail;
	int len;
public:
	List() : head(nullptr), tail(nullptr), len(0) {
		cout << "List 0" << endl;
	}
	List(int n) : head(nullptr), tail(nullptr)
	{
		cout << "List " << len << endl;
		Node* p = nullptr;
/*		if (n < 0)
			len = 0;
		else 
			len = n; */
		len = n<0 ? 0 : n;
		for (int i=0; i<n; i++) {
			p = new Node;
			if (i == 0) {
				head = tail = p;
			} else {
				tail->next = p;
				p->prev = tail;
				tail = p;
			}
		}
	}
	List(const List& l) : head(nullptr), tail(nullptr), len(0){
		Node* p = l.head;
		for (int i=0; i<l.len; i++) {
			insert_back(p->data);
			p = p->next;
		}
	}
	~List() {
		Node* p = head;
		cout << "delete " << endl;
		while(p) {
			cout << "delete " << p->data.id << endl;
			head = head->next;
			delete p;
			p = head;
		}
	}
	void insert_front(Data d) {
		head = new Node(d, nullptr, head);
		if (len == 0) {
			tail = head;
		} else {
//		p->next = head;
//		head->prev = p;
//		head = p;
			head->next->prev = head;
		}
		len++;
	}
	void insert_back(Data d) {
		Node* p = new Node(d, tail, nullptr);
		if (len == 0) {
			head = tail = p;
		} else {
			tail->next = p;
			tail = p;
		}
		len++;
	}
	void delete_front() {
		if (len == 0)
			return;
		if (len == 1) {
			delete head;
			head = tail = nullptr;
		} else if (len > 1) {
			Node *p = head->next;
			delete head;
			head = p;
			head->prev = nullptr;
		}
		len--;
	}
	void delete_back() {
		if (len == 0)
			return;
		if (len == 1) {
			delete tail;
			head = tail = nullptr;
		} else if (len > 1) {
			Node* p = tail->prev;
			delete tail;
			tail = p;
			tail->next = nullptr;
		}
		len--;
	}
	Node* search_by_id(int id) {
		Node* p = head;
		while(p) {
			if (p->data.id == id)
				return p;
			p = p->next;
		} 
		return nullptr;
	}
	bool remove_by_id(int id) {
		Node* p = search_by_id(id);
		if (p == nullptr)
			return false;
		else if (len == 1) {
			delete p;
			head = tail = nullptr;
			len = 0;
		} else if (p == head) {
			head = head->next;
			head->prev = nullptr;
			delete p;
			len--;
		} else if (p == tail) {
			delete_back();
		} else {
			p->next->prev = p->prev;
			p->prev->next = p->next;
			delete p;
			len--;
		}
		return true;
	}
	void show() {
		Node * p = head;
		cout << "++++++front -> back++++++++++++++" << endl;
		for (int i=0; i<len; i++) {
			cout << p->data.id << " " << p->data.name 
				<< " " << p->data.age << endl;
			p = p->next;
		}
		cout << "------back -> front-----" << endl;
		p = tail;
		while(p) {
			cout << p->data.id << " " << p->data.name 
				<< " " << p->data.age << endl;
			p = p->prev;	
		}
		cout << "========================" << endl;
	}
};

int main()
{
	List l1;
	l1.insert_front({1001, "Tom", 12});
	l1.insert_front({1002, "Jack", 11});
	l1.insert_front({1003, "Mike", 14});
	l1.insert_back({1004, "Jim", 13});
/*	l1.show();
	l1.remove_by_id(1001);
	l1.show();
	l1.remove_by_id(1003);
	l1.show();
	if (!l1.remove_by_id(1009))
		cout << "删除失败" << endl;
*/
	List l2(l1);
	l2.delete_back();
	
	l1.show();
	l2.show();
	

//	List l2(4);
//	l2.show();

	return 0;
}
