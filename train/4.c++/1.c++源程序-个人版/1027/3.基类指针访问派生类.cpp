#include<iostream>
//3.cpp
using namespace std;
#if 0
练习1:Stu,Ste
用一个数组[8], 储存Stu和Ste的对象
Person *arr[8];
arr[i]=new Stu;
if(arr[i]->isGood())
    arr[i]->show();
练习2:
基类：Person
      id,name,Person *next
      static Person *head
      virtual void show()
      virtual bool is Good()
派生：Ste
      论文数量count
      void show()
      isGood(){
      论文数量大于3是优秀
      }
派生：Stu
      score
      void show()
      isGood(){
      分数大于90是优秀
      }
Person *head
new Stu;
new Ste;
#endif

class Person{
    protected:
    int id;
    string name;
    Person *next;
    public:
    Person() : Person(0,"--"){}
    Person(int i,string n) : id(i),name(n),next(NULL){}
    virtual void show(){
	cout<<id<<" "<<name<<" ";
    }
    virtual bool isGood(){
	return true;
    }
};

class Stu : public Person{
    int score;
    public:
    Stu() : Person(0,"--"),score(0){}
    Stu(int i,string n,int num) : Person(i,n),score(num){}
    bool isGood(){
	return score>90;
    }
    void show(){
	Person::show();
	cout<<score<<endl;
    }
};

class Ste : public Person{
    int count;
    public:
    Ste() : Person(0,"--"),count(0){}
    Ste(int i,string n,int num) : Person(i,n),count(num){}
    bool isGood(){
	return count>3;
    }
    void show(){
	Person::show();
	cout<<count<<endl;
    }
};
int main(){
    #if 1
    //练习1
    Person *arr[10]={NULL};
    for(int i=0,j=0;i<10;i++){
	if(i>4)
	    arr[i]=new Ste(2000+j,"Red",j++);
	else
	    arr[i]=new Stu(1000+i,"lisi",88+i);
    }
    cout<<"优秀名单:"<<endl;
    /*
    for(int i=0;i<10;i++){
	if(arr[i]->isGood())
	    arr[i]->show();
    }
    */
    for(auto m:arr)
	if(m->isGood())
	    m->show();
    #endif
    return 0;
}
