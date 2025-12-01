# Minimal build system for a tiny 64-bit kernel
# Supports a menuconfig-like prompt that writes config.mk
# Variables (can be overridden on the command line):
#  CROSS ?= x86_64-elf-
#  CC, LD, AS, OBJCOPY are derived from CROSS
#  ENABLE_HELLO: enable the hello message (1)
#  LANGUAGE: en or de (default en)
#  USE_GRUB: if 1, create GRUB ISO with grub-mkrescue (default 1)

CROSS ?= x86_64-elf-
CC      := $(CROSS)gcc
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
AS      := $(CC)

BUILD_DIR := build
SRC_DIR := src
ISO_DIR := iso
BOOT_DIR := $(ISO_DIR)/boot
GRUB_DIR := $(BOOT_DIR)/grub

KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin

# include a generated config.mk if present, otherwise defaults
-include config.mk

ENABLE_HELLO ?= 1
LANGUAGE ?= en
USE_GRUB ?= 1
ENABLE_DEBUG ?= 0
OPT_LEVEL ?= O2
ENABLE_BIN ?= 1
CUSTOM_CFLAGS ?=

# base CFLAGS
CFLAGS  := -m64 -ffreestanding -nostdlib -fno-stack-protector -Wall -Wextra -Iinclude

# debug and optimization
ifeq ($(ENABLE_DEBUG),1)
CFLAGS += -g
endif

ifeq ($(OPT_LEVEL),O0)
CFLAGS += -O0
else
CFLAGS += -O2
endif

# append any custom flags provided via menuconfig
ifneq ($(strip $(CUSTOM_CFLAGS)),)
CFLAGS += $(CUSTOM_CFLAGS)
endif

LDFLAGS := -T link.ld

SRC := $(SRC_DIR)/kernel.c $(SRC_DIR)/boot.S
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SRC))) $(patsubst $(SRC_DIR)/%.S,$(BUILD_DIR)/%.o,$(filter %.S,$(SRC)))

.PHONY: all clean iso run menuconfig config

ALL_BINS := $(KERNEL_ELF)
ifeq ($(ENABLE_BIN),1)
ALL_BINS += $(KERNEL_BIN)
endif

all: $(ALL_BINS) iso

# generate build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# compile C files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo CC $@
	$(CC) $(CFLAGS) -c $< -o $@ -DLANGUAGE_$(shell echo $(LANGUAGE) | tr a-z A-Z) $(if $(ENABLE_HELLO),-DENABLE_HELLO,)

# assemble .S files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S | $(BUILD_DIR)
	@echo AS $@
	$(AS) $(CFLAGS) -c $< -o $@

# link ELF
$(KERNEL_ELF): $(OBJ)
	@echo LD $@
	$(LD) $(LDFLAGS) -o $@ $^

# optional raw binary (not used by GRUB but provided)
$(KERNEL_BIN): $(KERNEL_ELF)
	@echo OBJCOPY $@
	$(OBJCOPY) -O binary $< $@

# build ISO with GRUB
iso: $(KERNEL_ELF)
	@echo Creating ISO directory structure...
	mkdir -p $(GRUB_DIR)
	cp $(KERNEL_ELF) $(BOOT_DIR)/kernel.elf
	cat > $(GRUB_DIR)/grub.cfg <<'EOF'
	menuentry "Z-Kernel" {
	  multiboot2 /boot/kernel.elf
	  boot
	}
	EOF
	@if [ $(USE_GRUB) -eq 1 ]; then \
		if command -v grub-mkrescue >/dev/null 2>&1; then \
			grub-mkrescue -o zkernel.iso $(ISO_DIR) >/dev/null; \
			echo "Created zkernel.iso"; \
		else \
			echo "Error: grub-mkrescue not found. Install grub2-common/grub-efi or use USE_GRUB=0"; exit 1; \
		fi; \
	else \
		echo "Skipping iso creation (USE_GRUB != 1)"; \
	fi

# run in qemu
run: all
	qemu-system-x86_64 -cdrom zkernel.iso -m 512M -serial stdio

# menuconfig: simple shell prompt to write config.mk
menuconfig:
	@$(SHELL) scripts/menuconfig

config: # show effective config
	@echo "ENABLE_HELLO=$(ENABLE_HELLO)"
	@echo "LANGUAGE=$(LANGUAGE)"
	@echo "USE_GRUB=$(USE_GRUB)"
	@echo "ENABLE_DEBUG=$(ENABLE_DEBUG)"
	@echo "OPT_LEVEL=$(OPT_LEVEL)"
	@echo "ENABLE_BIN=$(ENABLE_BIN)"
	@echo "CUSTOM_CFLAGS=$(CUSTOM_CFLAGS)"

clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) zkernel.iso config.mk
