SRC = $(wildcard *.c) #表示取出当前目录下的所有.c文件
OBJ = $(patsubst %.c,%.o,$(SRC)) #把所有的.c文件转换成同名的.o文件
ARG = a.out  #makefile中的 目标:依赖 分号左则表示目标,右侧表示依赖

$(ARG):$(OBJ)
	gcc $^ -o $@
%.o : %.c
	gcc -c $< -o $@

.PHONY:clean
clean:
	rm -rf *.c *.o 
		 
