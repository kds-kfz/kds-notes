#include<unistd.h>
#include<cstring>
//#include"player.h"
#include"game.h"
#include"shop.h"
#include"font.h"
using namespace std;

//ch字符串转整型函数，返回ch字符串里所有数字
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

//ch字符串转string字符串函数
void chTost(string &s1,char *s2){
    int i=0;
    s1.resize(0);
    for(;*s2!='\0';s2++,i++)
	s1+=*s2;
    s1[i]='\0';
}

//自定义标准输入函数
void choice_input(int up,int right,char arr[]){
    for(int i=0;i<sizeof(&arr);i++)
	arr[i]='\0';
    fun_cursor_up(up);
    fun_cursor_right(right);
    clear_cursor(sizeof(&arr));
    fun_cursor_left(sizeof(&arr));
    cin>>arr;
    menu_choose(20);//接收回车，传20个字节的缓存区接收回车
}
void common_input(int up,int down,int right,char arr[]){

}

//玩家注册函数
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

	Player *pl=new Player{name1,key1};//创建新玩家

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

//显示所有玩家函数
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

//玩家登录函数：包括玩家登录成功后操作
void Game::Login(){
    interface(26,86,23);
    int flag;
    char name2[20]={};
    char key2[20]={};
    string name1;
    string key1;
    char ch;
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
			while(1){
			interface(26,86,23);
			cout<<cursor_right2<<"显示玩家自己信息\n"<<endl;
			p1->second->shownews();
			cout<<cursor_right2<<"注:输入q退出"<<endl;
			cout<<cursor_right2<<"输入您的选择:";
			ch=fixed_input1(buf,20,&flag);
			if(ch=='q')
			    break;
			}
			break;
		    case '2':
			while(1){
			interface(26,86,23);
			cout<<cursor_right2<<"显示玩家基本状态\n"<<endl;
			p1->second->showAttr1();
			cout<<cursor_right2<<"注:输入q退出"<<endl;
			cout<<cursor_right2<<"输入您的选择:";
			ch=fixed_input1(buf,20,&flag);
			if(ch=='q')
			    break;
			}
			break;
		    case '3':
			while(1){
			interface(26,86,23);
			cout<<cursor_right2<<"显示玩家加成状态\n"<<endl;
			p1->second->showAttr2();
			cout<<cursor_right2<<"注:输入q退出"<<endl;
			cout<<cursor_right2<<"输入您的选择:";
			ch=fixed_input1(buf,20,&flag);
			if(ch=='q')
			    break;
			}
			break;
		    case '4':show_bag(p1);break;
		    case '5':shop(p1);break;
		    case '6':show_equip(p1);break;
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

//画装备栏表格函数
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
    fun_cursor_up(19);
    fun_cursor_left(16);
    cout<<blue<<"我 "<<green<<"的 "<<
	purple<<"装 "<<yellow<<"备 "<<dark_green<<"栏"<<"\033[0m";
    fun_cursor_down(3);
    fun_cursor_left(32);
    cout<<green<<"1.帽子:";
    fun_cursor_right(5);
    cout<<green<<"2.衬衣:";
    fun_cursor_right(5);
    cout<<green<<"3.裤子:";
    fun_cursor_down(4);
    fun_cursor_left(31);
    cout<<green<<"4.腰带:";
    fun_cursor_right(5);
    cout<<green<<"5.鞋子:";
    fun_cursor_right(5);
    cout<<green<<"6.武器:";
    fun_cursor_down(4);
    fun_cursor_left(31);
    cout<<green<<"7.红药:";
    fun_cursor_right(5);
    cout<<green<<"8.蓝药:";
    fun_cursor_right(5);
    cout<<green<<"9.经药:";
    fun_cursor_down(3);
    fun_cursor_left(20);

    cout<<purple<<"功 能 界 面";
    fun_cursor_down(1);
    fun_cursor_left(22);

    cout<<blue<<"a.格数:";//选择格数
    fun_cursor_down(1);
    fun_cursor_left(7);
    cout<<blue<<"b.装备:";//列出对应格数，装备id
    fun_cursor_down(1);
    fun_cursor_left(7);
    cout<<blue<<"c.选择:";//选择装备id
    fun_cursor_down(1);
    fun_cursor_left(7);
    cout<<blue<<"d.结果:";//退出
    fun_cursor_down(1);
    fun_cursor_left(7);
    //    cout<<blue<<"e.确定(y)退出(q):";//输入玩家选择
    //    fun_cursor_right(24);
    cout<<blue<<"e.继续(y)退出(q):";//输入玩家选择
    fun_cursor_right(24);
    fun_cursor_up(15);
    cout<<yellow<<"我的加成状态";
    fun_cursor_down(2);
    fun_cursor_left(17);
    cout<<blue<<"A.血量:";
    fun_cursor_down(1);
    fun_cursor_left(7);
    cout<<blue<<"B.蓝量:";
    fun_cursor_down(1);
    fun_cursor_left(7);
    cout<<blue<<"C.攻速:";
    fun_cursor_down(1);
    fun_cursor_left(7);
    cout<<blue<<"D.移速:";
    fun_cursor_down(1);
    fun_cursor_left(7);
    cout<<blue<<"E.物理:";
    fun_cursor_down(1);
    fun_cursor_left(7);
    cout<<blue<<"F.法术:";
    fun_cursor_down(1);
    fun_cursor_left(7);
    cout<<blue<<"G.物防:";
    fun_cursor_down(1);
    fun_cursor_left(7);
    cout<<blue<<"H.法防:";
    fun_cursor_down(1);
    fun_cursor_left(7);
    cout<<blue<<"I.负重:";
    fun_cursor_down(1);
    fun_cursor_left(7);
    cout<<blue<<"J.精神:";
    fun_cursor_down(1);
    fun_cursor_left(7);
    cout<<blue<<"K.智力:";
    fun_cursor_down(1);
    fun_cursor_left(7);
    cout<<blue<<"L.体力:";
}   

