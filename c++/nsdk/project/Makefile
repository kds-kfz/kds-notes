CPP := $(CROSS)g++
CC  := $(CROSS)gcc

#x64 Win32 Lnx64
PLATFORM=Lnx64

SUB_DIRS := ../src
RELEASE_LIB_DIR := ../lib/$(PLATFORM)

LDLIBS := -pthread -ldl

CXXFLAGS := -g -g3 -O0 -fPIC -Wall -Werror -Wno-sign-compare -Wno-unused-variable -Wno-deprecated-declarations -Wno-unused-function -std=c++17 -D_GLIBCXX_USE_CXX11_ABI=0
#不打印任何警告
#CXXFLAGS := -g -g3 -O0 -fPIC -w -std=c++17

INC_FLAGS := -I ../include

# 与 VS2022 发布工程保持同一组正式编译单元。src 目录还包含历史样例和
# 未进入发布 ABI 的实验实现，不能通过通配符自动吸入。
CPPSRC_ALL := $(SUB_DIRS)/nsdk.cpp \
	$(SUB_DIRS)/nsdk_event.cpp \
	$(SUB_DIRS)/nsdk_thread.cpp
CPPOBJ := $(patsubst %.cpp,%.o,$(CPPSRC_ALL))

SRC := $(foreach dir, $(SUB_DIRS), $(wildcard $(dir)/*.c))
OBJ := $(SRC:%.c=%.o)

#CFLAGS := -g -g3 -O0 -fPIC -fsigned-char -Wall -Werror
CFLAGS := -g -g3 -O0 -fsigned-char
CFLAGS += -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-incompatible-pointer-types
CFLAGS += $(INC_FLAGS) -D_GNU_SOURCE

SHAREDOK ?= true

ifeq ($(SHAREDOK), true)
TARGET := libnsdk.so

all: $(TARGET)
libnsdk.so: $(CPPOBJ) $(OBJ)
	@mkdir -p $(RELEASE_LIB_DIR)
	$(CPP) -fPIC -shared -o $@ $(CPPOBJ) $(OBJ) $(LDLIBS)
	mv $@ $(RELEASE_LIB_DIR)
else
TARGET := libnsdk.a

all: $(TARGET)
libnsdk.a: $(CPPOBJ) $(OBJ)
	@mkdir -p $(RELEASE_LIB_DIR)
	$(AR) -rv $@ $(CPPOBJ) $(OBJ)
	mv $@ $(RELEASE_LIB_DIR)
endif

%.o : %.cpp
	$(CPP) -c $< -o $@  $(INC_FLAGS) $(CXXFLAGS)

%.o : %.c
	$(CC) -c $< -o $@  $(INC_FLAGS) $(CFLAGS)

clean:
	-@rm -f $(RELEASE_LIB_DIR)/libnsdk.*
	-@rm -f $(CPPOBJ) $(OBJ)
