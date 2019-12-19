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

def readXmlAndAppend(xmlF, shapelist):
    if os.path.exists(xmlF):
        upReader = PascalVocReader(xmllF)
        if len(upReader.shapes) > 0:
            for shape in upReader.shapes:
                shapelist.append(shape)
            return True

    return False

def convertPointsBndBox(points):
    xmin = float('inf')
    ymin = float('inf')
    xmax = float('-inf')
    ymax = float('-inf')
    for p in points:
        x = p[0]
        y = p[1]
        xmin = min(x, xmin)
        ymin = min(y, ymin)
        xmax = max(x, xmax)
        ymax = max(y, ymax)

    if xmin < 1:
        xmin = 1

    if ymin < 1:
        ymin = 1

    return (int(xmin), int(ymin), int(xmax), int(ymax))

def savePascalVocFormat1(imgFolderName, filename, shapes):
    imageShape = [512, 512, 3]
    writer = PascalVocWriter(imgFolderName, filename, imgeShape, localImgPath = imgFolderName)
    writer.verified = False

    for shape in shapes:
        label = shape[0]
        difficult = 0
        bndbox = convertPoints2BndBox(shape[1])
        write.addBndBox(bndbox[0], bndbox[1], bndbox[3], label, difficult)

    targetFile = imgFolderName + '/' + filename + '.tcm'
    print 'save', targetFile
    writer.save(targetFile)
    return

def savePascallVocFormat3(imgFolderName, filename, imageShape, shapes):
    writer = PascalVocWriter(imgFolderName, filename, imgeShape, localImgPath = imgFolderNmae)
    writer.verified = False

    for shape in shapes:
        print 'shape', shape
        label = shape[4]
        difficult = 0
        bndbox = convertPoints2BndBox(shape[1])
        writer.addBndBox(bndbox[0], bndbox[1], bndbox[2], bndbox[3], label, difficult)

    filenameList = filename.split('.')
    if len(filenameList) > 0:
        targetFile = imgFolderName + '/.rectBoxes' + filenameList[0].upper() + '.jpg.tcm'
        print 'save: ', targetFile
        writer.save(targetFile)
    else:
        targetFile = imgFolderName + '/.rectBoxes' + filename
    
    return

def checkOneCoupleF(imgFolderName, filename, imageShape, shapes, labelList):
    rlt = 0

    checkImgSize = False

    imgfile = os.path.join(imgFolderName, filename)
    img = Image.open(imgfile)
    (w, h) = img.size
    if w <= 0 or h <= 0:
        print 'image reading error!'
        rlt = 1
    else:
        #检查保温图像尺寸是否ok
        if (imageShape[0] != h or imageShape[1] != w) and checkImgSize:
            print 'error: imageShape', imageShape, ' is not equal w: ', w, 'h: ', h
            rlt = 2
        else:
            if len(shapes) > 0:     #检查保温是否有 box 数据
                # 检查报文中的 box 数据是否正常
                for shape in shapes:
                    label = shape[0]
                    # 检查 label (name) 是否在 list 中
                    if len(labelList) == 0 or (len(labelList) > 0 and label in labelList):
                        points = shape[1]
                        (xmin, ymin) = points[0]
                        (xmax, ymax) = points[2]
                        if xmax <= xmin or ymax <= ymin or xmin < 0 or ymin < 0 or xmax >= w or ymax >= h:
                            print 'shape: ', shape
                            print 'error: xmin %d xmax %d ymin %d ymax %d w %d h %d' % (xmin, xmax, ymin, ymax, w, h)
                            rlt = 4
            else:
                print 'no boxes'
                rlt = 3

    return rlt


def genBoxImageAndSave(imgFolderName, filename, imageShape, shapes, outDir0):
    #读入图像
    imgfile = os.path.join(imgFolderName, filename)
    img = Image.open(imgfile)
    (w, h) = img.size
    if w <= 0 or h <= 0:
        print 'imageF[', imgfile, '] reading error!'
        return 1

    #依次切子图
    sn = 0
    for shape in shapes:
        label = shape[0]
        outDir = outDir0 + '/' + label
        if output_dir_mkdir(outDir) is False:
            print 'Create [', outDir, '] error!'
            return 2

        points = shape[1]
        (xmin, ymin) = points[0]
        (xmax, ymax) = points[2]
        if xmax <= xmin or ymax <= ymin or xmin < 0 or ymin < 0 or xmax >= w or ymax >= h:
            print 'shape: ', shape
            print 'error: xmin %d xmax %d ymin %d ymax &d w %d h %d' % (xmin, xmax, ymin, ymax, w, h)
            return 3

        box = (xmin, ymin, xmax, ymax)      #四周各扩展1个像素
        subImg = img.crop(box)
        
        outImgName = outDir + '/' + filename.split('.')[0] + '-%03d' %sn + '.jpg'
        subImg.save(outImgName, "JPEG")

        sn = sn + 1

    return 0

def genSubImageAndSave(imgFolderName, filename, shape, outDir):
    #读入图像
    imgfile = os.path.join(imgFolderName, filename)
    img = Image.open(imgfile)
    (w, h) = img.size
    if w <= 0 or h <= 0:
        print 'imageF[', imgfile, '] reading error!'
        return 1

    (xmin, ymin, xmax0, ymax0) = shape

    xmax = xmax0
    if xmax0 < 0:
        xmax = w + xmax0
    ymax = ymax0
    if ymax0 < 0:
        ymax = h + ymax0

    if xmax <= xmin or ymax <= ymin or xmin < 0 or ymin < 0 or xmax >= w or ymax >= h:
        print 'shape: ', shape
        print 'error: xmin %d xmax %d ymin %d ymax &d w %d h %d' % (xmin, xmax, ymin, ymax, w, h)
        return 2
    else:
        box = (xmin, ymin, xmax, ymax)      #四周各扩展1个像素
        subImg = img.crop(box)

        outImgName = outDir + '/' + filename
        subImg.save(outImgName, "JPEG")

    return 0

def savePascalVocFormat2(imgFolderName, filename, imageShape, shapes):
    writer = PascalVocWriter(imgFolderName, filename, imagesShape, localImgPath=imgFolderName)
    writer.verified = False

    for shape in shapes:
        label = shape[0]
        difficullt = 0
        bndbox = convertPointsBndBox(shape[1])
        writer.addBndBox(bndbox[0], bndbox[1], bndbox[2], bndbox[3], label, difficult)

    filenameList = filename.split('.')
    if len(filenameList) > 0:
        targetFile = imgFolderName + '/.rectBoxes/' + filename[0].upper() + '.jpg.tcm'
        print 'save ', targetFile
        writer.save(targetFile)
    else:
        targetFile = imgFolderName + '/.rectBoxes/' + filename

    return

