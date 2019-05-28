#include<iostream>
//4.dynamic_cast.cpp
using namespace std;

#if 0
//_cast c++强制转换
1.static_cast
    1.可以转数据
    2.不能转const
    3.不能转非继承关系的类指针
    4.可以转派生指针指向基类
    5.不能转虚继承类的指针
    ((当基类有虚函数，)派生继承虚基类时
    不能将基类指针强转成派生虚基类指针,
    即派生类指针指向基类指针)

2.const_cast 
    可以转const
    只处理属性的指针或引用，不会转换数据类型

3.dynamic_cast 
    可以转继承的类
    安全的转换继承的类指针
    必须有虚函数

4.reinterpret_cast
    可以转底层
    转换底层的二进制数据
#endif
#if 0
struct Base{
    int n;
};
struct Der :public Base{
    int m;
};
struct A{
    int w;
};
int main(){

    //static_cast 转换数据类型,不转换const属性
    int a=10;
    const int *p=&a;
    //double d =(double)a;//可以转换任意基本数据类型
    //double d=double(a);//可以转换任意指针
    int *p2=(int *)p;//可以转换const属性
    *p2=90;
    cout<<a<<endl;//90
    double d=static_cast<double>(a);
    cout<<d<<endl;//90
    //static_cast不能转换const属性
    //int *p3=static_cast<int *>(p);//错误
    //*p3=45;
    cout<<a<<endl;//90
    Der *pd=new Der;
    Base *pb=new Base;
    A *pa=new A;
    pd->n=220;
    pd->m=700;
    pb->n=7;
    //pb=pd;//正确，小指大
    //pd=pb;//错误,大指小
    pd=(Der *)pb;//强转，派生指针指向基类对象，能强制类型转换，但有危险

    //pd=static_cast<int *>(pb);

    //能转换有继承关系的指针，
    //但是不考虑派生和基类的关系，如果没有继承关系的不能转换
    //pd=static_cast<int *>(pa);
    
    //正确，换指向，小指大
    //cout<<pb->n<<endl;//220

    //强转，大指小
    cout<<pd->n<<endl;//7
    cout<<pd->m<<endl;//0，非法空间，
    //大指针可以指向8字节，而小指针指向4字节
    //强转后，大指针可以指向高四位，即m的值，低4位是继承Base的值

    return 0;
}
#endif
#if 1
struct Base{
    int n;
//    virtual void show(){cout<<"Base"<<endl;}
};
struct Der : virtual public Base{
    int m;
//    virtual void show(){cout<<"Der"<<endl;}
};
struct A{
    int w;
};
int main(){
#if 0
    //static_cast 转换数据类型,不转换const属性
    int a=10;
    const int *p=&a;
    //double d =(double)a;//可以转换任意基本数据类型
    //double d=double(a);//可以转换任意指针
    int *p2=(int *)p;//可以转换const属性
    *p2=90;
    cout<<a<<endl;//90
    double d=static_cast<double>(a);
    cout<<d<<endl;//90
    //static_cast不能转换const属性
    //int *p3=static_cast<int *>(p);//错误
    //*p3=45;
    cout<<a<<endl;//90
#endif
    Der *pd=new Der;
    Base *pb=new Base;
    A *pa=new A;
    pd->n=220;
    pd->m=700;
    pb->n=7;
    pb=pd;//正确，小指大
    //pd=pb;//错误,大指小
    pd=(Der *)pb;//强转，派生指针指向基类对象，能强制类型转换，但有危险

    //pd=static_cast<int *>(pb);

    //能转换有继承关系的指针，
    //但是不考虑派生和基类的关系，如果没有继承关系的不能转换
    //pd=static_cast<int *>(pa);
    
    //正确，换指向，小指大
    //cout<<pb->n<<endl;//220

    //强转，大指小
    cout<<pd->n<<endl;//7
    cout<<pd->m<<endl;//0，非法空间，
    //大指针可以指向8字节，而小指针指向4字节
    //强转后，大指针可以指向高四位，即m的值，低4位是继承Base的值

    return 0;
}
#endif