//装备栏，操作界面功能函数
//a数组存id，b数组存数量，c数组存选择模式，e存选择的id，num是b数组下标
void Game::function(map<string,Player *>::iterator p,
	int *a,int *b,char *c,int e,int num){
    int i=0;
    int k=0;
    int k2=0;
    for(;i<10;i++){
/*******************************************************************/		    
	//配备装备
	if(a[i]==e&&strcmp(c,"q")==0){
	    for(auto &m:p->second->bag)
		if(m.second.first->getid()==e){
		    if(b[num]>0){//如果装备数量不是0
			//判断人的部位和武器稀有是否已经配备
		    if(p->second->parts[num]!=m.second.first->getpa()||
			p->second->rares[num]!=m.second.first->getra()){
			if(m.second.first->getst()==1){
			    //保存部位+稀有
			    p->second->parts[num]=m.second.first->getpa();
			    p->second->rares[num]=m.second.first->getra();
				    
			    m.second.second--;
			    b[num]-=1;
			    p->second->addAttr(m.second.first->getat());
			    set_cursor(24,22);
			    cout<<"配备成功";
			    k=1;
			    k2=1;
			}
			else if(m.second.first->getst()==2){
			    set_cursor(24,22);
			    cout<<"请到时装栏配备";
			    k=1;
			    k2=1;
			    }
			else if(m.second.first->getst()==2){
			    set_cursor(24,22);
			    cout<<"请到首饰栏配备";
			    k=1;
			    k2=1;
			    }
			}
		    else{//判断人的部位和武器稀有已经配备
			k=1;
			k2=1;
			set_cursor(24,22);
			cout<<"装备已经配备";
			break;
			}
		    }
		else{//装备数量是0
		    k=1;
		    k2=1;
		    set_cursor(24,22);
		    cout<<"该装备数量为0";
		    }
		break;//背包里找到装备id，配备成功后跳出for循环
		}//遍历背包里的装备id
	    }//数组里找到了输入装备的id,同时是配备装备模式

/*******************************************************************/		    
	//卸载装备
	if(a[i]==e&&strcmp(c,"p")==0){
	    for(auto &m:p->second->bag)
		if(m.second.first->getid()==e){//判断正被id正确

//		    if(num[0]>0){//如果装备数量不是0
		    //判断人的部位和武器稀有是否已经配备，是
		if(p->second->parts[num]==m.second.first->getpa()||
			p->second->rares[num]==m.second.first->getra()){
		    if(m.second.first->getst()==1){//判断是否普通，是 
			//清空部位+稀有
			p->second->parts[num]=0;
			p->second->rares[num]=0;

			m.second.second++;//背包装备数量+1
			b[num]+=1;//装备数量+1
			//卸载属性
			p->second->reduceAttr(m.second.first->getat());
			set_cursor(24,22);
			cout<<"卸载成功";
			k=1;
			k2=1;
		    }
		    else if(m.second.first->getst()==2){
			set_cursor(24,22);
			cout<<"请到时装栏卸载";
			k=1;
			k2=1;
		    }
		    else if(m.second.first->getst()==2){
			set_cursor(24,22);
			cout<<"请到首饰栏卸载";
			k=1;
			k2=1;
		    }
		}
		else{//判断人的部位和武器稀有已经没有装备
		    k=1;
		    k2=1;
		    set_cursor(24,22);
		    cout<<"部位已经没有装备";
		    break;
		    }
		break;//背包里找到装备id，配备成功后跳出for循环
	    }
	}
/*******************************************************************/		    
	if(k2==0){
	    k=1;
	    set_cursor(24,22);
	    cout<<"选择错误";
	}
	if(k==0){
	    set_cursor(24,22);
	    cout<<"没有该装备";
	}
    }//for 10次
    return;
}

