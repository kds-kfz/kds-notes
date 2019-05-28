#ifndef _GAME_H
#define _GAME_H
#include<map>
#include"player.h"
#include"shop.h"

#if 0
//游戏参数GAME设置
GAME 1：成语运行方式1，不读数据库
GAME 2：创数据库goods,player,bag表格
GAME 3：程序运行方式2，读取数据库
#endif

#define GAME 2

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
		map<int,pair<Actor *,int> >::iterator r2,int n);
	void show_bag(map<string,Player *>::iterator p);
	void show_equip(map<string,Player *>::iterator p);
	void show_equip_num(map<string,Player *>::iterator p);
	void game_clear_cursor(int x,int y,int row,int col);
	void function(map<string,Player *>::iterator p,
		int *a,int *b,char *c,int e,int num);
};

//其它函数声明
//char转数字
int chToint(char p[]);
//char转string
void chTost(string &s1,char *s2);
void chTost2(string &s1,char *s2);
//选择位置输入
void choice_input(int up,int right,char arr[]);
//普通输入
void common_input(int up,int down,int right,char []);
//字符串中提取字符
void ChToch(char *a,char *b);

TYPE change1(int n);
PARTS change2(int n);
RARE change3(int n);
WEALTH change4(int n);

//读取数据库godds表格数据，传入goods容器
void read_goods_table(map<int,pair<Actor *,int> >&g);
//读取数据库player表格数据，传入player容器
void read_player_table(map<string,Player *>&p);
//读取数据库bag表格数据，传入player容器
void read_bag_table(map<string,Player *>&p,map<int,pair<Actor *,int> >g);

#endif
