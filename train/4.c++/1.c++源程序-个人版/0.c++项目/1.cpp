表格
1.所有装备 每一种装备有自己的id 固定属性效果
2.玩家 自己信息 身上的装备 存储装备id 耐久度
3.背包 所有物品 id 个数 耐久度
-----------------------------------
玩家装备

struct Attribute{
    hp;	    1是必须包含的
    mp;
    spd;
    atk;//普通攻击1
    atm;//魔法
    def;//物理防御1
    res;//魔法防御
}
//读数据库，保存
class Player{
    int id;
    string name;
    int gold;
    int exp;
    int level;
    int diamond;//钻石
    Attribute attr1;//玩家自身的状态,随着等级提高
    Attribute attr2;//所有加成状态
    public:
    //玩家换装备的函数，包含衣服，时装和首饰 
    void changeEqu(const Equipment &eq){}
    //移除装备
    removeEqu
    //穿着
    addEqu
    //换装被，然后更新状态
    void useProp(const Prop &pr){}
    //显示自己状态
    void showAttr(){
	//输出各种自身的状态加上加成的状态
    }//原有状态+装备
};
class Actor{//作用物,物品
    public:
    virtual void show()=0;
};
class Prop : public Actor{//道具
    virtual void changeState(Player &p)=0;
};
class Equipment : public Actor{//装备
    enum PART{//部位
//	    1	    2	3     4
	Hat,Shirt,Pants,Belt,Shoes,Weapon ...
//	帽子，衬衣，裤子，腰带，鞋子，武器
    };
    enum Type{//装备类型
	Clothes,//普通的装备
	Dress,//时装
	Orbament,//首饰
	...
    };
    Attribute attr;//属性
};
class Blood : public Prop{//血量
    //增加玩家血量
    void changeState(Player &p){}
};

