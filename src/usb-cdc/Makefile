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
CFLAGS += -I. -I../http

LDFLAGS := 

export CFLAGS LDFLAGS

TOPDIR := $(shell pwd)
export TOPDIR


obj-y += uart_cdc.o uart.o


all : 
	make -C ./ -f $(TOPDIR)/Makefile.build
	$(CPP) -o $(TARGET) built-in.o $(LDFLAGS)

clean:
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.d")
	rm -f $(TARGET)
