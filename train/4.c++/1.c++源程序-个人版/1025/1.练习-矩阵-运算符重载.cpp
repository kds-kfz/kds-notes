#include<iostream>
//1.cpp
//练习：写一个2x2的矩阵类

using namespace std;

class Matrix{
    int a[4];
    public:
    /*  +,-,*,>>,<<,=,+=,-=  */
    Matrix(){
	for(int i=0;i<4;i++)
	    a[i]=0;
    }
    Matrix(int d1,int d2,int d3,int d4){
	a[0]=d1;
	a[1]=d2;
	a[2]=d3;
	a[3]=d4;
    }
    Matrix(const Matrix &d){
	for(int i=0;i<4;i++)
	    a[i]=d.a[i];
    }
    Matrix &operator=(const Matrix &d){
	for(int i=0;i<4;i++)
	    a[i]=d.a[i];
	return *this;
    }
    void show(){
	cout<<"{ "<<a[0]<<","<<a[1]<<" }\n";
	cout<<"  "<<a[2]<<","<<a[3]<<"  ";	
    }
    ~Matrix(){}

    friend ostream &operator<<(ostream &out,const Matrix &d);
    friend istream &operator>>(istream &in,Matrix &d);
    friend Matrix operator+(const Matrix &d1,const Matrix &d2);
    friend Matrix operator-(const Matrix &d1,const Matrix &d2);
    friend Matrix operator+=(Matrix &d1,const Matrix &d2);
    friend Matrix operator-=(Matrix &d1,const Matrix &d2);
    friend Matrix operator*(const Matrix &d1,const Matrix &d2);
    friend Matrix operator*(int n,const Matrix &d1);
    friend bool operator==(const Matrix &d1,const Matrix &d2);
    friend bool operator>(const Matrix &d1,const Matrix &d2);
}; 

ostream &operator<<(ostream &out,const Matrix &d){
    out<<"{ "<<d.a[0]<<","<<d.a[1]<<" }\n";
    out<<"  "<<d.a[2]<<","<<d.a[3]<<"  ";
    return out;
}
istream &operator>>(istream &in,Matrix &d){
    in>>d.a[0]>>d.a[1]>>d.a[2]>>d.a[3];
    return in;
}    
Matrix operator+(const Matrix &d1,const Matrix &d2){
    Matrix d;
    for(int i=0;i<4;i++)
	d.a[i]=d1.a[i]+d2.a[i];
    return d;
}
Matrix operator-(const Matrix &d1,const Matrix &d2){
    Matrix d;
    for(int i=0;i<4;i++)
	d.a[i]=d1.a[i]-d2.a[i];
    return d;
}
Matrix operator+=(Matrix &d1,const Matrix &d2){
    for(int i=0;i<4;i++)
	d1.a[i]+=d2.a[i];
    return d1;
}
Matrix operator-=(Matrix &d1,const Matrix &d2){
    for(int i=0;i<4;i++)
	d1.a[i]-=d2.a[i];
    return d1;
}
Matrix operator*(const Matrix &d1,const Matrix &d2){
    Matrix d;
    d.a[0]=d1.a[0]*d2.a[0]+d1.a[1]*d2.a[2];
    d.a[1]=d1.a[0]*d2.a[1]+d1.a[1]*d2.a[3];
    d.a[2]=d1.a[2]*d2.a[0]+d1.a[3]*d2.a[2];
    d.a[3]=d1.a[2]*d2.a[1]+d1.a[3]*d2.a[3];
    return d;
}
Matrix operator*(int n,const Matrix &d1){
    Matrix d;
    for(int i=0;i<4;i++)
	d.a[i]=n*d1.a[i];
    return d;
}
bool operator==(const Matrix &d1,const Matrix &d2){
    return 
	d1.a[0]==d2.a[0]&&d1.a[1]==d2.a[1]&&
	d1.a[2]==d2.a[2]&&d1.a[3]==d2.a[3];
}
bool operator>(const Matrix &d1,const Matrix &d2){
    int n1=d1.a[0]*d1.a[3]-d1.a[1]*d1.a[2];
    int n2=d2.a[0]*d2.a[3]-d2.a[1]*d2.a[2];
    return n1>n2;
}
int main(){
    Matrix a1(1,2,3,4);
    Matrix a2(1,2,2,1);
    Matrix a3(a1);
    Matrix a4=a2;
    Matrix a5(a3);
    Matrix a6(a4);
//    cin>>a1;
    cout<<"a1==a3==a5\n";
    cout<<"a2==a4==a6\n";
    cout<<"a1 :\n"<<a1<<endl;
    cout<<"a2 :\n"<<a2<<endl;
    if(a1==a3)
	cout<<"a1==a3"<<endl;
    if(a1>a2)
	cout<<"a1>a2"<<endl;
    cout<<"a1+a2-----------"<<endl;
    cout<<a1+a2<<endl;
    cout<<"a3-a4-----------"<<endl;
    cout<<a3-a4<<endl;
    cout<<"a5+=a6----------"<<endl;
    a5+=a6;
    cout<<a5<<endl;
    cout<<"a5-=a6----------"<<endl;
    a5-=a6;
    cout<<a5<<endl;
    cout<<"2*a3------------"<<endl;
    cout<<2*a3<<endl;
    cout<<"a1*a2-----------"<<endl;
    cout<<a1*a2<<endl;

    return 0;
}
