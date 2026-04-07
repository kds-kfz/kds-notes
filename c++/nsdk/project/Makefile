CPP := $(CROSS)g++
CC  := $(CROSS)gcc

#x64 Win32 Lnx64
PLATFORM=Lnx64

SUB_DIRS := ./vs2022/nsdk
RELEASE_LIB_DIR := ../lib/$(PLATFORM)

LDLIBS := -lstdc++

CXXFLAGS := -g -g3 -O0 -fPIC -Wall -Werror -Wno-sign-compare -Wno-unused-variable -Wno-deprecated-declarations -Wno-unused-function -std=c++17
#不打印任何警告
#CXXFLAGS := -g -g3 -O0 -fPIC -w -std=c++17

INC_FLAGS := -I $(SUB_DIRS)

CPPSRC_ALL := $(foreach dir, $(SUB_DIRS), $(wildcard $(dir)/*.cpp))
CPPOBJ := $(patsubst %.cpp,%.o,$(CPPSRC_ALL))

SRC := $(foreach dir, $(SUB_DIRS), $(wildcard $(dir)/*.c))
OBJ := $(SRC:%.c=%.o)

#CFLAGS := -g -g3 -O0 -fPIC -fsigned-char -Wall -Werror
CFLAGS := -g -g3 -O0 -fsigned-char
CFLAGS += -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-incompatible-pointer-types
CFLAGS += $(INC_FLAGS) -D_GNU_SOURCE

NSDKOBJ := $(SUB_DIRS)/nsdk.o

SHAREDOK := true

ifeq ($(SHAREDOK), true)
TARGET := libnsdk.so

all: $(TARGET)
libnsdk.so: $(NSDKOBJ)
	$(CPP) -fPIC -shared -o $@ $(NSDKOBJ) $(LDLIBS)
	mv $@ $(RELEASE_LIB_DIR)
else
TARGET := libnsdk.a

all: $(TARGET)
libnsdk.a: $(NSDKOBJ)
	ar -rv $@ $(NSDKOBJ)
	mv $@ $(RELEASE_LIB_DIR)
endif

%.o : %.cpp
	$(CPP) -c $< -o $@  $(INC_FLAGS) $(CXXFLAGS)

%.o : %.c
	$(CC) -c $< -o $@  $(INC_FLAGS) $(CFLAGS)

clean:
	-@rm -f $(RELEASE_LIB_DIR)/libnsdk.*
	-@rm -f $(CPPOBJ) $(OBJ)