//显示装备栏所有数据+装备栏系列操作
void Game::show_equip_num(map<string,Player *>::iterator p){
    int num[10]={0};//存对应部位所有道具数量，帽子存num[0]...//0-6:7个
    int rare_num1[10]={0};//帽子，存道具稀有度道具数量，稀有读相同的存一个，实际有8种
    int rare_num2[10]={0};
    int rare_num3[10]={0};
    int rare_num4[10]={0};
    int rare_num5[10]={0};
    int rare_num6[10]={0};
    int rare_num7[10]={0};
    int id_num1[10]={0};//存道具id，区分：衣服，时装，首饰
    int id_num2[10]={0};
    int id_num3[10]={0};
    int id_num4[10]={0};
    int id_num5[10]={0};
    int id_num6[10]={0};
    int id_num7[10]={0};

    int count[10]={0};
    char buf[20]={};
    char buf1[20]={};
    int flag;
    for(auto m:p->second->bag){
	//	    if(m.second.first->getra()==Common)
	if(m.second.first->getpa()==Hat){
	    id_num1[count[0]++]=m.second.first->getid();
	    switch(m.second.first->getra()){
		case Common:
		    rare_num1[0]+=m.second.second;break;
		case Senior:
		    rare_num1[1]+=m.second.second;break;
		case Smriti:
		    rare_num1[2]+=m.second.second;break;
		case Relic:
		    rare_num1[3]+=m.second.second;break;
		case Artifact:
		    rare_num1[4]+=m.second.second;break;
		case Fame:
		    rare_num1[5]+=m.second.second;break;
		case Epic:
		    rare_num1[6]+=m.second.second;break;
		case Limit:
		    rare_num1[7]+=m.second.second;break;
		default :break;
	    }
	}
	else if(m.second.first->getpa()==Shirt){
	    id_num2[count[1]++]=m.second.first->getid();
	    switch(m.second.first->getra()){
		case Common:
		    rare_num2[0]+=m.second.second;break;
		case Senior:
		    rare_num2[1]+=m.second.second;break;
		case Smriti:
		    rare_num2[2]+=m.second.second;break;
		case Relic:
		    rare_num2[3]+=m.second.second;break;
		case Artifact:
		    rare_num2[4]+=m.second.second;break;
		case Fame:
		    rare_num2[5]+=m.second.second;break;
		case Epic:
		    rare_num2[6]+=m.second.second;break;
		case Limit:
		    rare_num2[7]+=m.second.second;break;
		default :break;
	    }
	}
	else if(m.second.first->getpa()==Pants){
	    id_num3[count[2]++]=m.second.first->getid();
	    switch(m.second.first->getra()){
		case Common:
		    rare_num3[0]+=m.second.second;break;
		case Senior:
		    rare_num3[1]+=m.second.second;break;
		case Smriti:
		    rare_num3[2]+=m.second.second;break;
		case Relic:
		    rare_num3[3]+=m.second.second;break;
		case Artifact:
		    rare_num3[4]+=m.second.second;break;
		case Fame:
		    rare_num3[5]+=m.second.second;break;
		case Epic:
		    rare_num3[6]+=m.second.second;break;
		case Limit:
		    rare_num3[7]+=m.second.second;break;
		default :break;
	    }
	}
	else if(m.second.first->getpa()==Belt){
	    id_num4[count[3]++]=m.second.first->getid();
	    switch(m.second.first->getra()){
		case Common:
		    rare_num4[0]+=m.second.second;break;
		case Senior:
		    rare_num4[1]+=m.second.second;break;
		case Smriti:
		    rare_num4[2]+=m.second.second;break;
		case Relic:
		    rare_num4[3]+=m.second.second;break;
		case Artifact:
		    rare_num4[4]+=m.second.second;break;
		case Fame:
		    rare_num4[5]+=m.second.second;break;
		case Epic:
		    rare_num4[6]+=m.second.second;break;
		case Limit:
		    rare_num4[7]+=m.second.second;break;
		default :break;
	    }
	}
	else if(m.second.first->getpa()==Shoes){
	    id_num5[count[4]++]=m.second.first->getid();
	    switch(m.second.first->getra()){
		case Common:
		    rare_num5[0]+=m.second.second;break;
		case Senior:
		    rare_num5[1]+=m.second.second;break;
		case Smriti:
		    rare_num5[2]+=m.second.second;break;
		case Relic:
		    rare_num5[3]+=m.second.second;break;
		case Artifact:
		    rare_num5[4]+=m.second.second;break;
		case Fame:
		    rare_num5[5]+=m.second.second;break;
		case Epic:
		    rare_num5[6]+=m.second.second;break;
		case Limit:
		    rare_num5[7]+=m.second.second;break;
		default :break;
	    }
	}
	else if(m.second.first->getpa()==Weapon){
	    id_num6[count[5]++]=m.second.first->getid();
	    switch(m.second.first->getra()){
		case Common:
		    rare_num6[0]+=m.second.second;break;
		case Senior:
		    rare_num6[1]+=m.second.second;break;
		case Smriti:
		    rare_num6[2]+=m.second.second;break;
		case Relic:
		    rare_num6[3]+=m.second.second;break;
		case Artifact:
		    rare_num6[4]+=m.second.second;break;
		case Fame:
		    rare_num6[5]+=m.second.second;break;
		case Epic:
		    rare_num6[6]+=m.second.second;break;
		case Limit:
		    rare_num6[7]+=m.second.second;break;
		default :break;
	    }
	}
	else if(m.second.first->getpa()==Drug){
	    id_num7[count[6]++]=m.second.first->getid();
	    switch(m.second.first->getra()){
		case Common:
		    rare_num7[0]+=m.second.second;break;
		case Senior:
		    rare_num7[1]+=m.second.second;break;
		case Smriti:
		    rare_num7[2]+=m.second.second;break;
		case Relic:
		    rare_num7[3]+=m.second.second;break;
		case Artifact:
		    rare_num7[4]+=m.second.second;break;
		case Fame:
		    rare_num7[5]+=m.second.second;break;
		case Epic:
		    rare_num7[6]+=m.second.second;break;
		case Limit:
		    rare_num7[7]+=m.second.second;break;
		default :break;
	    }
	}
    }
    for(int i=0;i<8;i++)
	num[0]+=rare_num1[i];
    for(int i=0;i<8;i++)
	num[1]+=rare_num2[i];
    for(int i=0;i<8;i++)
	num[2]+=rare_num3[i];
    for(int i=0;i<8;i++)
	num[3]+=rare_num4[i];
    for(int i=0;i<8;i++)
	num[4]+=rare_num5[i];
    for(int i=0;i<8;i++)
	num[5]+=rare_num6[i];
    for(int i=0;i<8;i++)
	num[6]+=rare_num7[i];

    set_point_num(24,7,num[0]);//帽子坐标
    set_point_num(36,7,num[1]);//衬衣坐标
    set_point_num(48,7,num[2]);//裤子坐标

    set_point_num(24,11,num[3]);//腰带坐标
    set_point_num(36,11,num[4]);//鞋子坐标
    set_point_num(48,11,num[5]);//武器坐标

    set_point_num(24,15,num[6]);//红药坐标
    set_point_num(36,15,0);//蓝药坐标
    set_point_num(48,15,0);//经验坐标

    p->second->showAttr3();
/*
    //待写，为完成
    set_cursor(61,17);
    cout<<blue<<"500 |500+0";
    set_cursor(61,18);
    cout<<blue<<"800 |800+0";
    set_cursor(61,19);
    cout<<blue<<"300 |300+0";
    set_cursor(61,20);
    cout<<blue<<"900 |900+0";
*/
    while(1){

	set_cursor(24,19);
	char ch=fixed_input1(buf,20,&flag);
	set_cursor(24,20);
	int fg2=0;
	char input_id[20]={};
    /**********************************************************************/
	//判断我的背包里装备是否是空
	if(ch=='1'){
	    if(id_num1[0]!=0){//不是空，列出对应部位装备所有装备id
		fg2=1;
		for(int i=0;i<10;i++){
		    if(id_num1[i]!=0)
			cout<<id_num1[i]<<" ";
		}
	    }
	    else{
		set_cursor(24,22);
		cout<<"无";
	    }
	}
	else if(ch=='2'){
	    if(id_num2[0]!=0){
		fg2=1;
		for(int i=0;i<10;i++){
		    if(id_num2[i]!=0)
			cout<<id_num2[i]<<" ";
		}
	    }
	    else{
		set_cursor(24,22);
		cout<<"无";
	    }
	}
	else if(ch=='3'){
	    if(id_num3[0]!=0){
		fg2=1;
		for(int i=0;i<10;i++){
		    if(id_num3[i]!=0)
			cout<<id_num3[i]<<" ";
		}
	    }
	    else{
		set_cursor(24,22);
		cout<<"无";
	    }
	}
	else if(ch=='4'){
	    if(id_num4[0]!=0){
		fg2=1;
		for(int i=0;i<10;i++){
		    if(id_num4[i]!=0)
			cout<<id_num4[i]<<" ";
		}
	    }
	    else{
		set_cursor(24,22);
		cout<<"无";
	    }
	}
	else if(ch=='5'){
	    if(id_num5[0]!=0){
		fg2=1;
		for(int i=0;i<10;i++){
		    if(id_num5[i]!=0)
			cout<<id_num5[i]<<" ";
		}
	    }
	    else{
		set_cursor(24,22);
		cout<<"无";
	    }
	}
	else if(ch=='6'){
	    if(id_num6[0]!=0){
		fg2=1;
		for(int i=0;i<10;i++){
		    if(id_num6[i]!=0)
			cout<<id_num6[i]<<" ";
		}
	    }
	    else{
		set_cursor(24,22);
		cout<<"无";
	    }
	}
	else if(ch=='7'){
	    if(id_num7[0]!=0){
		fg2=1;
		for(int i=0;i<10;i++){
		    if(id_num7[i]!=0)
			cout<<id_num7[i]<<" ";
		}
	    }
	    else{
		set_cursor(24,22);
		cout<<"无";
	    }
	}
    /**********************************************************************/
	else{//输入的不是装备栏号码：1-9 则显示没有该装备栏
	    set_cursor(24,22);
	    cout<<"没有该装备栏";
	}
	if(fg2==1){
	    set_cursor(24,21);
	    fixed_input(input_id,20,&flag);
	    int id=chToin(input_id);
	    char bf[20]={};
	    ChToch(bf,input_id);
	    if(ch=='1'){//选择第1个装备栏，帽子
	    //a数组存id，b数组存数量，c数组存选择模式，e存选择的id，num是b数组下标
		function(p,id_num1,num,bf,id,0);
	    #if 0
		int i=0;
		int k=0;
		int k2=0;
		for(;i<10;i++){
/*******************************************************************/		    
		    //配备帽子装备
		    if(id_num1[i]==id&&strcmp(bf,"q")==0){
		    for(auto &m:p->second->bag)
			if(m.second.first->getid()==id){
			if(num[0]>0){//如果装备数量不是0
			    //判断人的部位和武器稀有是否已经配备
			    if(p->second->parts[0]!=m.second.first->getpa()||
				p->second->rares[0]!=m.second.first->getra()){
				if(m.second.first->getst()==1){
				    //保存部位+稀有
				    p->second->parts[0]=m.second.first->getpa();
				    p->second->rares[0]=m.second.first->getra();
				    
				    m.second.second--;
				    num[0]-=1;
				    p->second->addAttr(m.second.first->getat());
				    set_cursor(24,22);
				    cout<<"配备成功";
				    k=1;
				    k2=1;
				    }
				else if(m.second.first->getst()==2){
				    set_cursor(24,22);
				    cout<<"请到时装栏配备";
				    k=1;
				    k2=1;
				    }
				else if(m.second.first->getst()==2){
				    set_cursor(24,22);
				    cout<<"请到首饰栏配备";
				    k=1;
				    k2=1;
				    }
				}
				else{//判断人的部位和武器稀有已经配备
				    k=1;
				    k2=1;
				    set_cursor(24,22);
				    cout<<"装备已经配备";
				    break;
				    }
				}
			    else{//装备数量是0
				k=1;
				k2=1;
				set_cursor(24,22);
				cout<<"该装备数量为0";
			    }
			    break;//背包里找到装备id，配备成功后跳出for循环
			}//遍历背包里的装备id
		    }//数组里找到了输入装备的id,同时是配备装备模式

/*******************************************************************/		    
		    //卸载帽子装备
		    if(id_num1[i]==id&&strcmp(bf,"p")==0){
		    for(auto &m:p->second->bag)
			if(m.second.first->getid()==id){//判断正被id正确

//			if(num[0]>0){//如果装备数量不是0
			    //判断人的部位和武器稀有是否已经配备，是
			    if(p->second->parts[0]==m.second.first->getpa()||
				p->second->rares[0]==m.second.first->getra()){
				if(m.second.first->getst()==1){//判断是否普通，是 
				    //清空部位+稀有
				    p->second->parts[0]=0;
				    p->second->rares[0]=0;

				    m.second.second++;//背包装备数量+1
				    num[0]+=1;//装备数量+1
				    //卸载属性
				    p->second->reduceAttr(m.second.first->getat());
				    set_cursor(24,22);
				    cout<<"卸载成功";
				    k=1;
				    k2=1;
				    }
				else if(m.second.first->getst()==2){
				    set_cursor(24,22);
				    cout<<"请到时装栏卸载";
				    k=1;
				    k2=1;
				    }
				else if(m.second.first->getst()==2){
				    set_cursor(24,22);
				    cout<<"请到首饰栏卸载";
				    k=1;
				    k2=1;
				    }
				}
				else{//判断人的部位和武器稀有已经没有装备
				    k=1;
				    k2=1;
				    set_cursor(24,22);
				    cout<<"部位已经没有装备";
				    break;
				    }
			    break;//背包里找到装备id，配备成功后跳出for循环
			    }
		    }
/*******************************************************************/		    
		    if(k2==0){
			k=1;
			set_cursor(24,22);
			cout<<"选择错误";
			}
		    if(k==0){
			set_cursor(24,22);
			cout<<"没有该装备";
			}
		}//for 10次
	    #endif
	    }//ch==1
	    else if(ch=='2')//选择第2个装备栏，衬衣
	    //a数组存id，b数组存数量，c数组存选择模式，e存选择的id，num是b数组下标
		function(p,id_num2,num,bf,id,1);
	    else if(ch=='3')//选择第3个装备栏，裤子
	    //a数组存id，b数组存数量，c数组存选择模式，e存选择的id，num是b数组下标
		function(p,id_num3,num,bf,id,2);
	    else if(ch=='4')//选择第4个装备栏，腰带
	    //a数组存id，b数组存数量，c数组存选择模式，e存选择的id，num是b数组下标
		function(p,id_num4,num,bf,id,3);
	    else if(ch=='5')//选择第5个装备栏，鞋子
	    //a数组存id，b数组存数量，c数组存选择模式，e存选择的id，num是b数组下标
		function(p,id_num5,num,bf,id,4);
	    else if(ch=='6')//选择第6个装备栏，武器
	    //a数组存id，b数组存数量，c数组存选择模式，e存选择的id，num是b数组下标
		function(p,id_num6,num,bf,id,5);
	    else if(ch=='7')//选择第7个装备栏，红药
	    //a数组存id，b数组存数量，c数组存选择模式，e存选择的id，num是b数组下标
		function(p,id_num7,num,bf,id,6);
	}
    /**********************************************************************/
	//更新装备栏所有数据
	game_clear_cursor(61,9,14,12);
	
	set_point_num(24,7,num[0]);//帽子坐标
	set_point_num(36,7,num[1]);//衬衣坐标
	set_point_num(48,7,num[2]);//裤子坐标

	set_point_num(24,11,num[3]);//腰带坐标
	set_point_num(36,11,num[4]);//鞋子坐标
	set_point_num(48,11,num[5]);//武器坐标

	set_point_num(24,15,num[6]);//红药坐标
	set_point_num(36,15,0);//蓝药坐标
	set_point_num(48,15,0);//经验坐标

	p->second->showAttr3();
    /**********************************************************************/
	//光标回到装备栏操作界面
	while(1){
	    set_cursor(34,23);
	    save_cursor();//保存光标位置
	    ch=fixed_input1(buf,20,&flag);
	    if(ch=='y'){
		game_clear_cursor(24,19,4,18);//坐标x，坐标y，行，列
		recover_cursor();
		//set_cursor(34,23);
		clear_cursor(sizeof(buf));
		break;
	    }
	    else if(ch=='q')
		return;
	    else{
		recover_cursor();//恢复光标位置
		clear_cursor(sizeof(buf)-1);//清除荧幕刚才输入的内容
	    }
	}
    }
}

