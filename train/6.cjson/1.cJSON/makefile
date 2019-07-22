SRC=$(wildcard *.c)
OBJ=$(patsubst %.c,%.o,$(SRC))
FLAG=-lm
all : a.out1 a.out2 a.out3 a.out4 a.out51 a.out52 a.out6 a.out7 a.out8
a.out1 : 1.生成json.o cJSON.o
	gcc -o $@ $^ $(FLAG)
a.out2 : 2.生成json数组.o cJSON.o
	gcc -o $@ $^ $(FLAG)
a.out3 : 3.解析json.o cJSON.o
	gcc -o $@ $^ $(FLAG)
a.out4 : 4.生成对象.o cJSON.o
	gcc -o $@ $^ $(FLAG)
a.out51 : 5.对象解析.o cJSON.o
	gcc -o $@ $^ $(FLAG)
a.out52 : 5.删除.o cJSON.o
	gcc -o $@ $^ $(FLAG)
a.out6 : 6.练习.o cJSON.o
	gcc -o $@ $^ $(FLAG)
a.out7 : 7.练习.o cJSON.o
	gcc -o $@ $^ $(FLAG)
a.out8 : 8.解析.o cJSON.o
	gcc -o $@ $^ $(FLAG)
%.o : %.c
	gcc -o $@ -c $<
.PHNOY : clean
clean:
	rm $(OBJ) a.out1 a.out2 a.out3 a.out4 a.out51 a.out52 a.out6 a.out7 a.out8
