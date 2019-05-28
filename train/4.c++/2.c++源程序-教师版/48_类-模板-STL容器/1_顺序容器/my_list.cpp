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
		Iterator() : pi(nullptr) { }
		Iterator(Node* p) : pi(p) { }
		Iterator& operator++() {
			pi = pi->next;
			return *this;
		}
		Iterator operator++(int) {
			Iterator t(pi);
			pi = pi->next;
			return t;
		}
		Iterator& operator--() {
			pi = pi->prev;
			return *this;
		}
		Iterator operator--(int) {
			Iterator t(pi);
			pi = pi->prev;
			return t;
		}
		bool operator==(const Iterator& it) {
			return pi == it.pi;
		}
		bool operator!=(const Iterator& it) {
			return pi != it.pi;
		}
		T& operator*() {
			return pi->value;
		}
		T* operator->() {
			return &pi->value;
		}
	};
	List() : len(0) {
		head = tail = new Node{};
	}
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
	List(int n, const T& v) : len(n) {
		head = tail = new Node{v, nullptr, nullptr};
		for (int i=0; i<n-1; i++) {
			tail->next = new Node{v, tail, nullptr};
			tail = tail->next;
		}
	}
	List(const List& l) : len(l.len) {
		Node* p = l.head;
		head = tail = new Node{p->value, nullptr, nullptr};
		p = p->next;
		while(p) {
			tail->next = new Node{p->value, tail, nullptr};
			tail = tail->next;
			p = p->next;
		}
	}
	~List() {
		Node* p = head;
		while(head) {
			cout << "delete " << p->value << endl;
			p = head->next;
			delete head;
			head = p;
		}
	}
	T& front() {
		return head->value;
	}
	T& back() {
		return tail->value;
	}
	int size() {
		return len;
	}
	bool empty() {
		return len == 0;
	}
	void push_back(const T& t) {
		if (len != 0) {
			tail->next = new Node{t, tail, nullptr};
			tail = tail->next;
		} else { 
			head->value = t;
		}
		len++;
	}
	void push_front(const T& t) {
		if (len !=0 ) {
			head->prev = new Node{t, nullptr, head};
			head = head->prev;
		} else {
			head->value = t;
		}
		len++;
	}
	void pop_back() {
		if (len > 1) {
			tail->prev->next = nullptr;
			Node* p = tail;
			tail = tail->prev;
			delete p;
			len--;
		} else if (len == 1) {
			len = 0;
		} 
	}
	void pop_front() {
		if (len > 1) {
			Node* p = head;
			head = head->next;
			head->prev = nullptr;
			delete p;
			len--;
		} else if (len == 1) 
			len = 0;
	}

	void remove(const T& t) {
		Node* p = head;
		if (len == 0)
			return;
		while(p) {
			if (p->value == t) {
				if (len == 1)
					len = 0;
				else if (p == head) {
					pop_front();
					p = head;
				} else if (p == tail) {
					pop_back();
					//return;
					p = nullptr;
				} else {
					p->next->prev = p->prev;
					p->prev->next = p->next;
					Node* t = p->next;
					delete p;
					len--;
					p = t;
				}
			} else
				p = p->next;
		}
	}
	Iterator begin() {
		return Iterator(head);	
	}
	Iterator end() {
		return Iterator(tail->next);
	}
};

int main()
{
	List<int> l0;
	l0.push_back(12);
	l0.push_back(13);
	l0.push_back(14);
	l0.push_back(15);
	l0.push_front(12);
	l0.push_front(100);
	l0.push_front(102);
	l0.push_front(12);
//	l0.pop_back();
//	l0.pop_front();
//	l0.remove(12);
//	cout << l0.front() << endl;
//	cout << l0.back() << endl;
//	l0.show();
	for (auto m : l0) {
		cout << m << ",";
	}
	cout << endl;
	for (List<int> ::Iterator it = l0.begin(); it != l0.end(); it++) {
		cout << *it << ". ";
	}
	cout << endl;
/*	List<int> l(4); 
	l.show();
	List<int> l2(4, 10);
	l2.show();
	List<int> l3(l2);
	l3.show();
*/
	return 0;
}