//从字符串提取字符
void ChToch(char *a,char *b){
    for(int i=0,j=0;b[i]!='\0';i++,b++){
	if(b[i]<'0'||b[i]>'9'){
	    a[j++]=b[i];
	}
    }
}

//清屏函数，参数：x坐标，y坐标，row行，col列
void Game::game_clear_cursor(int x,int y,int row,int col){
    set_cursor(x,y);
    for(int i=0;i<row;i++){
	clear_cursor(col);
	fun_cursor_down(1);
	fun_cursor_left(col);
    }
}

//显示装备栏函数+显示装备栏所以数据
void Game::show_equip(map<string,Player *>::iterator p){
    device_plot();//画表
    show_equip_num(p);//显示装备数量
}

//显示自己背包函数
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

//显示所有商品后，购买商品函数
void Game::pay(char w,map<string,Player *>::iterator r,
	map<int,pair<Actor *,int> >::iterator r2,int n){
    int set=r->second->getgo()-r2->second.first->getva()*n;    
    int set1=r->second->getdi()-r2->second.first->getdi()*n;    
    switch(w){
	case '1':
	    if(set>=0){
		if(n>0&&r2->second.second>=n){//购买数量不是0，商品数量大于等于购买数
		r->second->set_wallet1(set);//玩家钱包结算
		/*
		   r->second->bag.insert({r2->second.first->getid(),
		   r2->second.first});//添加到背包
		 */
		if(r->second->bag.find(r2->second.first->getn1())
			==r->second->bag.end())
		    r->second->bag.insert({r2->second.first->getn1(),
			    {r2->second.first,n}});//添加到背包
		else
		    r->second->bag[r2->second.first->getn1()].second+=n;
		    
		    cout<<cursor_right4<<"购买成功"<<endl;
		    if(r2->second.second>0)//商品数量-1
			r2->second.second-=n;
		}
		else
		    cout<<cursor_right4<<"购买失败"<<endl;
	    }
	    else
		cout<<cursor_right4<<"购买失败"<<endl;
//	    sleep(1);
	    break;
	case '2': 
	    if(set1>=0){
		if(n>0&&r2->second.second>=n){//购买数量不是0,商品数量大于等于购买数
		r->second->set_wallet2(set1);//玩家钱包结算
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
			    {r2->second.first,n}});//添加到背包
		else
		    r->second->bag[r2->second.first->getn1()].second+=n;
		    
		    cout<<cursor_right4<<"购买成功"<<endl;
		    if(r2->second.second>0)//商品数量-1
			r2->second.second-=n;
		}
		else
		    cout<<cursor_right4<<"购买失败"<<endl;
	    }
	    else
		cout<<cursor_right4<<"购买失败"<<endl;
