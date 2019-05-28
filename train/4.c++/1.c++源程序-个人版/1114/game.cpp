#include<unistd.h>
#include<cstring>
//#include"player.h"
#include"game.h"
#include"shop.h"
#include"font.h"
using namespace std;
int chToin(char p[]){
    int num=0;
    for(int i=0;p[i]!='\0';i++){
	if(p[i]>='0'&&p[i]<='9'){
	    num*=10;
	    num+=p[i]-48;
	}
    }
    return num;
}
void chTost(string &s1,char *s2){
    int i=0;
    s1.resize(0);
    for(;*s2!='\0';s2++,i++)
	s1+=*s2;
    s1[i]='\0';
}
void choice_input(int up,int right,char arr[]){
    for(int i=0;i<sizeof(&arr);i++)
	arr[i]='\0';
    fun_cursor_up(up);
    fun_cursor_right(right);
    clear_cursor(sizeof(&arr));
    fun_cursor_left(sizeof(&arr));
    cin>>arr;
    menu_choose(20);
}
void common_input(int up,int down,int right,char arr[]){

}
//void Game::Loginok(map<string,Player *>::iterator p){
//    int flag;
//    char buf[20]={};

void Game::Register(){
    while(1){
    interface(26,86,23);
    string name1;
    string key1;
    string choose1;
    
    char name2[20]={};
    char key2[20]={};
    char choose2[20]={};

    int flag;
    int fg=1;
    cout<<cursor_right2<<"玩家注册界面\n"<<endl;
    cout<<cursor_right3<<"新帐号:";
    fixed_input(name2,20,&flag);
    cout<<cursor_right4<<"新密码:";
    fixed_input(key2,20,&flag);

    chTost(name1,name2);
    chTost(key1,key2);

    Player *pl=new Player{name1,key1};
    
    player.insert({pl->getna(),pl});
    cout<<cursor_right4<<"玩家注册成功\n"<<endl;

    cout<<cursor_right4<<"1.注册新玩家"<<endl;
    cout<<cursor_right4<<"2.退出"<<endl;
    cout<<cursor_right2<<"请输入您的选择:";
    while(fg){
	char choose=fixed_input1(choose2,20,&flag);
	if(choose=='1') fg=0;
	else if(choose=='2') return;
	else{
	    cout<<cursor_right1<<"error\n";sleep(1);return;
	    return;
	    }
	}
    }
}

void Game::show_allplayer(){
    interface(26,86,23);
    cout<<cursor_right4<<"所有玩家信息\n"<<endl;
    cout<<message_cursor1<<"id号\t"<<"帐号\t"<<"密码\t"<<"等级\t"<<"经验\t"
	<<"负重\t"<<"金币\t"<<"钻石\t"<<"QB\t"<<"代币"<<endl;
    for(auto m:player){
	cout<<message_cursor2
<<"-------------------------------------------------------------------------"<<endl;
	cout<<message_cursor3
	    <<m.second->getid()<<"\t"<<m.first<<"\t"<<m.second->getke()<<"\t"
	    <<m.second->getle()<<"\t"<<m.second->getex()<<"\t"
	    <<m.second->getwe()<<"\t"<<m.second->getgo()<<"\t"
	    <<m.second->getdi()<<"\t"<<m.second->getqb()<<"\t"
	    <<m.second->getto()<<endl;
    }
    sleep(5);
}

