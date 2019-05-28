#include"shop.h"
#include"font.h"
//shop.cpp
int Actor::count=1000;//初始化部位防具id
//int Actor::count2=1000;//初始化部位防具id
//int Actor2::count2=2000;//初始化类型装备id
/*
Equip * Factory_All::produceEqu(int l,TYPE t,string n1,string n2,
	int h,int m,float s1,float s2,int a1,int a2,int d,
	    int r,int t2,int g,RARE r2,WEALTH w){
	switch(t){
	case Weapon:
	    return new WEAPON{l,n1,n2,s1,a1,a2,t2,g,r2,w};
	case Drug:
	    return new DRUG{l,n1,n2,h,g,r2,w};
	default:return NULL;
    }
}
*/
#if 0
//第一代工厂
Armor * Factory_All::produceArm(int l,int w2,PARTS p,string n1,string n2,
    int h,int m,float s1,float s2,int a1,int a2,int d,
    int r,int t2,int g,int d2,RARE r2,WEALTH w,int s3){
	switch(p){
	case Hat:
	    return new HAT{l,w2,n1,n2,a1,d,r,t2,g,d2,r2,w,s3};
	case Shirt:
	    return new SHIRT{l,w2,n1,n2,h,s1,d,t2,g,d2,r2,w,s3};
	case Pants:
	    return new PANTS{l,w2,n1,n2,h,d,r,t2,g,d2,r2,w,s3};
	case Belt:
	    return new BELT{l,w2,n1,n2,h,d,r,t2,g,d2,r2,w,s3};
	case Shoes:
	    return new SHOES{l,w2,n1,n2,s2,d,r,t2,g,d2,r2,w,s3};
	default:return NULL;
    }
}
Armor * Factory_All::produceArm(int l,int w2,TYPE t,PARTS p,string n1,string n2,
	int h,int m,float s1,float s2,int a1,int a2,int d,
	    int r,int t2,int g,int d2,RARE r2,WEALTH w,int s3){
	switch(t){
	case Dress:
	    switch(p){
		case Hat:
		    return new HAT{l,w2,n1,n2,a1,d,r,t2,g,d2,r2,w,s3};
		case Shirt:
		    return new SHIRT{l,w2,n1,n2,h,s1,d,t2,g,d2,r2,w,s3};
		case Pants:
		    return new PANTS{l,w2,n1,n2,h,d,r,t2,g,d2,r2,w,s3};
		case Belt:
		    return new BELT{l,w2,n1,n2,h,d,r,t2,g,d2,r2,w,s3};
		case Shoes:
		    return new SHOES{l,w2,n1,n2,s2,d,r,t2,g,d2,r2,w,s3};
		default:return NULL;
	    }
	case Orbament:
	    switch(p){
		case Hat:
		    return new HAT{l,w2,n1,n2,a1,d,r,t2,g,d2,r2,w,s3};
		case Shirt:
		    return new SHIRT{l,w2,n1,n2,h,s1,d,t2,g,d2,r2,w,s3};
		case Pants:
		    return new PANTS{l,w2,n1,n2,h,d,r,t2,g,d2,r2,w,s3};
		case Belt:
		    return new BELT{l,w2,n1,n2,h,d,r,t2,g,d2,r2,w,s3};
		case Shoes:
		    return new SHOES{l,w2,n1,n2,s2,d,r,t2,g,d2,r2,w,s3};
		default:return NULL;
	    }
	case Hand:
	    switch(p){
		case Weapon:
		    return new WEAPON{l,w2,n1,n2,s1,a1,a2,t2,g,d2,r2,w,s3};
		default:return NULL;
	    }
	case Body:
	    switch(p){
		case Drug:
		    return new DRUG{l,w2,n1,n2,h,g,d2,r2,w,s3};
		default:return NULL;
	    }
	default:return NULL;
    }
}
#endif
#if 1
//第二代工厂
/*
Armor * Factory_All::produceArm(int i,int l,int w2,PARTS p,string n1,string n2,
    int h,int m,float s1,float s2,int a1,int a2,int d,
    int r,int t2,int g,int d2,RARE r2,WEALTH w,int s3){
	switch(p){
	case Hat:
	    return new HAT{i,l,w2,n1,n2,a1,d,r,t2,g,d2,r2,w,s3};
	case Shirt:
	    return new SHIRT{i,l,w2,n1,n2,h,s1,d,t2,g,d2,r2,w,s3};
	case Pants:
	    return new PANTS{i,l,w2,n1,n2,h,d,r,t2,g,d2,r2,w,s3};
	case Belt:
	    return new BELT{i,l,w2,n1,n2,h,d,r,t2,g,d2,r2,w,s3};
	case Shoes:
	    return new SHOES{i,l,w2,n1,n2,s2,d,r,t2,g,d2,r2,w,s3};
	default:return NULL;
    }
}
*/
//全能生产部位防具函数,可生产衣服，时装，饰品,防具，武器，药剂
Armor * Factory_All::produceArm(int i,int l,int w2,TYPE t,PARTS p,string n1,string n2,
	int h,int m,float s1,float s2,int a1,int a2,int d,
	    int r,int t2,int g,int d2,RARE r2,WEALTH w,int s3){
	switch(t){
	case Clothes://生产衣服
	    switch(p){
		case Hat:
		    return new HAT{i,l,w2,t,n1,n2,a1,d,r,t2,g,d2,r2,w,s3};
		case Shirt:
		    return new SHIRT{i,l,w2,t,n1,n2,h,s1,d,t2,g,d2,r2,w,s3};
		case Pants:
		    return new PANTS{i,l,w2,t,n1,n2,h,d,r,t2,g,d2,r2,w,s3};
		case Belt:
		    return new BELT{i,l,w2,t,n1,n2,h,d,r,t2,g,d2,r2,w,s3};
		case Shoes:
		    return new SHOES{i,l,w2,t,n1,n2,s2,d,r,t2,g,d2,r2,w,s3};
		default:return NULL;
	    }
	case Dress://生产时装
	    switch(p){
		case Hat:
		    return new HAT{i,l,w2,t,n1,n2,a1,d,r,t2,g,d2,r2,w,s3};
		case Shirt:
		    return new SHIRT{i,l,w2,t,n1,n2,h,s1,d,t2,g,d2,r2,w,s3};
		case Pants:
		    return new PANTS{i,l,w2,t,n1,n2,h,d,r,t2,g,d2,r2,w,s3};
		case Belt:
		    return new BELT{i,l,w2,t,n1,n2,h,d,r,t2,g,d2,r2,w,s3};
		case Shoes:
		    return new SHOES{i,l,w2,t,n1,n2,s2,d,r,t2,g,d2,r2,w,s3};
		default:return NULL;
	    }
	case Orbament://生产饰品
	    switch(p){
		case Hat:
		    return new HAT{i,l,w2,t,n1,n2,a1,d,r,t2,g,d2,r2,w,s3};
		case Shirt:
		    return new SHIRT{i,l,w2,t,n1,n2,h,s1,d,t2,g,d2,r2,w,s3};
		case Pants:
		    return new PANTS{i,l,w2,t,n1,n2,h,d,r,t2,g,d2,r2,w,s3};
		case Belt:
		    return new BELT{i,l,w2,t,n1,n2,h,d,r,t2,g,d2,r2,w,s3};
		case Shoes:
		    return new SHOES{i,l,w2,t,n1,n2,s2,d,r,t2,g,d2,r2,w,s3};
		default:return NULL;
	    }
	case Hand://生产武器
	    switch(p){
		case Weapon:
		    return new WEAPON{i,l,w2,t,n1,n2,s1,a1,a2,t2,g,d2,r2,w,s3};
		default:return NULL;
	    }
	case Body://生产药水
	    switch(p){
		case Drug:
		    return new DRUG{i,l,w2,t,n1,n2,h,g,d2,r2,w,s3};
		default:return NULL;
	    }
	default:return NULL;
    }
}
#endif
void Attribute::show(){
    /*
    cout<<"生命\t"<<"魔法\t"<<"攻速\t"<<"移速"
	<<"普攻\t"<<"魔攻\t"<<"物抗\t"<<"魔抗"<<endl;
    cout<<hp<<"\t"<<mp<<"\t"<<spd<<"\t"
	<<atk<<"\t"<<atm<<"\t"<<def<<"\t"<<res<<endl;
	*/
    cout<<cursor_right2<<"生命值："<<hp<<"\n"<<endl;;
    cout<<cursor_right3<<"魔法值："<<mp<<"\n"<<endl;;
    cout<<cursor_right4<<"攻击速度："<<spd<<"\n"<<endl;;
    cout<<cursor_right5<<"移动速度："<<spd2<<"\n"<<endl;;
    cout<<cursor_right6<<"物理伤害："<<atk<<"\n"<<endl;;
    cout<<cursor_right2<<"魔法伤害："<<atm<<"\n"<<endl;;
    cout<<cursor_right3<<"物理防御："<<def<<"\n"<<endl;;
    cout<<cursor_right4<<"魔法防御："<<res<<"\n"<<endl;;
}
PARTS intput_part(char n){
    PARTS p;
    switch(n){
	case '1':p=Hat;break;
	case '2':p=Shirt;break;
	case '3':p=Pants;break;
	case '4':p=Belt;break;
	case '5':p=Shoes;break;
	case '6':p=Weapon;break;
	case '7':p=Drug;break;
	default :p=Other;break;
    }
    return p;
}
TYPE intput_type(char n){
    switch(n){
	case '1':return Hand;
	case '2':return Body;
	case '3':return Clothes;
	case '4':return Dress;
	case '5':return Orbament;
    }
}
WEALTH intput_wealth(char n){
    switch(n){
	case '1':return Gold;
	case '2':return Dimond;
	case '3':return Points;
	case '4':return Token;
    }
}
RARE intput_rare(char n){
    switch(n){
	case '1':return Common;
	case '2':return Senior;
	case '3':return Smriti;
	case '4':return Relic;
	case '5':return Artifact;
	case '6':return Fame;
	case '7':return Epic;
	case '8':return Limit;
	default :return otheR;
    }
}
string getRare(RARE r){
    string s;
    switch(r){
	case Common:s="普通";break;
	case Senior:s="高级";break;
	case Smriti:s="传承";break;
	case Relic:s="圣物";break;
	case Artifact:s="神器";break;
	case Fame:s="传说";break;
	case Epic:s="史诗";break;
	case Limit:s="限定";break;
	default :s="没有该稀有度";break;
    }
    return s;
}
string getPant(PARTS p){
    string s;
    switch(p){
	case Hat:s="帽子";break;
	case Shirt:s="上衣";break;
	case Pants:s="裤子";break;
	case Belt:s="腰带";break;
	case Shoes:s="鞋子";break;
	case Weapon:s="武器";break;
	case Drug:s="药剂";break;
	default:s="没有该部位";break;
    }
    return s;
}
void HAT::show(){//显示效果
    cout<<cursor_right2<<"装备id:"<<id<<endl;
    cout<<cursor_right2<<"稀有度:"<<getRare(attr.rare)<<endl;
    cout<<cursor_right2<<"耐久度:"<<attr.ten<<endl;
    cout<<cursor_right2<<"装备名称:"<<name<<endl;
    cout<<cursor_right2<<"装备属性:"<<nature<<endl;
    cout<<cursor_right2<<"装备等级:"<<level<<endl;
    cout<<cursor_right2<<"装备重量:"<<weight<<"kg"<<endl;
    cout<<cursor_right2<<"金币售价:"<<getva()<<endl;
    cout<<cursor_right2<<"钻石售价:"<<getdi()<<endl;
    cout<<cursor_right2<<"装备部位:"<<getPant(part)<<endl;
    cout<<cursor_right2<<"物理伤害:+"<<attr.atk<<endl;
    cout<<cursor_right2<<"物理防御:+"<<attr.def<<endl;
    cout<<cursor_right2<<"魔法防御：+"<<attr.res<<endl;
}
void HAT::show2(){//显示效果
    cout<<cursor_right7<<id<<" |"<<getRare(attr.rare)<<" |"<<attr.ten<<" |"<<name<<" |"
	<<level<<" |"<<getva()<<" |"<<getdi()<<" |"<<getPant(part)
	<<"| 0 | 0 | 0 | 0 | "<<attr.atk<<" | 0 |"<<attr.def<<" |"<<attr.res<<endl;
}
void HAT::show3(){//显示效果
    cout<<cursor_right7<<id<<" |"<<attr.ten<<" |"<<name<<" |"
	<<level<<" |"<<getPant(part)
	<<"| 0 | 0 | 0 | 0 | "<<attr.atk<<" | 0 |"<<attr.def<<" |"<<attr.res;
}
void SHIRT::show(){//显示效果
    cout<<cursor_right2<<"装备id:"<<id<<endl;
    cout<<cursor_right2<<"稀有度:"<<getRare(attr.rare)<<endl;
    cout<<cursor_right2<<"耐久度:"<<attr.ten<<endl;
    cout<<cursor_right2<<"装备名称:"<<name<<endl;
    cout<<cursor_right2<<"装备属性:"<<nature<<endl;
    cout<<cursor_right2<<"装备等级:"<<level<<endl;
    cout<<cursor_right2<<"装备重量:"<<weight<<"kg"<<endl;
    cout<<cursor_right2<<"金币售价:"<<getva()<<endl;
    cout<<cursor_right2<<"钻石售价:"<<getdi()<<endl;
    cout<<cursor_right2<<"装备部位:"<<getPant(part)<<endl;
    cout<<cursor_right2<<"生命值:+"<<attr.hp<<endl;
    cout<<cursor_right2<<"攻击速度:+"<<attr.spd<<endl;
    cout<<cursor_right2<<"物理防御：+"<<attr.def<<endl;
}
void SHIRT::show2(){//显示效果
    cout<<cursor_right7<<id<<" |"<<getRare(attr.rare)<<" |"<<attr.ten<<" |"<<name<<" |"
	<<level<<" |"<<getva()<<" |"<<getdi()<<" |"<<getPant(part)<<"|"
	<<attr.hp<<"| 0 | "<<attr.spd<<" | 0 | 0 | 0 | "<<attr.def<<" |0"<<endl;
}
void SHIRT::show3(){//显示效果
    cout<<cursor_right7<<id<<" |"<<attr.ten<<" |"<<name<<" |"
	<<level<<" |"<<getPant(part)<<"|"
	<<attr.hp<<"| 0 | "<<attr.spd<<" | 0 | 0 | 0 | "<<attr.def<<" |0";
}
void PANTS::show(){//显示效果
    cout<<cursor_right2<<"装备id:"<<id<<endl;
    cout<<cursor_right2<<"稀有度:"<<getRare(attr.rare)<<endl;
    cout<<cursor_right2<<"耐久度:"<<attr.ten<<endl;
    cout<<cursor_right2<<"装备名称:"<<name<<endl;
    cout<<cursor_right2<<"装备属性:"<<nature<<endl;
    cout<<cursor_right2<<"装备等级:"<<level<<endl;
    cout<<cursor_right2<<"装备重量:"<<weight<<"kg"<<endl;
    cout<<cursor_right2<<"金币售价:"<<getva()<<endl;
    cout<<cursor_right2<<"钻石售价:"<<getdi()<<endl;
    cout<<cursor_right2<<"装备部位:"<<getPant(part)<<endl;
    cout<<cursor_right2<<"生命值:+"<<attr.hp<<endl;
    cout<<cursor_right2<<"物理防御:+"<<attr.def<<endl;
    cout<<cursor_right2<<"魔法防御:+"<<attr.res<<endl;
}
void PANTS::show2(){//显示效果
    cout<<cursor_right7<<id<<" |"<<getRare(attr.rare)<<" |"<<attr.ten<<" |"<<name<<" |"
	<<level<<" |"<<getva()<<" |"<<getdi()<<" |"<<getPant(part)<<"|"
	<<attr.hp<<"| 0 | 0 | 0 | 0 | 0 | "<<attr.def<<" | "<<attr.res<<endl;
}
void PANTS::show3(){//显示效果
    cout<<cursor_right7<<id<<" |"<<attr.ten<<" |"<<name<<" |"
	<<level<<" |"<<getPant(part)<<"|"
	<<attr.hp<<"| 0 | 0 | 0 | 0 | 0 | "<<attr.def<<" | "<<attr.res;
}
void BELT::show(){//显示效果
    cout<<cursor_right2<<"装备id:"<<id<<endl;
    cout<<cursor_right2<<"稀有度:"<<getRare(attr.rare)<<endl;
    cout<<cursor_right2<<"耐久度:"<<attr.ten<<endl;
    cout<<cursor_right2<<"装备名称:"<<name<<endl;
    cout<<cursor_right2<<"装备属性:"<<nature<<endl;
    cout<<cursor_right2<<"装备等级:"<<level<<endl;
    cout<<cursor_right2<<"装备重量:"<<weight<<"kg"<<endl;
    cout<<cursor_right2<<"金币售价:"<<getva()<<endl;
    cout<<cursor_right2<<"钻石售价:"<<getdi()<<endl;
    cout<<cursor_right2<<"装备部位:"<<getPant(part)<<endl;
    cout<<cursor_right2<<"生命值:+"<<attr.hp<<endl;
    cout<<cursor_right2<<"物理防御:+"<<attr.def<<endl;
    cout<<cursor_right2<<"魔法防御:+"<<attr.res<<endl;
}
void BELT::show2(){//显示效果
    cout<<cursor_right7<<id<<" |"<<getRare(attr.rare)<<" |"<<attr.ten<<" |"<<name<<" |"
	<<level<<" |"<<getva()<<" |"<<getdi()<<" |"<<getPant(part)<<"|"
	<<attr.hp<<"| 0 | 0 | 0 | 0 | 0 |"<<attr.def<<" |"<<attr.res<<endl;
}
void BELT::show3(){//显示效果
    cout<<cursor_right7<<id<<" |"<<attr.ten<<" |"<<name<<" |"
	<<level<<" |"<<getPant(part)<<"|"
	<<attr.hp<<"| 0 | 0 | 0 | 0 | 0 |"<<attr.def<<" |"<<attr.res;
}
void SHOES::show(){//显示效果
    cout<<cursor_right2<<"装备id:"<<id<<endl;
    cout<<cursor_right2<<"稀有度:"<<getRare(attr.rare)<<endl;
    cout<<cursor_right2<<"耐久度:"<<attr.ten<<endl;
    cout<<cursor_right2<<"装备名称:"<<name<<endl;
    cout<<cursor_right2<<"装备属性:"<<nature<<endl;
    cout<<cursor_right2<<"装备等级:"<<level<<endl;
    cout<<cursor_right2<<"装备重量:"<<weight<<"kg"<<endl;
    cout<<cursor_right2<<"金币售价:"<<getva()<<endl;
    cout<<cursor_right2<<"钻石售价:"<<getdi()<<endl;
    cout<<cursor_right2<<"装备部位:"<<getPant(part)<<endl;
    cout<<cursor_right2<<"移动速度:+"<<attr.spd2<<endl;
    cout<<cursor_right2<<"物理防御:+"<<attr.def<<endl;
    cout<<cursor_right2<<"魔法防御:+"<<attr.res<<endl;
}
void SHOES::show2(){//显示效果
    cout<<cursor_right7<<id<<" |"<<getRare(attr.rare)<<" |"<<attr.ten<<" |"<<name<<" |"
	<<level<<" |"<<getva()<<" |"<<getdi()<<" |"<<getPant(part)<<"| "
	<<"0 | 0 | 0 | "<<attr.spd2<<" | 0 | 0 | "<<attr.def<<" |"<<attr.res<<endl;
}
void SHOES::show3(){//显示效果
    cout<<cursor_right7<<id<<" |"<<attr.ten<<" |"<<name<<" |"
	<<level<<" |"<<getPant(part)<<"| "
	<<"0 | 0 | 0 | "<<attr.spd2<<" | 0 | 0 | "<<attr.def<<" |"<<attr.res;
}
void WEAPON::show(){//显示效果
    cout<<cursor_right2<<"装备id:"<<id<<endl;
    cout<<cursor_right2<<"稀有度:"<<getRare(attr.rare)<<endl;
    cout<<cursor_right2<<"耐久度:"<<attr.ten<<endl;
    cout<<cursor_right2<<"装备名称:"<<name<<endl;
    cout<<cursor_right2<<"装备属性:"<<nature<<endl;
    cout<<cursor_right2<<"装备等级:"<<level<<endl;
    cout<<cursor_right2<<"装备重量:"<<weight<<"kg"<<endl;
    cout<<cursor_right2<<"金币售价:"<<getva()<<endl;
    cout<<cursor_right2<<"钻石售价:"<<getdi()<<endl;
    cout<<cursor_right2<<"装备部位:"<<getPant(part)<<endl;
    cout<<cursor_right2<<"攻击速度:+"<<attr.spd<<endl;
    cout<<cursor_right2<<"物理伤害:+"<<attr.atk<<endl;
    cout<<cursor_right2<<"魔法伤害:+"<<attr.atm<<endl;
}
void WEAPON::show2(){//显示效果
    cout<<cursor_right7<<id<<" |"<<getRare(attr.rare)<<" |"<<attr.ten<<" |"<<name<<" |"
	<<level<<" |"<<getva()<<" |"<<getdi()<<" |"<<getPant(part)<<"|"
	<<" 0 | 0 | "<<attr.spd<<" | 0 |"<<attr.atk<<"| "<<attr.atm<<"| 0 |0"<<endl;
}
void WEAPON::show3(){//显示效果
    cout<<cursor_right7<<id<<" |"<<attr.ten<<" |"<<name<<" |"
	<<level<<" |"<<getPant(part)<<"|"
	<<" 0 | 0 | "<<attr.spd<<" | 0 |"<<attr.atk<<"| "<<attr.atm<<"| 0 |0";
}
void DRUG::show(){//显示效果
    cout<<cursor_right2<<"装备id:"<<id<<endl;
    cout<<cursor_right2<<"稀有度:"<<getRare(attr.rare)<<endl;
//    cout<<"耐久度:"<<attr.ten<<endl;
    cout<<cursor_right2<<"装备名称:"<<name<<endl;
    cout<<cursor_right2<<"装备属性:"<<nature<<endl;
    cout<<cursor_right2<<"装备等级:"<<level<<endl;
    cout<<cursor_right2<<"装备重量:"<<weight<<"kg"<<endl;
    cout<<cursor_right2<<"金币售价:"<<getva()<<endl;
    cout<<cursor_right2<<"钻石售价:"<<getdi()<<endl;
    cout<<cursor_right2<<"装备部位:"<<getPant(part)<<endl;
    cout<<cursor_right2<<"生命值:+"<<attr.hp<<endl;
}
void DRUG::show2(){//显示效果
    cout<<cursor_right7<<id<<" |"<<getRare(attr.rare)<<" |"<<attr.ten<<" |"<<name<<" |"
	<<level<<" |"<<getva()<<" |"<<getdi()<<" |"<<getPant(part)<<"|"
	<<attr.hp<<"|"<<attr.mp<<"| 0 | 0 | 0 | 0 | 0 | 0"<<endl;
}
void DRUG::show3(){//显示效果
    cout<<cursor_right7<<id<<" |"<<attr.ten<<" |"<<name<<" |"
	<<level<<" |"<<getPant(part)<<"|"
	<<attr.hp<<"|"<<attr.mp<<"| 0 | 0 | 0 | 0 | 0 | 0";
}