//	    sleep(1);
	    break;
    }
//    sleep(1);
}

//显示所有商品后，购买商品函数
void Game::buy_goods(map<string,Player *>::iterator r){
    int flag;
    int id=0;
    char buf[20]={};
    while(1){
	interface(26,86,23);
	cout<<cursor_right2<<"地下城道具商城\n"<<endl;
	show_goods();
	cout<<cursor_right2<<"注:q则退出\n";
	cout<<cursor_right2<<"输入装备id可查看具体属性:";
//	cin>>id;
	char choose=fixed_input1(buf,20,&flag);
	id=chToin(buf);
	if(choose=='q')
	    return;
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
	    if(choose=='1'){
		cout<<cursor_right3<<"请输入购买数量:";
		char ch=fixed_input1(buf,20,&flag);
		int count=chToin(buf);
		pay(choose,r,p,count);
	    }
	    else if(choose=='2'){
		cout<<cursor_right3<<"请输入购买数量:";
		char ch=fixed_input1(buf,20,&flag);
		int count=chToin(buf);
		pay(choose,r,p,count);
	    }
	    else if(choose=='3')
		continue;
	    else if(choose=='4')
		return;
	    else
		cout<<cursor_right1<<"error"<<endl;
	    sleep(1);
	}
	else{//没找到 
	    cout<<cursor_right1<<"没有该商品"<<endl;
	    sleep(1);
	}
    }
}

