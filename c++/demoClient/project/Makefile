# Linux x64 客户端示例构建；链接 SocketClient 动态库。
CXX ?= g++
PLATFORM ?= Lnx64
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
COMMON := $(abspath $(ROOT)/..)
CLIENT_LIB ?= $(COMMON)/SocketClient/lib/$(PLATFORM)
OBJ_DIR := $(ROOT)/build/$(PLATFORM)
OUT_DIR := $(ROOT)/bin/$(PLATFORM)
SOURCES := $(wildcard $(ROOT)/src/*.cpp)
OBJECTS := $(patsubst $(ROOT)/src/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
TARGET := $(OUT_DIR)/demoClient
CPPFLAGS += -I$(COMMON)/SocketClient/include
CXXFLAGS += -std=c++17 -O2 -g -Wall -Wextra
LDFLAGS += -L$(CLIENT_LIB) -Wl,-rpath,$(CLIENT_LIB)
LDLIBS += -lsocketclient -pthread

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(OUT_DIR)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(OBJ_DIR)/%.o: $(ROOT)/src/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET)
