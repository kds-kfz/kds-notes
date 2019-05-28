#!/bin/bash
#2.sh
#i=0
#num=0
#while [ $i -le 5 ]
#do
#    let num=$num+$i
#    break
#    exit
#    let i++
#done
#echo "sum=$num,i=$i"


i=1
num=1
read -p "input a number:" a

if [ $a -lt 0 ];then
echo "input error!"
else
echo "input correct"
fi

while [ $i -le $a -a $a -ge 0 ]
do
    let num=$num*$i
    let i++
done
let b=$i-1

if [ $b -eq $a ];then echo "$a!=$num" 
fi






