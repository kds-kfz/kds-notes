SRC = $(wildcard *.c)
OBJ = $(patsubst %.c,%.o,$(SRC))	

all: server client

server : server.o
	gcc server.o -o server
client : client.o
	gcc client.o -o client

%.o : %.c
	gcc -c $< -o $@

.PHNOY: clean
clean:
	rm -rf $(OBJ) server client

