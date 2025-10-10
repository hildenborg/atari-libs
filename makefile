.DEFAULT_GOAL := all

include gen/tos.mk
include gen/aes.mk
include gen/vdi.mk
#include gen/line_a.mk

# Project build architecture settings
TOOLKIT	?= $(HOME)/toolchain/m68k-atari-elf

TOOLKIT_INC	:= $(TOOLKIT)/m68k-atari-elf/include
TOOLKIT_LIB	:= $(TOOLKIT)/m68k-atari-elf/lib

# Use m68k-atari-elf toolchain
# Toolkit executables, libraries and directories settings
TOOLKIT_BIN	:= $(TOOLKIT)/bin
#CPU := 68030
CC := $(TOOLKIT_BIN)/m68k-atari-elf-gcc
AR := $(TOOLKIT_BIN)/m68k-atari-elf-gcc-ar

CFLAGS := -Wall -Os -g
#CFLAGS := -Wall -Os -g -mcpu=$(CPU)

# Tos object list
TOSOBJS := $(foreach source,$(TOS_SOURCES),gen/$(basename $(source)).o)

# AES object list
AESOBJS := $(foreach source,$(AES_SOURCES),gen/$(basename $(source)).o)

# VDI object list
VDIOBJS := $(foreach source,$(VDI_SOURCES),gen/$(basename $(source)).o)

# LINE-A object list
#LINE_AOBJS := $(foreach source,$(LINE_A_SOURCES),gen/$(basename $(source)).o)

# Make lib
gen/libtos.a: $(TOSOBJS)
	$(AR) -rcs $@ $^

gen/libaes.a: $(AESOBJS)
	$(AR) -rcs $@ $^

gen/libvdi.a: $(VDIOBJS)
	$(AR) -rcs $@ $^

#gen/libline_a.a: $(LINE_AOBJS)
#	$(AR) -rcs $@ $^

# c source
%.o: gen/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

all:	gen/libtos.a gen/libaes.a gen/libvdi.a # gen/libline_a.a

clean:
	$(shell rm -r gen)

install:
	$(shell yes | cp -rf gen/tos.h $(TOOLKIT_INC)/tos.h)
	$(shell yes | cp -rf gen/aes.h $(TOOLKIT_INC)/aes.h)
	$(shell yes | cp -rf gen/aes_def.h $(TOOLKIT_INC)/aes_def.h)
	$(shell yes | cp -rf gen/vdi.h $(TOOLKIT_INC)/vdi.h)
	$(shell yes | cp -rf gen/vdi_def.h $(TOOLKIT_INC)/vdi_def.h)
#	$(shell yes | cp -rf gen/line_a.h $(TOOLKIT_INC)/line_a.h)
#	$(shell yes | cp -rf gen/line_a_def.h $(TOOLKIT_INC)/line_a_def.h)
	$(shell yes | cp -rf gen/libtos.a $(TOOLKIT_INC)/libtos.a)
	$(shell yes | cp -rf gen/libaes.a $(TOOLKIT_INC)/libaes.a)
	$(shell yes | cp -rf gen/libvdi.a $(TOOLKIT_INC)/libvdi.a)
#	$(shell yes | cp -rf gen/libline_a.a $(TOOLKIT_INC)/libline_a.a)
	