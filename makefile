#	Copyright (C) 2025 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

.DEFAULT_GOAL := all

MULTILIB_PATH ?= .
MULTILIB_FLAGS ?=
MULTILIB_TARGET ?= m68k-atari-elf
MULTILIB_TOOLKIT ?= $(HOME)/toolchain/m68k-atari-elf

ifeq ($(MULTILIB_PATH), .)
	MULTILIB_PATH :=
else
	MULTILIB_PATH := /$(MULTILIB_PATH)
endif

GEN_PATH ?= ./gen
BUILD_PATH := $(GEN_PATH)$(MULTILIB_PATH)

include $(GEN_PATH)/tos.mk
include $(GEN_PATH)/aes.mk
include $(GEN_PATH)/vdi.mk

TOOLKIT_INC	:= $(MULTILIB_TOOLKIT)/$(MULTILIB_TARGET)/include
TOOLKIT_LIB	:= $(MULTILIB_TOOLKIT)/$(MULTILIB_TARGET)/lib$(MULTILIB_PATH)

# Toolkit executables, libraries and directories settings
TOOLKIT_BIN	:= $(MULTILIB_TOOLKIT)/bin
CC := $(TOOLKIT_BIN)/$(MULTILIB_TARGET)-gcc
AR := $(TOOLKIT_BIN)/$(MULTILIB_TARGET)-gcc-ar

#CFLAGS := -Wall -Os -g
CFLAGS := $(MULTILIB_FLAGS) -Wall -Wextra -Os -g -DFAST_VDI #  -DDEBUG

# Tos object list
TOSOBJS := $(foreach source,$(TOS_SOURCES),$(BUILD_PATH)/$(basename $(source)).o)

# AES object list
AESOBJS := $(foreach source,$(AES_SOURCES),$(BUILD_PATH)/$(basename $(source)).o)

# VDI object list
VDIOBJS := $(foreach source,$(VDI_SOURCES),$(BUILD_PATH)/$(basename $(source)).o)

# Make libs
TOS_LIB=$(BUILD_PATH)/libtos.a
AES_LIB=$(BUILD_PATH)/libaes.a
VDI_LIB=$(BUILD_PATH)/libvdi.a

$(TOS_LIB): $(TOSOBJS)
	$(AR) -rcs $@ $^

$(AES_LIB): $(AESOBJS)
	$(AR) -rcs $@ $^

$(VDI_LIB): $(VDIOBJS)
	$(AR) -rcs $@ $^

# c source
$(BUILD_PATH)/%.o: $(GEN_PATH)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

# Create build directory
$(BUILD_PATH): $(shell mkdir -p $(BUILD_PATH))

all: $(BUILD_PATH) $(TOS_LIB) $(AES_LIB) $(VDI_LIB)

clean:
	$(shell rm -r gen)

install:
ifdef MULTILIB_PATH
	$(shell yes | cp -rf $(GEN_PATH)/def_types.h $(TOOLKIT_INC)/def_types.h)
	$(shell yes | cp -rf $(GEN_PATH)/tos.h $(TOOLKIT_INC)/tos.h)
	$(shell yes | cp -rf $(GEN_PATH)/aes.h $(TOOLKIT_INC)/aes.h)
	$(shell yes | cp -rf $(GEN_PATH)/aes_def.h $(TOOLKIT_INC)/aes_def.h)
	$(shell yes | cp -rf $(GEN_PATH)/vdi.h $(TOOLKIT_INC)/vdi.h)
	$(shell yes | cp -rf $(GEN_PATH)/vdi_def.h $(TOOLKIT_INC)/vdi_def.h)
endif
	$(shell yes | cp -rf $(BUILD_PATH)/libtos.a $(TOOLKIT_LIB)/libtos.a)
	$(shell yes | cp -rf $(BUILD_PATH)/libaes.a $(TOOLKIT_LIB)/libaes.a)
	$(shell yes | cp -rf $(BUILD_PATH)/libvdi.a $(TOOLKIT_LIB)/libvdi.a)
	