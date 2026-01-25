#	Copyright (C) 2026 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

.DEFAULT_GOAL := all

# Project settings
#CPU := 68030
BUILD_DIR := build
SRC_DIR := .
CFLAGS := -O0 -g -Wall -I../gen
#CFLAGS := -g -Wall -I../gen -mcpu=$(CPU)
LDFLAGS := -L../gen
TARGET_NAME ?= libtst

# Toolchain command settings
TOOLCHAIN_M68K	?= $(HOME)/toolchain/m68k-atari-elf
TOOLCHAIN_BIN := $(TOOLCHAIN_M68K)/bin
CC := $(TOOLCHAIN_BIN)/m68k-atari-elf-gcc
LD := $(TOOLCHAIN_BIN)/m68k-atari-elf-gcc
DUMP := $(TOOLCHAIN_BIN)/m68k-atari-elf-objdump
ELFTOPRG := $(TOOLCHAIN_BIN)/m68k-atari-elf-prg

# Targets
TARGET_PRG := $(BUILD_DIR)/$(TARGET_NAME).prg
TARGET_ELF := $(BUILD_DIR)/$(TARGET_NAME)

# Compile
$(BUILD_DIR)/main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

# Convert elf to prg
$(TARGET_PRG): $(TARGET_ELF)
	$(ELFTOPRG) $< $@

# Build elf executable
$(TARGET_ELF): $(BUILD_DIR)/main.o
	$(LD) $(LDFLAGS) $^ -o $@ -laes -ltos -lvdi

# Create build directory
$(BUILD_DIR): $(shell mkdir -p $(BUILD_DIR))

all:	$(BUILD_DIR) $(TARGET_PRG)

clean:
	$(shell rm -r $(BUILD_DIR))