//输入字符串返回装备部位函数
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

//输入字符串返回装备稀有函数
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

//搜索商品函数
void Game::seek_good(char p[],char l[],char r[],char g[]){
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

//搜索商品后，购买商品函数
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

//商店显示界面
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

//进入游戏
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
	//char choose=menu_choose(20);
	char choose=fixed_input1(begin,20,&flag);
	switch(choose){
	    case '1':Login();break;
	    case '2':Register();break;
	    case '3':show_allplayer();break;
	    case '4':return;
	    default:break;
	}
    }
}

//游戏开始
int Game::run(){
#if 1
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
    for(auto m:goods)//释放所有商品
	delete m.second.first;

    map<string,Player *>::iterator p=player.begin();

    if(p!=player.end()){//释放每个玩家的背包的所有装备
	for(auto m:p->second->bag)
	    delete m.second.first;
	p++;
    }

    for(auto m:player)//释放所有玩家
	delete m.second;
#endif
    return 0;
}

//测试代码1
void Game::test(){
    HAT hat(5,10,"紫金头盔","增加护甲与攻速",
	    60,100,40,2000,4000,30,Senior,Gold,1);
    WEAPON weap(10,50,"狙击步枪","增加攻击与攻速",
	    0.7,300,100,3000,6500,55,Fame,Gold,1);
    DRUG drug(15,25,"死者苏醒","增加大量生命值",
	    500,7000,60,Artifact,Gold,1);
    hat.show();
    cout<<"-------------------------------------------------------"<<endl;
    weap.show();
    cout<<"-------------------------------------------------------"<<endl;
    drug.show();
}

