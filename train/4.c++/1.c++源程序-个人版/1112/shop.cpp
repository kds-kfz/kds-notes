#include"shop.h"
//shop.cpp
/*
   enum Goods{
   HAT,SHIRT,PANTS,BELT,SHOES,WEAPON,
   CLOTHES,DRESS,ORBAMENTA
   };
 */
int Actor::count=1000;//初始化部位防具id
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
Armor * Factory_All::produceArm(int l,int w2,PARTS p,string n1,string n2,
    int h,int m,float s1,float s2,int a1,int a2,int d,
    int r,int t2,int g,int d2,RARE r2,WEALTH w){
	switch(p){
	case Hat:
	    return new HAT{l,w2,n1,n2,a1,d,r,t2,g,d2,r2,w};
	case Shirt:
	    return new SHIRT{l,w2,n1,n2,h,s1,d,t2,g,d2,r2,w};
	case Pants:
	    return new PANTS{l,w2,n1,n2,h,d,r,t2,g,d2,r2,w};
	case Belt:
	    return new BELT{l,w2,n1,n2,h,d,r,t2,g,d2,r2,w};
	case Shoes:
	    return new SHOES{l,w2,n1,n2,s2,d,r,t2,g,d2,r2,w};
	default:return NULL;
    }
}
Armor * Factory_All::produceArm(int l,int w2,TYPE t,PARTS p,string n1,string n2,
	int h,int m,float s1,float s2,int a1,int a2,int d,
	    int r,int t2,int g,int d2,RARE r2,WEALTH w){
	switch(t){
	case Dress:
	    switch(p){
		case Hat:
		    return new HAT{l,w2,n1,n2,a1,d,r,t2,g,d2,r2,w};
		case Shirt:
		    return new SHIRT{l,w2,n1,n2,h,s1,d,t2,g,d2,r2,w};
		case Pants:
		    return new PANTS{l,w2,n1,n2,h,d,r,t2,g,d2,r2,w};
		case Belt:
		    return new BELT{l,w2,n1,n2,h,d,r,t2,g,d2,r2,w};
		case Shoes:
		    return new SHOES{l,w2,n1,n2,s2,d,r,t2,g,d2,r2,w};
		default:return NULL;
	    }
	case Orbament:
	    switch(p){
		case Hat:
		    return new HAT{l,w2,n1,n2,a1,d,r,t2,g,d2,r2,w};
		case Shirt:
		    return new SHIRT{l,w2,n1,n2,h,s1,d,t2,g,d2,r2,w};
		case Pants:
		    return new PANTS{l,w2,n1,n2,h,d,r,t2,g,d2,r2,w};
		case Belt:
		    return new BELT{l,w2,n1,n2,h,d,r,t2,g,d2,r2,w};
		case Shoes:
		    return new SHOES{l,w2,n1,n2,s2,d,r,t2,g,d2,r2,w};
		default:return NULL;
	    }
	case Hand:
	    switch(p){
		case Weapon:
		    return new WEAPON{l,w2,n1,n2,s1,a1,a2,t2,g,d2,r2,w};
		default:return NULL;
	    }
	case Body:
	    switch(p){
		case Drug:
		    return new DRUG{l,w2,n1,n2,h,g,d2,r2,w};
		default:return NULL;
	    }
	default:return NULL;
    }
}
void Attribute::show(){
    cout<<"生命\t"<<"魔法\t"<<"攻速\t"
	<<"普攻\t"<<"魔攻\t"<<"物抗\t"<<"魔抗"<<endl;
    cout<<hp<<"\t"<<mp<<"\t"<<spd<<"\t"
	<<atk<<"\t"<<atm<<"\t"<<def<<"\t"<<res<<endl;
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
    cout<<"装备id:"<<id<<endl;
    cout<<"稀有度:"<<getRare(attr.rare)<<endl;
    cout<<"耐久度:"<<attr.ten<<endl;
    cout<<"装备名称:"<<name<<endl;
    cout<<"装备属性:"<<nature<<endl;
    cout<<"装备等级:"<<level<<endl;
    cout<<"装备重量:"<<weight<<"kg"<<endl;
    cout<<"金币售价:"<<getva()<<endl;
    cout<<"钻石售价:"<<getdi()<<endl;
    cout<<"装备部位:"<<getPant(part)<<endl;
    cout<<"物理伤害:+"<<attr.atk<<endl;
    cout<<"物理防御:+"<<attr.def<<endl;
    cout<<"魔法防御：+"<<attr.res<<endl;
}
void SHIRT::show(){//显示效果
    cout<<"装备id:"<<id<<endl;
    cout<<"稀有度:"<<getRare(attr.rare)<<endl;
    cout<<"耐久度:"<<attr.ten<<endl;
    cout<<"装备名称:"<<name<<endl;
    cout<<"装备属性:"<<nature<<endl;
    cout<<"装备等级:"<<level<<endl;
    cout<<"装备重量:"<<weight<<"kg"<<endl;
    cout<<"金币售价:"<<getva()<<endl;
    cout<<"钻石售价:"<<getdi()<<endl;
    cout<<"装备部位:"<<getPant(part)<<endl;
    cout<<"生命值:+"<<attr.hp<<endl;
    cout<<"攻击速度:+"<<attr.spd<<endl;
    cout<<"物理防御：+"<<attr.def<<endl;
}
void PANTS::show(){//显示效果
    cout<<"装备id:"<<id<<endl;
    cout<<"稀有度:"<<getRare(attr.rare)<<endl;
    cout<<"耐久度:"<<attr.ten<<endl;
    cout<<"装备名称:"<<name<<endl;
    cout<<"装备属性:"<<nature<<endl;
    cout<<"装备等级:"<<level<<endl;
    cout<<"装备重量:"<<weight<<"kg"<<endl;
    cout<<"金币售价:"<<getva()<<endl;
    cout<<"钻石售价:"<<getdi()<<endl;
    cout<<"装备部位:"<<getPant(part)<<endl;
    cout<<"生命值:+"<<attr.hp<<endl;
    cout<<"物理防御:+"<<attr.def<<endl;
    cout<<"魔法防御:+"<<attr.res<<endl;
}
void BELT::show(){//显示效果
    cout<<"装备id:"<<id<<endl;
    cout<<"稀有度:"<<getRare(attr.rare)<<endl;
    cout<<"耐久度:"<<attr.ten<<endl;
    cout<<"装备名称:"<<name<<endl;
    cout<<"装备属性:"<<nature<<endl;
    cout<<"装备等级:"<<level<<endl;
    cout<<"装备重量:"<<weight<<"kg"<<endl;
    cout<<"金币售价:"<<getva()<<endl;
    cout<<"钻石售价:"<<getdi()<<endl;
    cout<<"装备部位:"<<getPant(part)<<endl;
    cout<<"生命值:+"<<attr.hp<<endl;
    cout<<"物理防御:+"<<attr.def<<endl;
    cout<<"魔法防御:+"<<attr.res<<endl;
}
void SHOES::show(){//显示效果
    cout<<"装备id:"<<id<<endl;
    cout<<"稀有度:"<<getRare(attr.rare)<<endl;
    cout<<"耐久度:"<<attr.ten<<endl;
    cout<<"装备名称:"<<name<<endl;
    cout<<"装备属性:"<<nature<<endl;
    cout<<"装备等级:"<<level<<endl;
    cout<<"装备重量:"<<weight<<"kg"<<endl;
    cout<<"金币售价:"<<getva()<<endl;
    cout<<"钻石售价:"<<getdi()<<endl;
    cout<<"装备部位:"<<getPant(part)<<endl;
    cout<<"移动速度:+"<<attr.spd2<<endl;
    cout<<"物理防御:+"<<attr.def<<endl;
    cout<<"魔法防御:+"<<attr.res<<endl;
}
void WEAPON::show(){//显示效果
    cout<<"装备id:"<<id<<endl;
    cout<<"稀有度:"<<getRare(attr.rare)<<endl;
    cout<<"耐久度:"<<attr.ten<<endl;
    cout<<"装备名称:"<<name<<endl;
    cout<<"装备属性:"<<nature<<endl;
    cout<<"装备等级:"<<level<<endl;
    cout<<"装备重量:"<<weight<<"kg"<<endl;
    cout<<"金币售价:"<<getva()<<endl;
    cout<<"钻石售价:"<<getdi()<<endl;
    cout<<"装备部位:"<<getPant(part)<<endl;
    cout<<"攻击速度:+"<<attr.spd<<endl;
    cout<<"物理伤害:+"<<attr.atk<<endl;
    cout<<"魔法伤害:+"<<attr.atm<<endl;
}
void DRUG::show(){//显示效果
    cout<<"装备id:"<<id<<endl;
    cout<<"稀有度:"<<getRare(attr.rare)<<endl;
//    cout<<"耐久度:"<<attr.ten<<endl;
    cout<<"装备名称:"<<name<<endl;
    cout<<"装备属性:"<<nature<<endl;
    cout<<"装备等级:"<<level<<endl;
    cout<<"装备重量:"<<weight<<"kg"<<endl;
    cout<<"金币售价:"<<getva()<<endl;
    cout<<"钻石售价:"<<getdi()<<endl;
    cout<<"装备部位:"<<getPant(part)<<endl;
    cout<<"生命值:+"<<attr.hp<<endl;
}
