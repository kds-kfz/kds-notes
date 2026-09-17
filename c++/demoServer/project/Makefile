# Linux x64 回显服务构建；链接同级 SocketServer 动态库。
CXX ?= g++
PLATFORM ?= Lnx64
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
COMMON := $(abspath $(ROOT)/..)
SERVER_LIB ?= $(COMMON)/SocketServer/lib/$(PLATFORM)
OBJ_DIR := $(ROOT)/build/$(PLATFORM)
OUT_DIR := $(ROOT)/bin/$(PLATFORM)
OBJECT := $(OBJ_DIR)/SocketEchoServer.o
TARGET := $(OUT_DIR)/SocketEchoServer
CPPFLAGS += -I$(COMMON)/SocketServer/include
CXXFLAGS += -std=c++17 -O2 -g -Wall -Wextra
LDFLAGS += -L$(SERVER_LIB) -Wl,-rpath,$(SERVER_LIB)
LDLIBS += -lsocketserver -pthread

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECT)
	@mkdir -p $(OUT_DIR)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(OBJECT): $(ROOT)/src/SocketEchoServer.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJECT) $(TARGET)
