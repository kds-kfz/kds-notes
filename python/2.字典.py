#!/usr/bin/env python3
import os

clear = os.system('clear')

print('列表字典')
aliens = []

for alien_number in range(30):
    new_alien = {'color':'green', 'point':3}
    aliens.append(new_alien)

for alien in aliens[:4]:
    print(alien)
print('and so on')
print('total aliens: ' + str(len(aliens)))

print('\n字典列表')
pizza = {
    'crust': 'thick',
    'toppings': ['mushrooms', 'extra cheese']
}
for topping in pizza['toppings']:
    print(topping)

print('\n字典中存字典')
users = {
    'aeinstein':{
        'first': 'albert',
        'last': 'einstein',
        'location': 'princeton',
    },
    'mcurie':{
        'first': 'marie',
        'last': 'curie',
        'location': 'paris',
    },
}

for k,v in users.items():
    print('user_name: ' + k)
    print('full_name: ' + v['first'] + ' ' + v['last'])
    print('location: ' + v['location'].title() + '\n')

#python3 将raw-input 改为 input
print('输入测试:')
name = input('输入名字:')
age = input('输入年龄:')
print('name: ' + name + ', age: ' + age + '\n')
