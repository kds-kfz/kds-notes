#ifndef _GAME_H
#define _GAME_H
#include<map>
#include"player.h"

class Game{
    map<string,Player *>player;
    map<int,pair<Actor *,int> >goods;//id,商品,数量
    public:
	void Begin();
	void Register();
	void Login();
	void show_allplayer();

	//运行游戏
	int run();
	void test();
	//生产商品
	void produce_goods();
	//显示所有商品
	void show_goods();
	//
	void shop(map<string,Player *>::iterator r);
	void buy_goods(map<string,Player *>::iterator r);
	void buy_good();
	void seek_good(char p[],char l[],char r[],char g[]);
	void pay(char w,map<string,Player *>::iterator r,
		map<int,pair<Actor *,int> >::iterator r2);
	void show_bag(map<string,Player *>::iterator p);
	void show_equip();
};

//其它函数声明
//char转数字
int chToint(char p[]);
//char转string
void chTost(string &s1,char *s2);
//选择位置输入
void choice_input(int up,int right,char arr[]);
//普通输入
void common_input(int up,int down,int right,char []);


#endif
