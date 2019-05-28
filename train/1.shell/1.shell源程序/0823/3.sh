#!/bin/bash
#3.sh
echo "1-20,个位是5十位是1到不能相加"
read -p "please input number:" A
#echo "/**********method1**********/"
#i=1
#num=0
#while [ $i -le $A ]
#do
#    let a=$i%10
#    let b=$i/10
#    if [ $a -ne 5 -a $b -ne 1 ];then
#	let num=$num+$i
#    fi
#    let i++
#done

#echo "/**********method2**********/"

i=0
num=0
while [ $i -lt $A ]
do
    let i++
    let a=$i%10
    let b=$i/10
    if [ $a -eq 5 -o $b -eq 1 ];then
    continue
    fi
    let num=$num+$i

    
    
done
echo "num=$num,i=$i"




