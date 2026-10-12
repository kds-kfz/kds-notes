# Linux 共享库构建；Windows 仍使用 project/vs2022/SocketServer 工程。
CXX := $(CROSS)g++
PLATFORM ?= Lnx64
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
HP_INCLUDE ?= /usr/local/include
HP_LIB_DIR ?= /usr/local/lib
NSDK_INCLUDE ?= $(ROOT)/../nsdk/include
NSDK_LIB_DIR ?= $(ROOT)/../nsdk/lib/$(PLATFORM)
OBJ_DIR := $(ROOT)/build/$(PLATFORM)
OUT_DIR := $(ROOT)/lib/$(PLATFORM)
SOURCES := $(wildcard $(ROOT)/src/*.cpp)
OBJECTS := $(patsubst $(ROOT)/src/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))
TARGET := $(OUT_DIR)/libsocketserver.so

CPPFLAGS += -I$(ROOT)/include -I$(ROOT)/src -I$(NSDK_INCLUDE) -I$(HP_INCLUDE)
CXXFLAGS += -std=c++17 -O2 -g -fPIC -Wall -Wextra -pthread
LDFLAGS += -L$(HP_LIB_DIR) -L$(NSDK_LIB_DIR) -Wl,-z,defs
LDLIBS += -lhpsocket -lnsdk -pthread -ldl

.PHONY: all check-deps clean
all: $(TARGET)

check-deps:
	@test -f "$(HP_INCLUDE)/hpsocket/HPSocket.h" -o -f "$(HP_INCLUDE)/HPSocket.h" || { echo "缺少 Linux HP-Socket 开发头文件: $(HP_INCLUDE)"; exit 1; }
	@test -f "$(HP_LIB_DIR)/libhpsocket.so" -o -f "$(HP_LIB_DIR)/libhpsocket.a" || { echo "缺少 Linux HP-Socket 库: $(HP_LIB_DIR)"; exit 1; }
	@test -f "$(NSDK_INCLUDE)/nsdk.h" || { echo "缺少 nsdk 开发头文件: $(NSDK_INCLUDE)"; exit 1; }
	@test -f "$(NSDK_LIB_DIR)/libnsdk.so" -o -f "$(NSDK_LIB_DIR)/libnsdk.a" || { echo "缺少 nsdk 库: $(NSDK_LIB_DIR)"; exit 1; }

$(TARGET): $(OBJECTS) | check-deps
	@mkdir -p "$(OUT_DIR)"
	$(CXX) -shared -o "$@" $^ $(LDFLAGS) $(LDLIBS)

$(OBJ_DIR)/%.o: $(ROOT)/src/%.cpp | check-deps
	@mkdir -p "$(OBJ_DIR)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c "$<" -o "$@"

-include $(OBJECTS:.o=.d)

clean:
	$(RM) $(OBJECTS) $(OBJECTS:.o=.d) $(TARGET)
