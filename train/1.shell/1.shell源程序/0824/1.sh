#!/bin/bash
##1.sh

#i=1
#while [ $i -le 10 ]
#do
#echo -n "$i "
#let i++
#done
#echo
#
#let i=i-1
#
#until [ $i -lt 1 ]
#do
#echo -n "$i "
#let i--
#done
#echo


#while [ 1 ]
#do
#
#read -p "please a number:" A
##read -p "number bit:" B
#
#
#let a=$A/10000
#let b=$A/1000%10
#let c=$A/100%10
#let d=$A/10%10
#let e=$A%10
#if [ $a -ne 0 ];then
#B=5
#until [ $B -lt 1 ]
#do
#    j=9
#    if [ $B -eq 5 ];then
#    let C=$e
#    elif [ $B -eq 4 ];then
#    let C=$d
#    elif [ $B -eq 3 ];then
#    let C=$c
#    elif [ $B -eq 2 ];then
#    let C=$b
#    elif [ $B -eq 1 ];then
#    let C=$a
#    fi 
#    until [ $j -lt $C ]
#    do
#    let j--
#    done
#    let j++
#    echo -n "$j"
#    let B--
#done
#echo
#
#elif [ $b -ne 0 ];then
#B=4
#until [ $B -lt 1 ]
#do
#    j=9
#    if [ $B -eq 4 ];then
#    let C=$e
#    elif [ $B -eq 3 ];then
#    let C=$d
#    elif [ $B -eq 2 ];then
#    let C=$c
#    elif [ $B -eq 1 ];then
#    let C=$b
#    fi 
#    until [ $j -lt $C ]
#    do
#    let j--
#    done
#    let j++
#    echo -n "$j"
#    let B--
#done
#echo
#elif [ $c -ne 0 ];then
#B=3
#until [ $B -lt 1 ]
#do
#    j=9
#    if [ $B -eq 3 ];then
#    let C=$e
#    elif [ $B -eq 2 ];then
#   let C=$d
#    elif [ $B -eq 1 ];then
#    let C=$c
#    fi 
#    until [ $j -lt $C ]
#    do
#    let j--
#    done
#    let j++
#    echo -n "$j"
#    let B--
#done
#echo
#
#elif [ $d -ne 0 ];then
#B=2
#until [ $B -lt 1 ]
#do
#    j=9
#    if [ $B -eq 2 ];then
#    let C=$e
#    elif [ $B -eq 1 ];then
#    let C=$d
#    fi 
#    until [ $j -lt $C ]
#    do
#    let j--
#    done
#    let j++
#    echo -n "$j"
#    let B--
#done
#echo
#
#fi
#
#done



##输入数字，输出反转。12345 -> 54321
while [ 1 ]
do
read -p "please a number:" A
until [ $A -eq 0 ]
do
    let B=$A%10
    if [ $B -ne 0 ];then
    echo -n "$B"
    fi
    let A=$A/10
done
echo
done


if [ $? -eq 0 ];then
echo "/********this program correct********/"
else
echo "/********this program error !********/"
fi
