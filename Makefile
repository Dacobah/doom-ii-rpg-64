BUILD_DIR ?= build
SOURCE_DIR ?= .
N64_MKDFS_ROOT ?= filesystem

N64_ROM_TITLE = "Doom II RPG 64"
N64_ROM_CATEGORY = N
N64_ROM_SAVETYPE = none

# Default toolchain path if not specified in environment
N64_INST ?= /Users/gcoursaget/libdragon-toolchain

include $(N64_INST)/include/n64.mk

# Project sources
SRCS = $(wildcard src/*.cpp) third_party_libs/hash-library/hash-library/md5.cpp
OBJS = $(patsubst ./%,%,$(SRCS:%.cpp=$(BUILD_DIR)/%.o))

# Flags
CXXFLAGS += -std=gnu++17 -fsigned-char -fno-exceptions -fno-rtti \
            -fno-lifetime-dse -fno-delete-null-pointer-checks -fno-sized-deallocation \
            -Iinclude/n64_compat -Isrc -Ithird_party_libs/hash-library/hash-library \
            -Wno-write-strings -Wno-multichar -Wno-nonnull-compare -Wno-error \
            -DN64

N64_CXXFLAGS += -Wno-error

all: doom2rpg.z64

$(BUILD_DIR)/doom2rpg.elf: $(OBJS)
doom2rpg.z64: $(BUILD_DIR)/doom2rpg.dfs

clean:
	rm -rf $(BUILD_DIR) doom2rpg.z64 *.v64 *.elf

.PHONY: all clean
