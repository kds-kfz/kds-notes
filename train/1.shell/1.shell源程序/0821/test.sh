#!/bin/bash
clear
echo "题目一：去超市买水果，苹果6块1斤，西瓜2块1斤，算总价"
echo "请输入参数："
read -p "苹果斤数：" A
read -p "西瓜斤数：" B
C=`expr $A \* 6 + $B \* 2`
echo "总价：$C"

echo "/************************************************/"

echo "题目二：输入5个学生成绩，算平均满分"
read -p "学生1成绩：" A
read -p "学生2成绩：" B
read -p "学生3成绩：" C
read -p "学生4成绩：" D
read -p "学生5成绩：" E

#F=$(((A+B+C+D+E)/5))

#F=`expr ($A + $B + $C + $D + $E)`
#G=`expr $F / 5`

#F=$[ (A+B+C+D+E)/5 ]

let F=($A+$B+$C+$D+$E)/5

echo "5人平均分：$F"




