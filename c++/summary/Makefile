.PHONY: default clean

#-O0: 无优化(默认)
#-O和-O1: 使用能减少目标文 件大小以及执行时间并且不会使编译时间明显增加的优化.在编译大型程序的时候会显著增加编译时内存的使用.
#-O2: 包含-O1的优化并增加了不需要在目标文件大小和执行速度上进行折衷的优化.编译器不执行循环展开以及函数内联.此选项将增加编译时间和目标文件的执行性能.
#-Os: 专门优化目标文件大小,执行所有的不增加目标文件大小的-O2优化选项.并且执行专门减小目标文件大小的优化选项.
#-O3: 打开所有-O2的优化选项并且增加-finline-functions, -funswitch-loops,-fpredictive-commoning,-fgcse-after-reload and -ftree-vectorize优化选项

CXX := g++
#使用线程调试编译
ifeq ($(DEBUG),1)
DEBUG_OPTS=-g3 -pg -D__DEBUG__
else
DEBUG_OPTS=-O2
endif

# -Wall 选项意思是编译后显示所有警告
# -w 关闭编译时的警告

CXXFLAGS = -Wall $(DEBUG_OPTS) -g -std=c++11
TARGET := test1 test2 test3 test4 test5 test6

SRCS=$(wildcard ./*.cpp)
OBJS=$(patsubst %.cpp,%.o,$(SRCS))

default : $(TARGET)
test1: 1.虚函数学习.o
	$(CXX) -o $@ $^
test2: 2.类适配器.o
	$(CXX) -o $@ $^
test3: 3.对象适配器.o
	$(CXX) -o $@ $^
test4: 4.函数包装器.o
	$(CXX) -o $@ $^
test5: 5.函数适配器.o
	$(CXX) -o $@ $^
test6: 6.左值与右值.o
	$(CXX) -o $@ $^

%.o:%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

clean:
	rm $(TARGET) $(OBJS) -f
