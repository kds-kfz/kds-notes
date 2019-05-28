#include <iostream>
using namespace std;

template <typename T>
class List
{
	struct Node {
		T value;
		Node* prev;
		Node* next;
	};
	Node* head;
	Node* tail;
	int len;
public:
	class Iterator {
		Node* pi;
	public:
	//	...
	};
//	List()
	List(int n) : head(nullptr), tail(nullptr), len(n){
		Node* p = nullptr;
		p = new Node{};
		head = tail = p;
		for (int i=0; i<n-1; i++) {
			p = new Node{};	
			tail->next = p;
			p->prev = tail;
			tail = p;
		}
	}
	void show() {
		Node* p = head;
		while(p) {
			cout << p->value << endl;
			p = p->next;
		}
	}
//	List(int n, const T& v)
//	List(const List& l)// = delete;
	~List() {
		Node* p = head;
		while(head) {
			cout << "delete " << p->value << endl;
			p = head->next;
			delete head;
			head = p;
		}
	}
//	size()
//	empty()
//	push_back()
//	push_front()
//	pop_back()
//	pop_front()
//	remove(const T& t) 
//	begin()
//	end()
};

int main()
{
	List<int> l(4); 
	l.show();
	return 0;
}
