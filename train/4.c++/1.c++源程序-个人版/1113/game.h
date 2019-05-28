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
//	void Loginok(map<string,Player *>::itertor p);//不能传容器指针

	int run();
	void test();
//	void buy_goods();
	void produce_goods();
	void show_goods();
	void shop();
	void buy_goods();
	void seek_good();
};

//其它函数声明
//主界面
void chTost();

#endif
