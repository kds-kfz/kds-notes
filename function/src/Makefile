.PHNOY : clean

INCS := ../common

CC = gcc
CXX = g++

FLAG=-lm

COMMONSRCS=$(wildcard $(INCS)/*.c)
COMMONOBJS=$(patsubst %.c,%.o,$(COMMONSRCS))	

SRC=$(wildcard ./*.cpp)
OBJ=$(patsubst %.cpp,%.o,$(SRC))


all : 1.a.out 2.a.out ser.out cli.out main.out
1.a.out : 1.生成ocr应答数据.o $(INCS)/cJSON.o
	$(CXX) -o $@ $^ $(FLAG)
2.a.out : 6.解析ocr应答数据.o $(INCS)/cJSON.o
	$(CXX) -o $@ $^ $(FLAG)
ser.out : serve.o klog.o kfile.o ksocket.o kipc.o ksignal.o
	$(CXX) -o $@ $^
cli.out : client.o klog.o kfile.o ksocket.o kipc.o
	$(CXX) -o $@ $^
main.out : main.o kcfg.o kfile.o klog.o $(INCS)/cJSON.o kipc.o ksignal.o
	$(CXX) -o $@ $^

%.o : %.c
	$(CC) -o $@ -c $<
%.o : %.cpp
	$(CXX) -o $@ -c $< -std=c++0x $(libs) -g

clean:
	rm $(OBJ) $(COMMONOBJS) *.out *.o ../log/*.log -f 
