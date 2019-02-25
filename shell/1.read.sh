#!/bin/bash

clear
read -p "请输入账号：" A
read -s -p "请输入6位密码：" -n 6 B
echo
read -p "请在5秒内输入4位验证码：" -t 5 -n 4 C
echo
read -p "请输入11位手机号：" -n 11 D
echo
echo "****************************************"
echo "账号：$A"
echo "密码：$B"
echo "验证码：$C"
echo "手机号：$D"
