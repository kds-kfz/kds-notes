#!/usr/bin/env python
# -- coding: utf-8 --

import sys
import os.path
from PIL import Image, ImageDraw, ImageFont
#from pascal_voc_io import PascalVocWriter, PascalVocReader
import os

#~ import cv2
#~ import dicom
#~ from loadDicomGdcm import get_pixels_one_dcm
#import scipy.ndimage

def scanAllFiles(folderPath, type):
    names = []
    extensionJPG = ['.jpg']
    extensionTCM = ['.tcm']
    extensionXML = ['.xml']
    extensionDCM = ['.dcm']
    
    for root, dirs, files in os.walk(folderPath):
        for file in files:
            if type == 1:
                if file.lower().endswith(tuple(extensionJPG)):
                    names.append(file)
            elif type == 2:
                if file.lower().endswith(tuple(extensionXML)):
                    names.append(file)
            elif type == 3:
                if file.lower().endswith(tuple(extensionTCM)):
                    names.append(file)
            elif type == 4:
                if file.lower().endswith(tuple(extensionDCM)):
                    names.append(file)
    return names

def scanAllDir(folderPath):
    names = []
    extensions = ['.txt', '.sh', '.tar']
    count = 0

    for dir in os.listdir(folderPath):
        if dir.lower().endswith(tuple(extensions)) is False:
            names.append(dir)
            count = count + 1;
            #~ print count, ' ', dir
    return names

def til_GetFolderList(basicDir, curLayerNo):
    dirList = []    # 所有文件夹，第一个字段是此目录的级别
    fileList = []   # 所有目录

    files = os.listdir(basicDir)    # 返回一个列表，其中包含在目录条目的名称

    for f in files:
        if(os.path.isdir(basicDir + '/' + f)):
            if(f[0] == '.'):        # 排除隐藏目录
                pass
            else:
                dirList.append(f)
        if(os.path.isfile(basicDir + '/' + f)):
            dirList.append(f)       # 添加文件

   #~ for i in range(len(dirList)):
   #~     print dirList[i]
   #~     # 打印目录下所有的文件夹，目录级别+1
   #~     til_GetDolderList((), path + '/' + dl)

def genTrainTestTTxt(type):
    trainValF = open("aa");
    testF = open("aa");
    testBF = open("aa");
    roorDir = "aa"
    arootDir = "aa"

    files = scanAllFiles(rootDir, 1)
    sn = 0
    notNeedNo = 0
    for onef in files:
        needWrite = True
        if type == 1:
            # 判断标签是否有 m_LCLC 和 m_BAC 标签
            if hasNotNeedLabels(arootDir, onef, 0):
                needWrite = False
            if needWrite:
                wStr = os.path.splitext(onef)[0] + '\n'
                if sn == 0:
                    # 黑盒测试样本，占 1%
                    testBF.write(wStr)
                elif sn % 10 == 1:
                    # 训练测试样本，占 10%
                    testF.write(wStr)
                else:
                    # 村里样本，占 89%
                    trainValF.write(wStr)
                sn = (sn + 1) % 100
    #关闭文件
    trainValF.close()
    testF.close()
    testBF.close()
    if type == 1:
        print LABEL_NUM_LIST

#合并样本标签为: 03 和 05 合并为03, 04 和 06 合并为04
def mergeLabelTo2Classies():
    aroorDir = ""
    files = scanAllFiles(arootDir, 3)
    for oneF in files:
        xmlPath = arootDir + oneF

        #读数
        xmlF = open(xmlPath, "r")
        all_the_text = xmlF.read()
        xmlF.close()

        #print 'Before', all_the_text
        
        t1 = all_the_text.replace("<name>05</name>", "<name>03</name>")
        t2 = t1.replace("<name>06</name>", "<name>04</name>")

        #print 'After', all_the_text

        xmloutF = open(xmlPath, "w")
        xmloutF.write(2)
        xmloutF.close()

def output_dir_mkdir(dir):
    if dir != None:
        isExist = os.path.exists(dir)
        if not isExist:
            os.makedirs(dir)
            #win32api.SetFileAttributes('dfile', win32con.FILE_ATTRIBUTE_HIDOEN)
        isExist = os.path.exists(dir)
        if isExist:
            return True
        else:
            return False
    else:
        return True

#合成图像
def mergeImageAndSave(inDir, fileNameList, sn, outDir):
    if output_dir_mkdir(outDir) is False:
        return False

    #加载图像
    upIm = Image.open(os.path.join(inDir, filrNameList[sn - 1]))
    r, upImN2, upImN3 = upIm.split()
    im = Image.open(os.path.join(inDir, fullNameList[sn]))
    g, imN2, downIm3 = downIm.split()

    if upIm.size[0] == 512 and upIm.szie[1] == 512 and im.size[0] == 512 and downIm.szie[0] == 512 and downIm.size[1] == 512:
        #合并图层
        merg_img = Image.merge('RGB', (r, g, b))
    
        countStr = '%03d' % sn
        output_file = outDir + '/' + countStr + '.jpg'
        merge_imge.save(output_file)

    return True


