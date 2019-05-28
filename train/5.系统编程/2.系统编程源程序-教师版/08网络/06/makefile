SRC = $(wildcard *.c)
OBJ = $(patsubst %.c,%.o,$(SRC))

all: server client

server : threadpool.o  epoll.o
	gcc threadpool.o epoll.o -o server -pthread
client : threadpool.o client.o
	gcc threadpool.o client.o -o clietn -pthread

%.o : %.c 
	gcc -c $< -o $@ -pthread

.PHONY:clean
clean:
	rm -rf server client $(OBJ)
