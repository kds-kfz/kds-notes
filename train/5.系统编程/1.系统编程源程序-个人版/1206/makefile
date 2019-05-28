SRC = $(wildcard *.c)
OBJ = $(patsubst %.c,%.o,$(SRC))
FLAG = -pthread
all : 1.serve 2.client
serve : serve.o
	gcc ser.o -o ser$(FLAG)
client : client.o
	gcc cli.o -o cli $(FLAG)
%.o : %.c
	gcc -c $< -o $@ $(FLAG)
.PHNOY:clean
clean:
	rm -rf $(OBJ) ser cli
