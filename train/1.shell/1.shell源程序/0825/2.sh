#!/bin/bash
##2.sh

fun()
{
    a=30
    echo "111"
}

#echo "$a"

for i in `seq 0 4`
do
    fun
done

echo "$a"


















