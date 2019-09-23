.PHNOY : clean

INCS := ../common/include
SRCS := ../common/src
LIBS := ../lib

CC = gcc
CXX = g++

#使用线程调试编译
ifeq ($(DEBUG),1)
DEBUG_OPTS=-g3 -pg -D__DEBUG__
else
DEBUG_OPTS=-O2
endif

CXXFLAGS = -Wall $(DEBUG_OPTS) -g -std=c++11
SHIELD = unknown-pragmas #屏蔽指定警告

#FLAG=-lm #链接数学库

COMMONSRCS=$(wildcard ../common/src/cjson/*.c)
COMMONOBJS=$(patsubst %.c,%.o,$(COMMONSRCS))

SRC=$(wildcard ./*.cpp)
OBJ=$(patsubst %.cpp,%.o,$(SRC))


all : 1.a.out 2.a.out ser.out cli.out main.out 3.a.out
1.a.out : 1.生成ocr应答数据.o $(SRCS)/cjson/cJSON.o
	$(CXX) -o $@ $^
2.a.out : 6.解析ocr应答数据.o $(SRCS)/cjson/cJSON.o
	$(CXX) -o $@ $^
3.a.out : testjson.o kfile.o
	$(CXX) -o $@ $^ $(LIBS)/libjsoncpp.a -I $(INCS)
ser.out : serve.o klog.o kfile.o ksocket.o kipc.o ksignal.o
	$(CXX) -o $@ $^
cli.out : client.o klog.o kfile.o ksocket.o kipc.o
	$(CXX) -o $@ $^
main.out : main.o kcfg.o kfile.o klog.o $(SRCS)/cjson/cJSON.o kipc.o ksignal.o
	$(CXX) -o $@ $^ $(LIBS)/libjsoncpp.a -I $(INCS)

%.o : %.c
	$(CC) -o $@ -c $< -I $(INCS)
%.o : %.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS) $(addprefix -Wno-,$(SHIELD)) -I $(INCS)

clean:
	rm $(OBJ) $(COMMONOBJS) $(OBJ) *.out ../log/*.log -f 
