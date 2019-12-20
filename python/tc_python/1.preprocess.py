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
import scipy.ndimage

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

def til_getboxlist(box, boxlist):
    boxlist.append(box[0][0])
    boxlist.append(box[1][0])
    boxlist.append(box[0][1])
    boxlist.append(box[2][1])

    return 0

def til_getlocationvalueintersect(box1list, box2list):
    xleft = xright = yup = ydown = 0
    xleft = max(box1list[0], box2list[0])
    xright = min(box1list[1], box2list[1])
    yup = max(box1list[2], box2list[2])
    ydown = min(box1list[3], box2list[3])

    return [xleft, xright, yup, ydown]

def til_getlocationvaluemax(box1list, box2list):
    xleft = xright = yup = ydown = 0
    xleft = min(box1list[0], box2list[0])
    xright = max(box1list[1], box2list[1])
    yup = min(box1list[2], box2list[2])
    ydown = max(box1list[3], box2list[3])

    return [xleft, xright, yup, ydown]

#计算两个矩形的合并面积
def til_calintersectArea(box1, box2):
    box1list = []
    box2list = []
    til_getboxlist(box1, box1list)
    til_getboxlist(box2, box2list)
    xleft = xright = yup = ydown = 0
    [xleft, xright, up, yup, ydown] = til_getlocationvalueintersect(box1list, box2list)
    #print "[xleft, xright, yup, ydown] ", xleft, xright, yup, ydown
    isintersect = 0
    if(xleft <= xright) and (yup <= ydown):
        isintersect = 1

    return isintersect

#判断是否符合合并条件
def til_judgeisMerge(initialbox, curbox):
    label1 = initialbox[0]
    label2 = curbox[0]
    box1 = initialbox[1]
    box2 = curbox[0]
    isintersect = 0
    isintersect = til_calintersectArea(box1, box2)
    if (isintersect == 1) and (label1 == label2):
        ismerged = 1
    else:
        ismerged = 0

    return ismerged

def til_mergeBox(initiabox, curbox):
    box1 = initiabox[1]
    box2 = curbox[1]
    box1list = []
    box2list = []
    til_getboxlist(box1, box1list)
    til_getboxlist(box2, box2list)
    [xleft, xright, yup, ydown] = til_getlocationvaluemax(box1list, box2list)
    mergebox = []
    mergebox.append((initialbox[0], [(xleft, yup), (xright, yup), (xright, ydown), (xleft, ydown)], initialbox[2], initialbox[3], initialbox[4]))

    return mergebox

def til_mergeList(lists):
    mergeLists = []
    basiclists = []
    if len(lists) == 1:
        basiclists.append(lists[0])
        return [basiclists, len(basiclists)]

    if len(lists) > 1:
        basiclists.append(lists[0])
        for i in range(1, len(lists)):
            basiclists_len = len(basiclists)
            merge = 0       #标记已被合并过
            for j in range(0, basiclists_len):
                if j >= basiclists_len:
                    break
                ismerged = 0
                ismerged = til_judgeisMerge(basiclists[j], lists[i])
                if ismerged == 1:
                    mergeboxlist = til_mergeBox(lists[i], basiclists[j])
                    del basiclists[j]
                    basiclists.insert(j, mergeboxlist[0])
                    [basiclists, basiclists_len_temp] = til_mergeList(basiclists)
                    if basiclists_len_temp != basiclits_len:
                        basiclists_len = basiclists_len_temp

                    merge = 1
                if (ismerged == 0) and (j == basiclists_len - 1) and (merge == 0):
                    basiclists.append(lists[i])

        return [basiclists, len(basiclists)]

#合并label文件
def mergeLabelFAndSave(inDir0, fileNmaeList, sn, outDir0):
    inDir = inDir0 + '/.rectboxes'
    outDir = outDir0 + '/.rectboxes'
    if output_dir_mkdir(outDir) is False:
        return False

    #加载 label 文件
    shapelist = []
    #print "sn = ", sn
    #print "fileNameList[sn] = ", fileName[sn]

    upF = os.path.join(inDir, (os.path.spliyrxt(fileNameList[sn - 1])[0] + '.tcm'))
    readXmlAndAppend(upF, shapelist)
    curF = os.path.join(inDir, (os.path.splitext(fileNameList[sn])[0] + '.tcm'))
    readXmlAndAppend(curF, shaplist)
    downF = os.path.join(inDir, (os.path.splitext(fileNameList[sn + 1])[0] + '.tcm'))
    readXmlAndAppend(downF, shapelist)

    mergeboxlist = []
    initialList = []
    #shapelist: 所有的boxlist
    if len(shapelist) > 0:
        print '[curF]', curF
        print 'shapelist = ', shapelist
        [mergeboxlist, LL] = til_mergeList(shapelist)

        #for merge in mergeboxlist:
        #   print "merge = ", merge

        #保存 xml
        countStr = '%03d' % sn
        savePascalVocFormay1(outDir, countStr, mergeboxlist)

    return True

