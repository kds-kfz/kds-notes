.PHNOY : clean

INCS := ../common/include
SRCS := ../common/src
LIBS := ../lib

CC = gcc
CXX = g++

#-O0: 无优化(默认)
#-O和-O1: 使用能减少目标文 件大小以及执行时间并且不会使编译时间明显增加的优化.在编译大型程序的时候会显著增加编译时内存的使用.
#-O2: 包含-O1的优化并增加了不需要在目标文件大小和执行速度上进行折衷的优化.编译器不执行循环展开以及函数内联.此选项将增加编译时间和目标文件的执行性能.
#-Os: 专门优化目标文件大小,执行所有的不增加目标文件大小的-O2优化选项.并且执行专门减小目标文件大小的优化选项.
#-O3: 打开所有-O2的优化选项并且增加-finline-functions, -funswitch-loops,-fpredictive-commoning,-fgcse-after-reload and -ftree-vectorize优化选项

#使用线程调试编译
ifeq ($(DEBUG),1)
DEBUG_OPTS=-g3 -pg -D__DEBUG__
else
DEBUG_OPTS=-O2
endif

# -Wall 选项意思是编译后显示所有警告
#  # -w 关闭编译时的警告

CXXFLAGS = -Wall $(DEBUG_OPTS) -g -std=c++11
SHIELD = unknown-pragmas #屏蔽指定警告pragmas, 这是 win32 上编译的

#FLAG=-lm #链接数学库

COMMONSRCS=$(wildcard ../common/src/cjson/*.c)
COMMONOBJS=$(patsubst %.c,%.o,$(COMMONSRCS))

SRC=$(wildcard ./*.cpp) \
	$(wildcard $(SRCS)/tinyxml/*.cpp)
OBJ=$(patsubst %.cpp,%.o,$(SRC))


all : 1.a.out 2.a.out ser.out cli.out main.out 3.a.out
1.a.out : 1.生成ocr应答数据.o $(SRCS)/cjson/cJSON.o
	$(CXX) -o $@ $^
2.a.out : 6.解析ocr应答数据.o $(SRCS)/cjson/cJSON.o
	$(CXX) -o $@ $^
3.a.out : testjson.o kfile.o klog.o kipc.o kpubfun.o
	$(CXX) -o $@ $^ $(LIBS)/libjsoncpp.a -I $(INCS)
ser.out : serve.o klog.o kfile.o ksocket.o kipc.o ksignal.o kpubfun.o
	$(CXX) -o $@ $^
cli.out : client.o klog.o kfile.o ksocket.o kipc.o kpubfun.o
	$(CXX) -o $@ $^
main.out : main.o kcfg.o kfile.o klog.o $(SRCS)/cjson/cJSON.o kipc.o ksignal.o kpubfun.o \
$(SRCS)/tinyxml/tinystr.o $(SRCS)/tinyxml/tinyxml.o $(SRCS)/tinyxml/tinyxmlerror.o $(SRCS)/tinyxml/tinyxmlparser.o
	$(CXX) -o $@ $^ $(LIBS)/libjsoncpp.a $(addprefix -I, $(INC))

%.o : %.c
	$(CC) -o $@ -c $< -I $(INCS)
%.o : %.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS) $(addprefix -Wno-,$(SHIELD)) -I $(INCS)

clean:
	rm $(OBJ) $(COMMONOBJS) $(OBJ) *.out ../log/*.log -f 
