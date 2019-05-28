#!/bin/bash
clear
echo "输入5个学生成绩，算平均分"
read -p "学生人数：" N
test $N -ne 0
if [ $? -eq 0 ]
then
    read -p "学生$N成绩:" A
    N=`expr $N - 1`
fi

test $N -ne 0
if [ $? -eq 0 ]
then
    read -p "学生$N成绩:" B
    let N=--N
fi

if test $N -ne 0
then
    read -p "学生$N成绩:" C
    N=$[ $N-1 ]
fi

if test $N -ne 0
then
    read -p "学生$N成绩:" D
    N=`expr $N - 1`
fi

if test $N -ne 0
then
    read -p "学生$N成绩:" E
    N=`expr $N - 1`
fi

let F=($A+$B+$C+$D+$E)/5

echo "5人平均分：$F"