void Game::Login(){
    interface(26,86,23);
    int flag;
    char name2[20]={};
    char key2[20]={};
    string name1;
    string key1;
    cout<<cursor_right2<<"玩家登录界面\n"<<endl;
    cout<<cursor_right3<<"帐号:";
    fixed_input(name2,20,&flag);
    chTost(name1,name2);
    
    map<string,Player *>::iterator p1=player.find(name1);
    if(p1!=player.end()){//找到帐号
	cout<<cursor_right3<<"帐号存在"<<endl;
	cout<<cursor_right4<<"密码:";
	fixed_input(key2,20,&flag);
//	chTost(key1,key2);
	if(strcmp(key2,(p1->second->getke()).c_str())==0){//密码正确
	    cout<<cursor_right3<<"登录成功"<<endl;
//	    sleep(1);
	    char buf[20]={};
	    while(1){
		interface(26,86,23);
		cout<<cursor_right2<<"玩家个人信息界面\n"<<endl;
		cout<<cursor_right3<<"1.自己信息\n"<<endl;
		cout<<cursor_right3<<"2.基本状态\n"<<endl;
		cout<<cursor_right3<<"3.加成状态\n"<<endl;
		cout<<cursor_right3<<"4.我的背包\n"<<endl;
		cout<<cursor_right3<<"5.黑色集市\n"<<endl;
		cout<<cursor_right3<<"6.我的装备\n"<<endl;
		cout<<cursor_right3<<"7.退  出\n"<<endl;
		cout<<cursor_right3<<"请输入您的选择:";
		char choose=fixed_input1(buf,20,&flag);
		switch(choose){
		    case '1':
			    interface(26,86,23);
			    cout<<cursor_right2<<"显示玩家自己信息\n"<<endl;
			    p1->second->shownews();
			    sleep(5);break;
		    case '2':
			    interface(26,86,23);
			    cout<<cursor_right2<<"显示玩家基本状态\n"<<endl;
			    p1->second->showAttr1();
			    sleep(5);break;
		    case '3':
			    interface(26,86,23);
			    cout<<cursor_right2<<"显示玩家加成状态\n"<<endl;
			    p1->second->showAttr2();
			    sleep(5);break;
		    case '4':show_bag(p1);break;
		    case '5':shop(p1);break;
		    case '6':show_equip();break;
		    case '7':return;
		    default:break;
		    }
    
		}
	}
	else{
	    cout<<cursor_right1<<"密码错误"<<endl;
	    sleep(2);
	}
    }
    else{//没有玩家帐号
	cout<<cursor_right1<<"帐号错误"<<endl;
	sleep(2);
    }
}
void device_plot(){
    interface(26,86,23);
    display_cheek1(22,60,14);//行，列，上移
    draw_row_line1(19,16,29,"- ");//上移，右移，n列，字符串
    draw_row_line2(4,58,17,"- ");//下移，左移，n列，字符串
    draw_row_line2(4,34,17,"- ");//下移，左移，n列，字符串
    draw_row_line2(4,34,17,"- ");//下移，右移，n列，字符串
    draw_col_line1(10,34,10,11,"- ");//上移，左移，右移，n列，字符串
    draw_col_line2(10,12,11,"- ");//上移，右移，n列，字符串
    draw_col_line2(10,12,18,"- ");//上移，右移，n列，字符串
}   
void Game::show_equip(){
    device_plot();    
}
void Game::show_bag(map<string,Player *>::iterator p){
//    map<int,Actor *>::iterator b=bag.begin();
    interface(26,86,23);
    cout<<cursor_right2<<"我的背包\n"<<endl;
    cout<<cursor_right8<<"id号|耐久|名称|等级|部位";
    cout<<blue<<"|生命|魔法|攻速|移速|物伤|魔伤|物防|魔防"
	<<red<<"|数量"<<endl;;
    for(auto m:p->second->bag){
	m.second.first->show3();
	cout<<" |"<<m.second.second<<endl;
    }
    sleep(5);
}
/*
cout<<"glod = "<<r->second->getgo()<<endl;
cout<<"value = "<<r2->second.first->getva()<<endl;
*/
void Game::pay(char w,map<string,Player *>::iterator r,
	map<int,pair<Actor *,int> >::iterator r2){
    int set=r->second->getgo()-r2->second.first->getva();    
    switch(w){
	case '1':
	    if(set>=0){
		r->second->set_wallet1(set);//玩家钱包结算
		/*
		r->second->bag.insert({r2->second.first->getid(),
			r2->second.first});//添加到背包
			*/
		if(r->second->bag.find(r2->second.first->getn1())
			==r->second->bag.end())
		r->second->bag.insert({r2->second.first->getn1(),
			{r2->second.first,1}});//添加到背包
		else
		    r->second->bag[r2->second.first->getn1()].second++;
		cout<<cursor_right4<<"购买成功"<<endl;
		}
	    else
		cout<<cursor_right4<<"购买失败"<<endl;
	    sleep(1);
	    break;
	case '2': 
	    if(set>=0){
		r->second->set_wallet2(set);//玩家钱包结算
		/*
		r->second->bag.insert({r2->second.first->getid(),
			r2->second.first});//添加到背包
			*/
		/*
		r->second->bag.insert({r2->second.first->getn1(),
			{r2->second.first,1}});//添加到背包
			*/
		if(r->second->bag.find(r2->second.first->getn1())
			==r->second->bag.end())
		r->second->bag.insert({r2->second.first->getn1(),
			{r2->second.first,1}});//添加到背包
		else
		    r->second->bag[r2->second.first->getn1()].second++;
		cout<<cursor_right4<<"购买成功"<<endl;
		}
	    else
		cout<<cursor_right4<<"购买失败"<<endl;
	    sleep(1);
	    break;
    }
}
void Game::buy_goods(map<string,Player *>::iterator r){
    int flag;
    int id=0;
    char buf[20]={};
    while(1){
    interface(26,86,23);
    cout<<cursor_right2<<"地下城道具商城\n"<<endl;
    show_goods();
    cout<<cursor_right2<<"输入装备id可查看具体属性:";
    cin>>id;
    map<int,pair<Actor *,int> >::iterator p=goods.find(id);
    if(p!=goods.end()){//找到商品id
	interface(26,86,23);
	cout<<cursor_right2<<"商品具体信息\n"<<endl;
	p->second.first->show();
	cout<<cursor_right2<<"选择购买方式"<<endl;
	cout<<cursor_right3<<"1.金币"<<endl;
	cout<<cursor_right4<<"2.钻石"<<endl;
	cout<<cursor_right2<<"3.返回"<<endl;
	cout<<cursor_right2<<"4.退出"<<endl;
	cout<<cursor_right3<<"请输入您的选择:";
	char choose=fixed_input1(buf,20,&flag);
	if(choose=='1')
	    pay(choose,r,p);
	else if(choose=='2')
	    pay(choose,r,p);
	else if(choose=='3')
	    continue;
	else if(choose=='4')
	    return;
	else
	    cout<<cursor_right1<<"error"<<endl;
	sleep(2);
    }
    else{//没找到 
	cout<<cursor_right1<<"没有该商品"<<endl;
	sleep(2);
	}
    }
}
PARTS stTopart(string &s){
    if(s=="Hat")return Hat;
    else if(s=="Shirt")return Shirt;
    else if(s=="Pants")return Pants;
    else if(s=="Belt")return Belt;
    else if(s=="Shoes")return Shoes;
    else if(s=="Weapon")return Weapon;
    else if(s=="Drug")return Drug;
    else return Other;
}
RARE stTorare(string &s){
    if(s=="Common")return Common;
    else if(s=="Senior")return Senior;
    else if(s=="Smriti")return Smriti;
    else if(s=="Relic")return Relic;
    else if(s=="Artifact")return Artifact;
    else if(s=="Fame")return Fame;
    else if(s=="Epic")return Epic;
    else if(s=="Limit")return Limit;
    else return otheR;
}
//void Game::seek_good(map<int,pair<Actor *,int> >::iterator s,
//	PARTS p,int l,RARE r,int g){

