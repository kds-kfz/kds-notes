#include<iostream>
//5.cpp
using namespace std;

//多继承,派生继承虚基类练习
class Animal{
    protected:
    string name;
    string sex;
    string age;
    bool birth;
    public:
    Animal() : Animal("--","--","--",0){cout<<"Animal"<<endl;}
    Animal(string n,string s,string a,bool b) : name(n),sex(s),age(a),birth(b){}
    virtual void show()=0;
    virtual ~Animal(){cout<<"~Animal"<<endl;}
};
class Horse : virtual public Animal{
    protected:
    string hair;
    string contour;
    public:
    Horse() : Horse("--","--","--",0,"--","--"){cout<<"Horse"<<endl;}
    Horse(string n,string s,string a,bool b,string h,string c) : 
	Animal(n,s,a,b),hair(h),contour(c){}
    void show(){
	cout<<name<<" "<<sex<<" "<<age<<" "<<birth<<" "<<hair<<" "<<contour<<endl;
    }
    ~Horse(){cout<<"~Horse"<<endl;}
};
class Donkey : virtual public Animal{
    protected:
    string hair;
    string contour;
    public:
    Donkey() : Donkey("--","--","--",0,"--","--"){cout<<"Donkey"<<endl;}
    Donkey(string n,string s,string a,bool b,string h,string c) : 
	Animal(n,s,a,b),hair(h),contour(c){}
    void show(){
	cout<<name<<" "<<sex<<" "<<age<<" "<<birth<<" "<<hair<<" "<<contour<<endl;
    }
    ~Donkey(){cout<<"~Donkey"<<endl;}
};
class Mule : public Horse,public Donkey{
//    string hair;
//    string contour;
    public:
    Mule(){cout<<"Donkey"<<endl;}
    Mule(string n,string s,string a,bool b,string h,string c) : 
//	Animal(n,s,a,b),
	Horse(n,s,a,b,h,c){

    void show(){
	cout<<name<<" "<<sex<<" "<<age<<" "<<birth<<" "<<Horse::hair<<" "<<Horse::contour<<endl;
    }
    ~Mule(){cout<<"~Mule"<<endl;}
};
int main(){
    Animal *a;
    Horse h("lisi","y","22",1,"长毛","高大");
    Donkey d("xixi","n","23",1,"短毛","白色嘴");
    Mule m("keke","n","24",0,"矮，卷毛","黑白嘴");
    a=&h;
    a->show();
    a=&d;
    a->show();
    a=&m;
    a->show();
    cout<<"A_size = "<<sizeof(Animal)<<endl;
    cout<<"H_size = "<<sizeof(Horse)<<endl;
    cout<<"D_size = "<<sizeof(Donkey)<<endl;
    cout<<"M_size = "<<sizeof(Mule)<<endl;
    return 0;
}