//工厂生产商品
void Game::produce_goods(){
    //    map<pair<int,Actor *>,int>goods;//id,商品,数量
    Factory_All ap;
    Armor *p1=ap.produceArm(
	    10,15,Hat,"限定头盔","增加攻击物抗和魔抗，限定",
	    0,0,0,0,60,0,100,40,2000,4000,30,Limit,Gold,1);
    Armor *p2=ap.produceArm(
	    10,15,Shirt,"史诗夹克","增加生命攻速和物抗，史诗",
	    100,0,0.3,0,0,0,60,0,1800,4500,35,Epic,Gold,1);
    Armor *p3=ap.produceArm(
	    10,15,Pants,"传说长裤","增加生命物抗和魔抗，传说",
	    200,0,0,0,0,0,70,50,2300,5000,40,Fame,Gold,1);
    Armor *p4=ap.produceArm(
	    10,15,Belt,"圣物腰带","增加生命物抗和魔抗，圣物",
	    300,0,0,0,0,0,100,100,2400,5500,45,Relic,Gold,1);
    Armor *p5=ap.produceArm(
	    10,15,Shoes,"神器足具","增加移速物抗和魔抗，神器",
	    0,0,0,60,0,0,50,40,1900,6000,50,Artifact,Gold,1);

    //等级+时装+部位+名字+描述+
    //生命+蓝+攻速+移速+物伤+魔伤+物抗+魔抗+耐久+造价+稀有+价值
    
    //level+TYPE+PARTS+name+nature+
    //hp+mp+spd+spd2+atk+atm+def+res+ten+Value+RARE+WEALTH

    /********************************************************时装(state=2)+部位*/
    //帽子：等级+时装+部位+名字+描述+0+0+0+0+物伤+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p6=ap.produceArm(
	    20,25,Dress,Hat,"普通头盔","增加攻击物抗和魔抗，普通",
	    0,0,0,0,60,0,100,40,2000,4000,30,Common,Gold,2);
    //衬衫：等级+时装+部位+名字+描述+生命+0+攻速+0+0+0+物抗+0+耐久+造价+稀有+价值
    Armor *p7=ap.produceArm(
	    20,25,Dress,Shirt,"高级夹克","增加生命攻速和物抗，高级",
	    100,0,0.3,0,0,0,60,0,1800,4500,35,Senior,Gold,2);
    //裤子：等级+时装+部位+名字+描述+生命+0+0+0+0+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p8=ap.produceArm(
	    20,25,Dress,Pants,"传承长裤","增加生命物抗和魔抗，传承",
	    200,0,0,0,0,0,70,50,2300,5000,40,Smriti,Gold,2);
    //腰带：等级+时装+部位+名字+描述+生命+0+0+0+0+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p9=ap.produceArm(
	    20,25,Dress,Belt,"圣物腰带","增加生命物抗和魔抗，圣物",
	    300,0,0,0,0,0,100,100,2400,5500,45,Relic,Gold,2);
    //鞋子：等级+时装+部位+名字+描述+0+0+0+移速+0+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p10=ap.produceArm(
	    20,25,Dress,Shoes,"神器足具","增加移速物抗和魔抗，神器",
	    0,0,0,60,0,0,50,40,1900,6000,50,Artifact,Gold,2);

