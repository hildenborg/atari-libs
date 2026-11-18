#	Copyright (C) 2026 Mikael Hildenborg
#	SPDX-License-Identifier: MIT

# AES library test programs, run in Hatari by run.sh.
# GEN_PATH: directory with a generated and built library (make GEN_PATH=<dir> all in the repo root).
#           Without it, the library installed in the toolchain is used.
# The harness and the runner are shared with the TOS tests in ../tos.

.DEFAULT_GOAL := all

BUILD_DIR ?= build
GEN_PATH ?=
TOOLCHAIN_M68K ?= $(HOME)/toolchain/m68k-atari-elf
TOOLCHAIN_BIN := $(TOOLCHAIN_M68K)/bin
CC := $(TOOLCHAIN_BIN)/m68k-atari-elf-gcc
ELFTOPRG := $(TOOLCHAIN_BIN)/m68k-atari-elf-prg
TOS_DIR := ../tos

CFLAGS := -O1 -g -Wall -Wextra -Wno-unused-parameter -I$(TOS_DIR)
LDFLAGS :=
# Relink when the library changes.
LIBFILES := $(addprefix $(if $(GEN_PATH),$(GEN_PATH),$(TOOLCHAIN_M68K)/m68k-atari-elf/lib)/,libaes.a libvdi.a libtos.a)
ifneq ($(GEN_PATH),)
CFLAGS := -I$(GEN_PATH) $(CFLAGS)
LDFLAGS += -L$(GEN_PATH)
endif
LIBS := -laes -lvdi -ltos

TESTS := $(basename $(wildcard a_*.c))
TESTS := $(filter-out a_util,$(TESTS))
PRGS := $(addprefix $(BUILD_DIR)/,$(addsuffix .prg,runner $(TESTS)))
# Runner program list: the test names in upper case, as a C string list.
comma := ,
empty :=
space := $(empty) $(empty)
RUNNER_PROGS := $(subst $(space),$(comma) ,$(patsubst %,"%",$(shell echo $(TESTS) | tr a-z A-Z)))

all: $(PRGS)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c a_util.h a_tree.h $(TOS_DIR)/harness.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/harness.o: $(TOS_DIR)/harness.c $(TOS_DIR)/harness.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# The runner gets the program list and directory from a generated header.
$(BUILD_DIR)/progs.h: $(wildcard a_*.c) | $(BUILD_DIR)
	printf '%s\n' '#define TEST_DIR "C:\\AESTEST\\"' '#define TEST_PROGS $(RUNNER_PROGS)' > $@

$(BUILD_DIR)/runner.o: $(TOS_DIR)/runner.c $(TOS_DIR)/harness.h $(BUILD_DIR)/progs.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -include $(BUILD_DIR)/progs.h -c $< -o $@

$(BUILD_DIR)/runner.elf: $(BUILD_DIR)/runner.o $(BUILD_DIR)/harness.o $(LIBFILES)
	$(CC) $(filter %.o,$^) -o $@ $(LDFLAGS) -ltos

$(BUILD_DIR)/%.elf: $(BUILD_DIR)/%.o $(BUILD_DIR)/a_util.o $(BUILD_DIR)/harness.o $(LIBFILES)
	$(CC) $(filter %.o,$^) -o $@ $(LDFLAGS) $(LIBS)

$(BUILD_DIR)/%.prg: $(BUILD_DIR)/%.elf
	$(ELFTOPRG) $< $@ > /dev/null

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
.SECONDARY:
