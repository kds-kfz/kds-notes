SRC = $(wildcard *.c)
OBJ = $(patsubst %.c,%.o,$(SRC))
FLAG = -pthread
CC = gcc
all : serve.out client.out
serve.out : 3.多进程_serve.o
	 $(CXX) -o $@ $^
client.out : 2.client.o
	 $(CXX) -o $@ $^
%.o : %.c
	gcc -c $< -o $@ $(FLAG)
.PHNOY:clean
clean:
	rm $(OBJ) *.out -f
