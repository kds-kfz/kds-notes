#!/bin/bash
. year.sh
. student.sh
. quit.sh
. area.sh




tmp()
{
    while [ 1 ]
    do
    echo "/*****menu interface*****/"
    echo "1:student mark avrage"
    echo "2:count area"
    echo "3:count year"
    echo "4:exit"

    read -p "please choose a number:" NUM
    case $NUM in
    1) mark ;;
    2) area ;;
    3) count ;;
    4) out ;;
    *) echo "error!" ;;
    esac

    done
}


tmp
