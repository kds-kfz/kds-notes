# Linux x64 动态库构建；依赖目标机器的 HP-Socket 与 OpenSSL 开发包。
CXX ?= g++
PLATFORM ?= Lnx64
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
HP_INCLUDE ?= /usr/local/include
HP_LIB_DIR ?= /usr/local/lib
OBJ_DIR := $(ROOT)/build/$(PLATFORM)
OUT_DIR := $(ROOT)/lib/$(PLATFORM)
SOURCES := $(wildcard $(ROOT)/src/*.cpp)
OBJECTS := $(patsubst $(ROOT)/src/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
TARGET := $(OUT_DIR)/libsocketclient.so
CPPFLAGS += -I$(ROOT)/include -I$(HP_INCLUDE)
CXXFLAGS += -std=c++17 -O2 -g -fPIC -Wall -Wextra
LDFLAGS += -L$(HP_LIB_DIR)
LDLIBS += -lhpsocket -lcrypto -pthread -ldl

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(OUT_DIR)
	$(CXX) -shared -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(OBJ_DIR)/%.o: $(ROOT)/src/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET)
