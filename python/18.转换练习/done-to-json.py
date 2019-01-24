#!/usr/bin/env python
# -- coding: utf-8 --

import os
import json

input_file = 'kd-fs-done.businessid'
output_file = 'business-map.json'

'''
example:
{
    "ptjy/ptyw":{
        "login":"ptjy_ptyw_login",
        "cccx":"ptjy_ptyw_cccx"
    }
}

## 说明:
## 这是一个读取kd-fs-done.businessid文件，生成business-map.json配置文件的脚本
## 用于引擎v2.0加载动态库 
'''

output = {} # 外层字典
inside = {} # 内层字典
count_1 = 0 

getpath = os.getcwd()
#print(getpath)

try:
#    input_file_path = getpath + "/../submodule/business/business_program.d/" + input_file
    input_file_path = getpath + "/" + input_file
    with open(input_file_path) as file_object:
        lines = file_object.readlines()
    for line in lines:
        # 获取key
        a = line.strip()
        b = a.rfind("/")
        H = a[:b]
        T = a[b+1:]
        H_1 = H[:H.index('/')]
        H_2 = H[H.index('/')+1:]
        count_1 += 1
        # 初始化
        inside[count_1] = {"":""}
        inside[count_1][T]=a.replace('/','_')
        if H not in output.keys():
            output[H] = {"":""}
    file_object.close()

    for k1,v1 in output.items():
        for count in range(1,count_1 + 1):
            for k2,v2 in inside[count].items():
                tmp = v2[:v2.rfind("_")].replace('_','/')
                if k1 == tmp:
                    tp = v2[v2.rfind("_")+1:]
                    output[k1][tp] = v2
    # 删除空键值
    for k in output.keys():
        del output[k][""]

    # 输出到终端
#    jsObj = json.dumps(output)
#    print(jsObj)

    total = 0
    for v in output.values():
        total+=len(v)
    print("统计接口总数: " + str(total) + " 个")
    print("统计业务模块总数: " + str(len(output)) + " 个")

except FileNotFoundError:
    print("Sorry,the file " + input_file_path + " don't exist !")

try:
#    output_file_path = getpath + "/../etc/" + output_file
#    output_file_path = getpath + "/../../../etc/" + output_file
    output_file_path = getpath + "/" + output_file
    with open(output_file_path,"w") as file_object:
        if len(output) != 0:
            # json格式化输出
            json.dump(output,file_object,indent=4)
            print("配置文件已生成")
        else:
            print("lead " + input_file_path + " fail !")
    
        file_object.close()

except FileNotFoundError:
    print("Sorry,the file " + input_file_path + " don't exist !")


