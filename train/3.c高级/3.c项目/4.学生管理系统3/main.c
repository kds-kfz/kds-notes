#include"manage.h"
#include"student.h"
#include"teacher.h"
//main.c
/***************************************
*项目:学生管理系统(多功能版)
*作者:开发者
*地点:教室305
*开始时间:2017年09月26日
*修改时间:2017年10月3日
*进度:
2017年09月27日项目要求全部完成
2017年09月28日做系统边框，字体
2017年09月29日
1，增加学生班级，职务，类别;增加教师职务
2，教师可对学生录成绩，班级，职务，类别
3，管理员可修改教师职务
2017年09月30日答辩
2017年10月01日
1，增加各门科目老师任课班级
2，只限班主任（与学生同班）可对学生录班级，职务，类别
3，各任课老师（与学生同班）可对学生录对应科目成绩
4，增加管理员密码简单加密（异或处理）
2017年10月02日
1，增加管理员所有新生一键排班
2，增加转入学生排班
3，完善边框功能
4，多功能学生系统完成
2017年10月03日
1，解决选择界面键盘输入问题
（可任意输入字符，字符串，空格，回车，界面一切正常）
系统版本：文件版v10.3
*存在问题:不是远程登录
***************************************/
int main(){//主函数
    my_system();//学生系统函数
    return 0;
}
