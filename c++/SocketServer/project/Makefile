CPP := $(CROSS)g++
CC  := $(CROSS)gcc

#x64 Win32 Lnx64
PLATFORM=Lnx64

SUB_DIRS := ./vs2022/SocketServer
RELEASE_LIB_DIR := ../lib/$(PLATFORM)

LDLIBS := -lstdc++ -lpthread -lm -ldl
#LDLIBS += -lnsdk
LDLIBS += -lhpsocket

CXXFLAGS := -g -g3 -O0 -fPIC -Wall -Werror -Wno-sign-compare -Wno-unused-variable -Wno-deprecated-declarations -Wno-unused-function -std=c++17
#不打印任何警告
#CXXFLAGS := -g -g3 -O0 -fPIC -w -std=c++17

INC_FLAGS := -I $(SUB_DIRS)
INC_FLAGS += -I $(SUB_DIRS)/vender/nsdk/include
INC_FLAGS += -I $(SUB_DIRS)/vender/hpsocket/include/$(PLATFORM)

LDFLAGS := -L $(SUB_DIRS)/vender/hpsocket/lib/$(PLATFORM)
#LDFLAGS += -L $(SUB_DIRS)/vender/nsdk/lib/$(PLATFORM)

CPPSRC_ALL := $(foreach dir, $(SUB_DIRS), $(wildcard $(dir)/*.cpp))
CPPOBJ := $(patsubst %.cpp,%.o,$(CPPSRC_ALL))

SRC := $(foreach dir, $(SUB_DIRS), $(wildcard $(dir)/*.c))
OBJ := $(SRC:%.c=%.o)

#CFLAGS := -g -g3 -O0 -fPIC -fsigned-char -Wall -Werror
CFLAGS := -g -g3 -O0 -fsigned-char
CFLAGS += -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-incompatible-pointer-types
CFLAGS += $(INC_FLAGS) -D_GNU_SOURCE

SHAREDOK := true

ifeq ($(SHAREDOK), true)
TARGET := libsocketserver.so

all: $(TARGET)
libsocketserver.so: $(CPPOBJ)
	$(CPP) -fPIC -shared -o $@ $(CPPOBJ) $(LDFLAGS) $(LDLIBS)
	mv $@ $(RELEASE_LIB_DIR)
else
TARGET := libsocketserver.a

all: $(TARGET)
libsocketserver.a: $(CPPOBJ)
	ar -rv $@ $(CPPOBJ)
	mv $@ $(RELEASE_LIB_DIR)
endif

%.o : %.cpp
	$(CPP) -c $< -o $@  $(INC_FLAGS) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

%.o : %.c
	$(CC) -c $< -o $@  $(INC_FLAGS) $(CFLAGS)

clean:
	-@rm -f $(RELEASE_LIB_DIR)/libsocketserver.*
	-@rm -f $(CPPOBJ) $(OBJ)
