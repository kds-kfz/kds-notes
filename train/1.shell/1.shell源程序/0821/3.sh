#!/bin/bash
clear
read -p "请输入帐号:" A
read -s -p "请输入6位密码：" -n 6 B
echo
read -p "请在5秒内输入4位验证码：" -t 10 -n 4 C
echo
read -p  "请输入11位手机号码:" -n 11 D
echo
echo "/***********************************************/"
echo "帐号:$A"
echo "密码:$B"
echo "验证码:$C"
echo "手机号:$D"

