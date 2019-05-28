#!/bin/bash

fun()
{

    if [ $# -eq 2 ];then
    let sum=$a+$b
    else
	return 1
    fi
}

read a b

fun a 

if [ $? -eq 1 ];then
    echo "error"
else
    echo "$sum"
fi