#if 0
    /********************************************************饰品(state=3)+部位*/
    //帽子：等级+时装+部位+名字+描述+0+0+0+0+物伤+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p11=ap.produceArm(
	    30,35,Orbament,Hat,"普通发带","增加攻击物抗和魔抗，普通",
	    0,0,0,0,60,0,100,40,2000,4000,30,Common,Gold,3);
    //衬衫：等级+时装+部位+名字+描述+生命+0+攻速+0+0+0+物抗+0+耐久+造价+稀有+价值
    Armor *p12=ap.produceArm(
	    30,35,Orbament,Shirt,"宝石横溢","增加生命攻速和物抗，高级",
	    100,0,0.3,0,0,0,60,0,1800,4500,35,Senior,Gold,3);
    //裤子：等级+时装+部位+名字+描述+生命+0+0+0+0+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p13=ap.produceArm(
	    30,35,Orbament,Pants,"西域牛仔","增加生命物抗和魔抗，传承",
	    200,0,0,0,0,0,70,50,2300,5000,40,Smriti,Gold,3);
    //腰带：等级+时装+部位+名字+描述+生命+0+0+0+0+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p14=ap.produceArm(
	    30,35,Orbament,Belt,"紫金腰带","增加生命物抗和魔抗，圣物",
	    300,0,0,0,0,0,100,100,2400,5500,45,Relic,Gold,3);
    //鞋子：等级+时装+部位+名字+描述+0+0+0+移速+0+0+物抗+魔抗+耐久+造价+稀有+价值
    Armor *p15=ap.produceArm(
	    30,35,Orbament,Shoes,"恶灵魔足","增加移速物抗和魔抗，神器",
	    0,0,0,60,0,0,50,40,1900,6000,50,Artifact,Gold,3);
#endif
    /********************************************************装备(state=1)-药剂*/
    Armor *p16=ap.produceArm(
	    30,35,Hand,Weapon,"绝世好剑","增加攻速物攻魔伤，传说",
	    0,0,0.7,0,300,100,0,0,3000,6500,55,Fame,Gold,1);
    Armor *p17=ap.produceArm(
	    30,35,Body,Drug,"死者苏醒","增加大量生命值，史诗",
	    500,0,0,0,0,0,0,0,0,7000,60,Epic,Gold,1);
    
    //默认工厂每件商品生产10个
    goods.insert({p1->getid(),{p1,10}});
    goods.insert({p2->getid(),{p2,10}});
    goods.insert({p3->getid(),{p3,10}});
    goods.insert({p4->getid(),{p4,10}});
    goods.insert({p5->getid(),{p5,10}});
    goods.insert({p6->getid(),{p6,10}});
    goods.insert({p7->getid(),{p7,10}});
    goods.insert({p8->getid(),{p8,10}});
    goods.insert({p9->getid(),{p9,10}});
    goods.insert({p10->getid(),{p10,10}});
    #if 0
    goods.insert({{p11->getid(),p11},10});
    goods.insert({{p12->getid(),p12},10});
    goods.insert({{p13->getid(),p13},10});
    goods.insert({{p14->getid(),p14},10});
    goods.insert({{p15->getid(),p15},10});
    #endif
    //武器和药剂生产1个
    goods.insert({p16->getid(),{p16,1}});
    goods.insert({p17->getid(),{p17,1}});

}

//显示所有商品
void Game::show_goods(){
    cout<<message_cursor4<<"道具id\t"<<"道具名称\t"<<"等级\t"<<"重量\t"
	<<"金币\t"<<"钻石\t"<<"数量"<<endl;
    for(auto m:goods){
	cout<<message_cursor5<<m.first<<"\t\t"<<m.second.first->getn1()<<"\t"
	    <<m.second.first->getle()<<"\t"<<m.second.first->getwe()<<"\t"
	    <<m.second.first->getva()<<"\t"<<m.second.first->getdi()<<"\t"
	    <<m.second.second<<endl;
    }
}

