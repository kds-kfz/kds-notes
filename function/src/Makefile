.PHNOY : clean

COMMONSRCS=$(wildcard ../common/*.c)
COMMONOBJS=$(patsubst %.c,%.o,$(COMMONSRCS))	

SRC=$(wildcard ./*.cpp)
OBJ=$(patsubst %.cpp,%.o,$(SRC))

CC = gcc
CXX = g++
INCS := ../common

FLAG=-lm


all : 1.a.out 2.a.out PubFun.out
1.a.out : 1.生成ocr应答数据.o cJSON.o
	$(CXX) -o $@ $^ $(FLAG)
2.a.out : 2.解析ocr应答数据.o cJSON.o
	$(CXX) -o $@ $^ $(FLAG)
PubFun.out : PubFun.o
	$(CXX) -o $@ $^

%.o : $(COMMONSRCS)
	$(CC) -o $@ -c $<
%.o : %.cpp
	$(CXX)  $(addprefix -I,$(INCS)) -c $< -o $@

clean:
	rm $(OBJ) $(COMMONOBJS) *.out -rf
