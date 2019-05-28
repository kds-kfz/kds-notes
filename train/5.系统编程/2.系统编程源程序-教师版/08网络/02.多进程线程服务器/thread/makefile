SRC = $(wildcard *.c)
OBJ = $(patsubst %.c,%.o,$(SRC))
FLAG = -pthread

all: server client
server : server.o
	gcc server.o -o server $(FLAG)
client : client.o
	gcc client.o -o client $(FLAG)

%.o : %.c
	gcc -c $< -o $@  $(FLAG)

.PHNOY:clean
clean:
	rm -rf $(OBJ) server client