//void Game::seek_good(PARTS p,int l,RARE r,int g){
void Game::seek_good(char p[],char l[],char r[],char g[]){
//    map<int,pair<Actor *,int> >::iterator s=goods.begin();
    string p1;
    string r1;
    int l1=0;
    int g1=0;
    chTost(p1,p);
    chTost(r1,r);

    PARTS t1=stTopart(p1);
    RARE t2=stTorare(r1);
    l1=chToin(l);
    g1=chToin(g);
    interface(26,86,23);
    cout<<cursor_right2<<"商品具体信息\n"<<endl;
    cout<<cursor_right8<<"id号|稀有|耐久|名称|等级|金币|钻石";
    cout<<blue<<"|部位|生命|魔法|攻速|移速|物伤|魔伤|物防|魔防"<<endl;

    for(auto m:goods){
	if(m.second.first->getpa()==t1&&m.second.first->getle()==l1){
	    m.second.first->show2();
	}
	/*
	else if((m.second.first->getle())==l1){
	    m.second.first->show2();
	}
	else if((m.second.first->getra())==t2){
	    m.second.first->show2();
	}
	else if((m.second.first->getva())==g1){
	    m.second.first->show2();
	}*/
//	m.second.first->show2();
    }
    sleep(5);
}

void Game::buy_good(){
    while(1){
    char buf[20]={};
    char in_buf[20]={};
    char in_buf1[20]={};
    char in_buf2[20]={};
    char in_buf3[20]={};
    char in_buf4[20]={};
    int flag;
    int fg=1;
    interface(26,86,23);
    cout<<cursor_right2<<"引索商品界面\n"<<endl;
    cout<<message_cursor7<<"1.部位:\n"<<endl;
    cout<<message_cursor8<<"3.稀有:\n"<<endl;
    cout<<message_cursor9<<"5.确定\n"<<endl;
//    set_cursor(47,5);
    fun_cursor_up(5);
    cout<<new_cursor7<<"2.等级:\n"<<endl;
    cout<<new_cursor8<<"4.价格:\n"<<endl;
    cout<<new_cursor9<<"6.返回\n"<<endl;
    cout<<message_cursor7<<"输入您的选择:";
    save_cursor();
    while(fg){
    char choose=fixed_input1(buf,20,&flag);
    switch(choose){
	case '1':choice_input(6,33,in_buf1);break;
	case '2':choice_input(6,54,in_buf2);break;
	case '3':choice_input(4,33,in_buf3);break;
	case '4':choice_input(4,54,in_buf4);break;
	case '5':seek_good(in_buf1,in_buf2,in_buf3,in_buf4);fg=0;break;
	case '6':return;
	default :break;
	}
	recover_cursor();
	clear_cursor(sizeof(buf));
	fun_cursor_left(sizeof(buf));
	}
    }
}

