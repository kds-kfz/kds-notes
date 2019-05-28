#!/bin/bash
##5.sh
display()
{
echo "1:register"
echo "2:login"
echo "3:quit"
echo "please choose a number form 1 to 3:"
echo "/************************************************/"
read -p "please input a number:" a
}

fun1()
{
echo "register"
read -p "name:" ID 
while [ 1 ]
do
    read -p "password:" password
    read -p "confirm  password:" password1
    if [ $password == $password1 ];then
	echo $ID > name.txt
	echo $password > password.txt
	echo "register success!"
	break
    else
	echo "twice password unlikeness"
    fi
done
sleep 2
}

fun2()
{
echo "login"
while [ 1 ]
do
    read -p "number:" A
    read -p "password:" B
    read C < name.txt
    read D < password.txt
    if [ $A == $C -a $B == $D ];then
	echo "login success!"
	break
    else 
	echo "login fail"
    fi 
done
sleep 2
}

fun3()
{
    read -p  "confirm  quit ? (y/n)" T
    if [ $T == y ];then
#    echo "quitting..."
#    sleep 2
#    break
    ji=5
    while [ $ji -gt 0 ]
    do
	echo -n "$ji "
	let ji--
	sleep 1
    done
    exit
    fi
}
main()
{
while [ 1 ]
do
display
case $a in
1)  fun1;;
2)  fun2;;
3)  fun3;;
*) echo "input error!";;
esac
done
}

main

#echo "输入一个字母，判断是否元音"
#read -p "please input a character:" str
#case $str in
#a|e|i|o|o) echo "vowel";;
#*) echo "unvowel";;
#esac
#
#if [ $? -eq 0 ];then
#    echo "/**********program correct**********/"
#fi