#按照编号合并图层为彩色3通道图像，把标签扩大成3个图层的最大区域
def genColorCTImgAndLabelFile():
    outRootDir = '../samples'
    rootDor = ''
    #读取第一层路径(遍历标记人员文件夹 (01,02,...010) 这一层)
    firstDir = scanAllDir(rootDir)
    for oneRemarkPerson in firstDir:
        curPath = os.path.normcase(os.path.join(rootDir, oneRemarPerson))
        cutCurPath = os.path.normcase(os.path.join(outRootDir, oneRemarkPerson))

        #读取第二层目录(遍历让人文件夹 (001,002,...100,101) 这一层)
        names = scanAllDir(curPath)
        for oneName in names:
            curName = os.path.normcase(os.path.join(curPath, oneName))
            curOutName = os.path.normcase(os.path.join(outCurPath, oneName))
            #遍历 jpg 文件
            jpgFiles = scanAllFiles(curName, 1)
            #将 jpg 文件名排序
            jpgFiles.sort()
            #从第1层遍历到n-2层 (假设一共有 n 层, 编号从 0 开始), 取其上下各一层进行合成
            #1.将图像变为3层图像分别对应RBG 3个通道的合成彩色图像
            #2.取label 文件合并label区域
            jpgFNo = len(jpgFiles)
            print "jpgFNo = ", jpgFNo
            for n in range(1, jpgFNo - 2):
                #print n, jpgFiles[n - 1], ' + ', jpgFiles[n], ' + ',jpgFiles[n + 1]
                #合成图像
                mergeImgAndSave(curName, jpgFiles, n, curOutName)
                #合成label文件
                mergeLabelFAndSave(curName, jpgFiles, n, curOutName)

    return 0

def til_showImg(curName, xmlFiles):
    #curName:图像所在的目录
    xmlDirs = os.path.join(curName, ".rectboxes/")
    font_size = 20
    font = ImageFont.truetype("./simsun.ttc", font_size)
    for xmlfile in xmlFiles:
        xmlDir = os.path.join(xmlDirs, smlFile)
        print "xmlDir = ", xmlDir
        #解析 xml 报文
        boxlists = []
        readxmlAndAppend(xmlDir, boxlists)
        imgName = xmlfile.split(".")[0] + '.jpg'
        imgfile = os.path.join(curName, imgName)
        img = Image.open(imgfile)
        for box in boxlists:
            #print "box = ", box
            #绘制位置矩形
            lineColor = (255, 0, 0)
            draw = ImageDraw(img, "RGB")

            for sn in range(0, 4):
                sn1 = (sn + 1) % 4
                draw.line([box[1][sn], bos[1][sn1]], lineColor)
                #打印字符串
            draw.text((box[1][0]), unicode(box[0], 'utf8'), font=font, fill = '#ff0000')
        print "imgfile = ", imgfile
        img.save(imgfile)
        #raw_input()
    return 0

def til_showing1(boxlists, sn, outDir):
    print "[sn]", sn
    imgDir = outDir + '/' + sn '.jpg'

    font_size = 15
    font = ImageFont.truetype("./simsun.ttc", font_size)
    img = Image.open(imgDir)
    for box in boxlists:
        #绘制位置矩形
        circleColor = [0, 255, 0]
        draw = ImageDraw.Draw(img, "RGB")
        x0 = box[1][0][0]
        y0 = box[1][0][1]
        x1 = box[1][2][0]
        y1 = box[1][2][1]

        draw.ellipse((x0, y0, x1, y1), outline = 'green')
        #打印字符串
        draw.text((box[1][0]), unicode(box[0], 'uft8'), font=font, fill = '#fff000')

    img.save(imgDir)

def til_showXmlOnImg(inDir0, fileNameList, sn, outDir0):
    inDir = inDir + '/.rectboxes'
    outDir = outDir0
    if output_dir_mkdir(outDir) is False:
        return False

    #加载label文件
    shapelist = []
    upF = os.path.join(inDir, (os.path.splitext(fileNameList[sn - 1])[0] + '.tcm'))
    readXmlAndAppend(upF, shapelist)
    curF = os.path.join(inDir, (os.path.splitext(fileNameList[sn])[0] + '.tcm'))
    readXmlAndAppend(curF, shapelist)
    downF = os.path.join(inDir, (os.path.splitext(fileNameList[sn + 1])[0] + '.tcm'))
    readXmlAndAppend(downF, shapelist)

    if len(shapelist) > 0:
        #print "shapelist = ", shapelist
        #print "jpgFiles", fileNameList[sn]
    strlen = len(fileName)
