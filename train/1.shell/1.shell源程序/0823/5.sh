#!/bin/bash
#5.sh
#输出1-100整数，每到整10到时候换行

##test1:method1-------------------------
#i=0
#j=0
#sum=0
#echo "output 1-100"
#while [ $i -lt 10 ]
#do
#    j=0
#    while [ $j -lt 10 ]
#    do
#	let j++
#	let num++
#	echo -n "$num "
#    done
#    echo
#    let i++
#done

##test1:method2--------------------------
#i=1
#while [ $i -le 100 ]
#do
#    echo -n "$i "
#    let a=$i%10
#    if [ $a -eq 0 ];then
#    echo
#    fi
#    let i++
#done

#输出1-100所有奇数的和

#echo "output 1-100 sum:"
##test2:method1--------------------
#i=1
#while [ $i -le 99 ]
#do
#    let A=$i%2
#    if [ $A -ne 0 ];then
#	let sum=$sum+i
#    fi
#    let i++
#done
#echo "intal is :$sum,i=$i"

#test2:method2-------------------
#i=1
#sum=0
#while [ $i -le 100 ]
#do 
#    let sum=$sum+$i
#    let i=i+2
#done
#echo "intal is :$sum"

#输出
#*****
#****
#***
#**
#*

##test3:method1-----------------
#j=0
#read -p "please input row :" B
#while [ 0 -lt $B ]
#do  
#    j=0
#    while [ $j -lt $B ]
#    do
#    echo -n "*"
#    let j++
#    done
#    echo
#    let B--
#done 
##test3:method2-------------------
#i=1
#read -p "please input row:" A
#while [ $i -le $A ]
#do
#    j=$A
#    while [ $j -ge $i ]
#    do
#	echo -n "*"
#	let j--
#    done
#    echo
#    let i++
#done
##test3:method3--------------------
#i=1
#read -p "please input row:" A
#while [ $i -le $A ]
#do
#    j=1
#    let B=$A-$i+1
#    while [ $j -le $B ]
#    do
#	echo -n "*"
#	let j++
#    done
#    echo
#    let i++
#done

##输出
#*
#**
#***
#****
#*****

##test4:method1------------------------
#i=0
#j=0
#read -p "plase input row:" A
#while [ $i -lt $A ]
#do  
#    j=0
#    while [ $j -le $i ]
#    do
#    echo -n "*"
#    let j++
#    done
#    echo
#    let i++
#done 

##test4:method2----------------------
i=1
read -p "please input row:" A
while [ $i -le $A ]
do
    j=1
    while [ $j -le $i ]
    do
	echo -n "*"
	let j++
    done
    echo
    let i++
done













