#	Copyright (C) 2026 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

# TOS library test programs, run in Hatari by run.sh.
# GEN_PATH: directory with a generated and built library (make GEN_PATH=<dir> all in the repo root).
#           Without it, the library installed in the toolchain is used.

.DEFAULT_GOAL := all

BUILD_DIR ?= build
GEN_PATH ?=
TOOLCHAIN_M68K ?= $(HOME)/toolchain/m68k-atari-elf
TOOLCHAIN_BIN := $(TOOLCHAIN_M68K)/bin
CC := $(TOOLCHAIN_BIN)/m68k-atari-elf-gcc
ELFTOPRG := $(TOOLCHAIN_BIN)/m68k-atari-elf-prg

CFLAGS := -O1 -g -Wall -Wextra -Wno-unused-parameter
LDFLAGS :=
# Relink when the library changes.
LIBFILES := $(addprefix $(if $(GEN_PATH),$(GEN_PATH),$(TOOLCHAIN_M68K)/m68k-atari-elf/lib)/,libtos.a)
ifneq ($(GEN_PATH),)
CFLAGS += -I$(GEN_PATH)
LDFLAGS += -L$(GEN_PATH)
endif

TESTS := $(basename $(wildcard t_*.c))
PRGS := $(addprefix $(BUILD_DIR)/,$(addsuffix .prg,runner child $(TESTS)))

all: $(PRGS)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c harness.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# The child program doesn't use the harness.
$(BUILD_DIR)/child.elf: $(BUILD_DIR)/child.o $(LIBFILES)
	$(CC) $(filter %.o,$^) -o $@ $(LDFLAGS) -ltos

$(BUILD_DIR)/%.elf: $(BUILD_DIR)/%.o $(BUILD_DIR)/harness.o $(LIBFILES)
	$(CC) $(filter %.o,$^) -o $@ $(LDFLAGS) -ltos

$(BUILD_DIR)/%.prg: $(BUILD_DIR)/%.elf
	$(ELFTOPRG) $< $@ > /dev/null

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
.SECONDARY:
