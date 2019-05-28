#include<iostream>
#include<cstdlib>
//2.cpp
using namespace std;

//析构造函数
class Integer{
    int num;
    int *i;
    public:
    Integer(int n){
	cout<<"i= "<<n<<endl;
	i= new int(n);
    }
    Integer(){
	cout<<"i= 0"<<endl;
	i= new int(0);
    }
    Integer(const Integer &in){
	cout<<"拷贝构造"<<endl;
	cout<<"i= "<<*in.i<<endl;
	i= new int(*in.i);
    }
    //析构函数，程序结束自动销毁对象
    //离开作用于后，函数被调用
    //只释放我们手动申请的空间
    //注意：如果不写析构函数，会默认的析构函数产生
    //~Integer(){}//手动申请空间的，不能释放
    //析构顺序和构造顺序相反
    ~Integer(){
	cout<<"delete :"<<*i<<endl;
	delete i;
    }
    //调用完分析函数后，才会释放本对象的内容，i本身
    void show(){
	cout<<"show i = "<<*i<<endl;
    }
};

int main(){
#if 0
    Integer a1;
    {
    Integer a2(5);
    }//离开作用于后，调用析构
    Integer a3(12);
    Integer a4(a3);
#endif
    Integer *p1=new Integer(88);//调用析构
    delete p1;//手动释放,删除对象，释放空间
    //free(p1);
    //没有调用析构函数，i指向的空间没有释放
    //只释放了num,*i占有空间没有释放

    //c语言的malloc不会调用析构函数
    //free不会调用析构函数
    Integer *p2=(Integer *)malloc(sizeof(Integer));
    //*p2=200;
    //==Integer(200);
    //*p2创造里一个对象，调用里构造函数和析构函数
    free(p2);

    return 0;
}
