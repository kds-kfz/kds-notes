#!/usr/bin/env python3
import os,re

#with os.popen('who', 'r') as f:
#    for eachLine in f:
#	print re.split(r'\s\s+|\t', eachLine.strip())
clear = os.system('clear')
s = 'This and that'
print(re.findall(r'(th\w+) and (th\w+)', s, re.I))

print(re.finditer(r'(th\w+) and (th\w+)', s, re.I).__next__().groups())
