SRC = $(wildcard *.cpp)  #取出当前目录下所有.cpp文件
OBJ = $(patsubst %.cpp,%.o,$(SRC)) #把当前目录下所有.cpp文件转换成.o文件(仅是文件名的更改)
TARGET = a.out
FLAG = -std=c++11 -pthread

$(TARGET) : $(OBJ)			  
	g++ $^ -o  $@ $(FLAG)  			 
%.o : %.cpp
	g++ -c $< -o $@ $(FLAG)			 

.PHONY : clean
clean:
	rm -rf $(TARGET) $(OBJ)