void Game::shop(map<string,Player *>::iterator r){
    char buf[20]={};
    int flag;
    while(1){
    interface(26,86,23);
    cout<<cursor_right2<<"欢迎光临地下集市\n"<<endl;
    cout<<cursor_right3<<"1.查看所有商品\n"<<endl;
    cout<<cursor_right4<<"2.引索商品\n"<<endl;
    cout<<cursor_right5<<"3.退出\n"<<endl;
    cout<<cursor_right6<<"请输入您的选择:";
    char choose=fixed_input1(buf,20,&flag);
    switch(choose){
	case '1':buy_goods(r);break;
	case '2':buy_good();break;
	case '3':return;
	default :break;
	}
    }
}


void Game::Begin(){
    while(1){
    interface(26,86,23);
    char begin[20];
    int flag;
    cout<<cursor_right2<<"欢迎来到 霹雳地下城\n"<<endl;
    cout<<cursor_right3<<"1.玩家登录\n"<<endl;
    cout<<cursor_right4<<"2.玩家注册\n"<<endl;
    cout<<cursor_right4<<"3.所有玩家\n"<<endl;
    cout<<cursor_right5<<"4.退出\n"<<endl;
    cout<<cursor_right2<<"请输入您的选择:";
//	char choose=menu_choose(20);
	char choose=fixed_input1(begin,20,&flag);
	switch(choose){
    case '1':Login();break;
	    case '2':Register();break;
	    case '3':show_allplayer();break;
	    case '4':return;
	    default:break;
//		     cout<<cursor_right1<<"error\n";sleep(1);break;
	}
    }
}

