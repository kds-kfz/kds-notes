#!/bin/bash

# $0 当前脚本的文件名
# $n 传递给脚本或函数的参数
# $# 传递给脚本或函数的参数个数
# $* 传递给脚本或函数的所有参数
# $@ 传递给脚本或函数的所有参数
# $? 上个命令的退出状态，或函数的返回值
# $$ 当前Shell进程ID。对于 Shell 脚本，就是这些脚本所在的进程ID

# "$*" 会将所有的参数作为一个整体，以"$1 $2 … $n"的形式输出所有参数；"$@" 会将各个参数分开，以"$1" "$2" … "$n" 的形式输出所有参数
# 不被双引号(" ")包含时，都以"$1" "$2" … "$n" 的形式输出所有参数

clear
echo "File Name: $0"
echo "First Parameter : $1"
echo "First Parameter : $2"
echo "Quoted Values: $@"
echo "Quoted Values: $*"
echo "Total Number of Parameters : $#"

echo
echo "print each param from \"\$*\""
for var in "$*"
do
    echo "$var"
done

echo "print each param from \$*"
for var in $*
do
    echo "$var"
done

if [ $? -eq 0 ];then
    echo "**********project correct**********"
fi
