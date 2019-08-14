.PHNOY : clean

INCS := ../common

CC = gcc
CXX = g++

FLAG=-lm

COMMONSRCS=$(wildcard $(INCS)/*.c)
COMMONOBJS=$(patsubst %.c,%.o,$(COMMONSRCS))	

SRC=$(wildcard ./*.cpp)
OBJ=$(patsubst %.cpp,%.o,$(SRC))


all : 1.a.out 2.a.out PubFun.out
1.a.out : 1.生成ocr应答数据.o $(INCS)/cJSON.o
	$(CXX) -o $@ $^ $(FLAG)
2.a.out : 2.解析ocr应答数据.o $(INCS)/cJSON.o
	$(CXX) -o $@ $^ $(FLAG)
PubFun.out : PubFun.o
	$(CXX) -o $@ $^

%.o : %.c
	$(CC) -o $@ -c $<
%.o : %.cpp
	$(CXX) -o $@ -c $<

clean:
	rm $(OBJ) $(COMMONOBJS) *.out -f