//游戏开始
int Game::run(){
#if 0
    time_t t;
    struct tm *showTime;
    produce_goods();//只生产1次商品
    while(1){
    interface(26,86,23);
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
    cout<<cursor_right4<<"请输入您的选择:";
    char choose=menu_choose(20);
    switch(choose){
	case '1':Begin();break;
	case '2':exit(-1);
	default:break;
//		 cout<<cursor_right1<<"error\n";sleep(1);break;
    }
    }
#endif
//    buy_good();
//    p1=atoi(p);
    /*
    for(int i=0;p[i]!='\0';i++){
	if(p[i]>='0'&&p[i]<='9'){
	    p1*=10;
	    p1+=p[i]-48;
	}
    }
    */
    /*
    char p[24]="123";
    int p1=chToin(p);
    cout<<p1<<endl;
    */
    int in=0;
    device_plot();    
    cin>>in;
//    while(1);
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

void Game::produce_goods(){
//    map<pair<int,Actor *>,int>goods;//id,商品,数量

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

#if 0
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
#endif
/************************************************************************装备-药剂*/
    Armor *p16=ap.produceArm(
	    30,35,Hand,Weapon,"绝世好剑","增加攻速物攻魔伤，传说",
	    0,0,0.7,0,300,100,0,0,3000,6500,55,Fame,Gold);
    Armor *p17=ap.produceArm(
	    30,35,Body,Drug,"死者苏醒","增加大量生命值，史诗",
	    500,0,0,0,0,0,0,0,0,7000,60,Epic,Gold);
    goods.insert({p1->getid(),{p1,1}});
    goods.insert({p2->getid(),{p2,1}});
    goods.insert({p3->getid(),{p3,1}});
    goods.insert({p4->getid(),{p4,1}});
    goods.insert({p5->getid(),{p5,1}});
    goods.insert({p6->getid(),{p6,1}});
    goods.insert({p7->getid(),{p7,1}});
    goods.insert({p8->getid(),{p8,1}});
    goods.insert({p9->getid(),{p9,1}});
    goods.insert({p10->getid(),{p10,1}});
#if 0
    goods.insert({{p11->getid(),p11},1});
    goods.insert({{p12->getid(),p12},1});
    goods.insert({{p13->getid(),p13},1});
    goods.insert({{p14->getid(),p14},1});
    goods.insert({{p15->getid(),p15},1});
#endif
    goods.insert({p16->getid(),{p16,1}});
    goods.insert({p17->getid(),{p17,1}});
    
}

void Game::show_goods(){
    cout<<message_cursor4<<"道具id\t"<<"道具名称\t"<<"等级\t"<<"重量\t"
	<<"金币\t"<<"钻石\t"<<"数量"<<endl;
    for(auto m:goods){
	/*
	cout<<message_cursor5<<m.first.first<<"\t\t"<<m.first.second->getn1()<<"\t"
	    <<m.first.second->getle()<<"\t"<<m.first.second->getwe()<<"\t"
	    <<m.first.second->getva()<<"\t"<<m.first.second->getdi()<<"\t"
	    <<m.second<<endl;
    */
	cout<<message_cursor5<<m.first<<"\t\t"<<m.second.first->getn1()<<"\t"
	    <<m.second.first->getle()<<"\t"<<m.second.first->getwe()<<"\t"
	    <<m.second.first->getva()<<"\t"<<m.second.first->getdi()<<"\t"
	    <<m.second.second<<endl;
    }
}

