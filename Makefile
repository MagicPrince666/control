CROSS_COMPILE = arm-linux-gnueabihf-
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CROSS_COMPILE)g++
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm

STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump

export AS LD CC CPP AR NM
export STRIP OBJCOPY OBJDUMP

CFLAGS := -Wall -O2 -g 

ifeq ($(CROSS_COMPILE),arm-linux-gnueabihf-)
CFLAGS += -I./src/md5 -I./src/http -I./src/usb-cdc -I./src/qianxun -I./src/server -I./src/server/include -I./src/tcp_server 
#CFLAGS += -I./src/rssi
LDFLAGS := -lm -lpthread -L./src/qianxun/lib_arm -lrtcm -Wl,-rpath=./src/qianxun -std=c99 #-L./src/server/lib -lssl -lcrypto
else
CFLAGS += -I./src/md5 -I./src/http -I./src/usb-cdc -I./src/qianxun -I./src/server -I./src/tcp_server
CFLAGS += -I./src/rssi
LDFLAGS := -lm -lpthread -L./src/qianxun/lib -lrtcm -Wl,-rpath=./src/qianxun -std=c99 #-lssl -lcrypto
endif

export CFLAGS LDFLAGS

TOPDIR := $(shell pwd)
export TOPDIR

TARGET := tcp_server

obj-y += thread.o
obj-y += src/md5/
obj-y += src/http/
obj-y += src/usb-cdc/
obj-y += src/qianxun/
#obj-y += src/rssi/
#obj-y += src/queue/
obj-y += src/tcp_server/
#obj-y += src/server/


all : 
	make -C ./ -f $(TOPDIR)/Makefile.build
	$(CPP) -o $(TARGET) built-in.o $(LDFLAGS)

clean:
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.d")
	rm -f $(TARGET)
