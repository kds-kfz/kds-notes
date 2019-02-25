#!/bin/bash

clear
# for 
# while
# continue 
# break 

# ***
# **
# *
read -p "please input row: " N
n=$N
i=0
while [ $i -lt $N ]
do
    j=0
    while [ $j -lt $N ]
    do
    echo -n "*"
    let j++
    done
    echo
    let N--
done

echo
i=1
while [ $i -le $n ]
do
    j=$n
    while [ $j -ge $i ]
    do
    echo -n "*"
    let j--
    done
    echo
    let i++
done

echo
i=1
while [ $i -le $n ]
do
    j=1
    let B=$n-$i+1
    while [ $j -le $B ]
    do
        echo -n "*"
        let j++
    done
    echo
    let i++
done

# *
# **
# ***
echo
i=0
while [ $i -lt $n ]
do
    j=0
    while [ $j -le $i ]
    do
        echo -n "*"
        let j++
    done
    echo
    let i++
done

echo



