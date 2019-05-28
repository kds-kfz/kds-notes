#include<unistd.h>
#include"player.h"
#include"game.h"
#include"shop.h"
#include"font.h"


//游戏开始
int Game::run(){
    time_t t;
    struct tm *showTime;
    while(1){
    system("clear");
    display_cheek(26,86);
    fun_cursor_up(23);
    time(&t);
    showTime=localtime(&t);
    cout<<cursor_right1<<"玩家装备游戏\n"<<endl;
    cout<<cursor_right6<<"游戏版本:v1.112\n"<<endl;
    cout<<cursor_right5<<"游戏时间:"
    #if 0
	<<showTime->tm_year+1900<<"/"
	<<showTime->tm_mon+1<<"/"
	<<showTime->tm_mday<<" "
    #endif
	<<showTime->tm_hour<<":"
	<<showTime->tm_min<<":"
	<<showTime->tm_sec<<endl<<endl;
    cout<<cursor_right2<<"1.开始游戏\n"<<endl;
    cout<<cursor_right3<<"2.退出游戏\n"<<endl;
    cout<<cursor_right4<<"输入您的选择:";
    char choose=menu_choose(20);
    switch(choose){
	case '1':break;
	case '2':exit(-1);
	default:cout<<cursor_right1<<"error\n";sleep(1);break;
    }
    }
    return 0;
}

void Game::test(){
    HAT hat(5,10,"紫金头盔","增加护甲与攻速",60,100,40,2000,4000,30,Senior,Gold);
    WEAPON weap(10,50,"狙击步枪","增加攻击与攻速",0.7,300,100,3000,6500,55,Fame,Gold);
    DRUG drug(15,25,"死者苏醒","增加大量生命值",500,7000,60,Artifact,Gold);
    hat.show();
    cout<<"-------------------------------------------------------"<<endl;
    weap.show();
    cout<<"-------------------------------------------------------"<<endl;
    drug.show();
}

void Game::show_goods(){
    map<pair<int,Actor *>,int>goods;//id,商品,数量

    Factory_All ap;
    Armor *p1=ap.produceArm(
	    10,15,Hat,"圣战头盔","增加攻击物抗和魔抗，普通",
	    0,0,0,0,60,0,100,40,2000,4000,30,Common,Gold);
    Armor *p2=ap.produceArm(
	    10,15,Shirt,"荣光夹克","增加生命攻速和物抗，高级",
	    100,0,0.3,0,0,0,60,0,1800,4500,35,Senior,Gold);
    Armor *p3=ap.produceArm(
	    10,15,Pants,"海神长裤","增加生命物抗和魔抗，传承",
	    200,0,0,0,0,0,70,50,2300,5000,40,Smriti,Gold);
    Armor *p4=ap.produceArm(
	    10,15,Belt,"龙麟腰带","增加生命物抗和魔抗，圣物",
	    300,0,0,0,0,0,100,100,2400,5500,45,Relic,Gold);
    Armor *p5=ap.produceArm(
	    10,15,Shoes,"忍者足具","增加移速物抗和魔抗，神器",
	    0,0,0,60,0,0,50,40,1900,6000,50,Artifact,Gold);

//等级+时装+部位+名字+描述+生命+蓝+攻速+移速+物伤+魔伤+物抗+魔抗+耐久+造价+稀有+价值
//level+TYPE+PARTS+name+nature+hp+mp+spd+spd2+atk+atm+def+res+ten+Value+RARE+WEALTH

/************************************************************************时装+部位*/
//帽子：等级+时装+部位+名字+描述+0+0+0+0+物伤+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p6=ap.produceArm(
	    20,25,Dress,Hat,"普通头盔","增加攻击物抗和魔抗，普通",
	    0,0,0,0,60,0,100,40,2000,4000,30,Common,Gold);
//衬衫：等级+时装+部位+名字+描述+生命+0+攻速+0+0+0+物抗+0+耐久+造价+稀有+价值
    Armor *p7=ap.produceArm(
	    20,25,Dress,Shirt,"上级衬衫","增加生命攻速和物抗，高级",
	    100,0,0.3,0,0,0,60,0,1800,4500,35,Senior,Gold);
//裤子：等级+时装+部位+名字+描述+生命+0+0+0+0+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p8=ap.produceArm(
	    20,25,Dress,Pants,"天空长裤","增加生命物抗和魔抗，传承",
	    200,0,0,0,0,0,70,50,2300,5000,40,Smriti,Gold);
