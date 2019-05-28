玩家装备模块


struct Attribute //各种属性
{
1	hp;		1是必须包含的	
	mp;
	spd;
1	atk; //普通攻击
	atm; //魔法
1	def; //物理防御
	res; //魔法防御
	...
};
class Player 
{
	int id;
	string name;
	int gold;
	int diamond;//钻石
	int exp;
	int level;
	Attribute attr1;//玩家自身的原始状态,随着等级提高
	Attribute attr2;//所有加成状态
public:
	//玩家换装备的函数, 包含衣服，时装和首饰
	void changeEqu(const Equipment& eq) {}
	//移除装备
	removeEqu
	//穿着
	addEqu
	void useProp(const Prop& pr) { 换装备，然后更新加成状态}
	void showAttr() { 
		输出各种自身的状态加上加成的状态
	}
};
class Actor //物品
{
public:
	virtual void show() = 0;
};
class Prop : public Actor　//道具
{
	virtual void changeStat(Player& p) = 0;
};
class Equipment : public Actor //装备
{
	enum PART {	
		Hat,
	 1	Shirt,	数字是必须有的
	 2	Pants,
	 3	Belt,
	 4	Shoes,	
		Weapon,
		...
	};
	//装备类型
	enum Type {
		Clothes,	//普通的装备
		Dress,		//时装
		Ornament	//首饰
		...
	};
	Attribute attr;
};

class Blood : public Prop
{
	void changeStat(Player& p) {
		p增加血量
	}
}


