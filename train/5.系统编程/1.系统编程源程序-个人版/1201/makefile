SRC = $(wildcard *.c) 
OBJ = $(patsubst %.c,%.o,$(SRC)) 
TARGET = a.out
FLAG =  -pthread
$(TARGET):$(OBJ)
	    gcc $^ -o $@ $(FLAG)
%.o : %.c
	    gcc -c $< -o $@ $(FLAG)
.PHONY : clean
clean:
	    rm -rf $(OBJ)