//腰带：等级+时装+部位+名字+描述+生命+0+0+0+0+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p9=ap.produceArm(
	    20,25,Dress,Belt,"天空腰带","增加生命物抗和魔抗，圣物",
	    300,0,0,0,0,0,100,100,2400,5500,45,Relic,Gold);
//鞋子：等级+时装+部位+名字+描述+0+0+0+移速+0+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p10=ap.produceArm(
	    20,25,Dress,Shoes,"天空足具","增加移速物抗和魔抗，神器",
	    0,0,0,60,0,0,50,40,1900,6000,50,Artifact,Gold);

/************************************************************************饰品+部位*/
//帽子：等级+时装+部位+名字+描述+0+0+0+0+物伤+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p11=ap.produceArm(
	    30,35,Orbament,Hat,"普通发带","增加攻击物抗和魔抗，普通",
	    0,0,0,0,60,0,100,40,2000,4000,30,Common,Gold);
//衬衫：等级+时装+部位+名字+描述+生命+0+攻速+0+0+0+物抗+0+耐久+造价+稀有+价值
    Armor *p12=ap.produceArm(
	    30,35,Orbament,Shirt,"宝石横溢","增加生命攻速和物抗，高级",
	    100,0,0.3,0,0,0,60,0,1800,4500,35,Senior,Gold);
//裤子：等级+时装+部位+名字+描述+生命+0+0+0+0+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p13=ap.produceArm(
	    30,35,Orbament,Pants,"西域牛仔","增加生命物抗和魔抗，传承",
	    200,0,0,0,0,0,70,50,2300,5000,40,Smriti,Gold);
//腰带：等级+时装+部位+名字+描述+生命+0+0+0+0+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p14=ap.produceArm(
	    30,35,Orbament,Belt,"紫金腰带","增加生命物抗和魔抗，圣物",
	    300,0,0,0,0,0,100,100,2400,5500,45,Relic,Gold);
//鞋子：等级+时装+部位+名字+描述+0+0+0+移速+0+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p15=ap.produceArm(
	    30,35,Orbament,Shoes,"恶灵魔足","增加移速物抗和魔抗，神器",
	    0,0,0,60,0,0,50,40,1900,6000,50,Artifact,Gold);

/************************************************************************装备-药剂*/
    Armor *p16=ap.produceArm(
	    30,35,Hand,Weapon,"绝世好剑","增加攻速物攻魔伤，传说",
	    0,0,0.7,0,300,100,0,0,3000,6500,55,Fame,Gold);
    Armor *p17=ap.produceArm(
	    30,35,Body,Drug,"死者苏醒","增加大量生命值，史诗",
	    500,0,0,0,0,0,0,0,0,7000,60,Epic,Gold);
    goods.insert({{p1->getid(),p1},1});
    goods.insert({{p2->getid(),p2},1});
    goods.insert({{p3->getid(),p3},1});
    goods.insert({{p4->getid(),p4},1});
    goods.insert({{p5->getid(),p5},1});
    goods.insert({{p6->getid(),p6},1});
    goods.insert({{p7->getid(),p7},1});
    goods.insert({{p8->getid(),p8},1});
    goods.insert({{p9->getid(),p9},1});
    goods.insert({{p10->getid(),p10},1});
    goods.insert({{p11->getid(),p11},1});
    goods.insert({{p12->getid(),p12},1});
    goods.insert({{p13->getid(),p13},1});
    goods.insert({{p14->getid(),p14},1});
    goods.insert({{p15->getid(),p15},1});
    goods.insert({{p16->getid(),p16},1});
    goods.insert({{p17->getid(),p17},1});
    
    cout<<"\t\t---道具商城---"<<endl;
    cout<<"道具id\t"<<"道具名称\t"<<"等级\t"<<"重量\t"
	<<"金币\t"<<"钻石\t"<<"数量\t"<<"效果"<<endl;
    for(auto m:goods){
	cout<<m.first.first<<"\t"<<m.first.second->getn1()<<"\t"
	    <<m.first.second->getle()<<"\t"<<m.first.second->getwe()<<"\t"
	    <<m.first.second->getva()<<"\t"<<m.first.second->getdi()<<"\t"
	    <<m.second<<"\t"<<m.first.second->getn2()<<endl;
    }
}

