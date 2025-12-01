# Minimal build system for a tiny 64-bit kernel with a lightweight C-based Kconfig flow

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

KCONFIG_CONFIG ?= .config
KCONFIG_AUTOCONFIG ?= include/config/auto.conf
KCONFIG_AUTOHEADER ?= include/generated/autoconf.h
KCONFIG_MK ?= config.mk

KCONFIG ?= scripts/kconfig/conf
MCONF ?= scripts/kconfig/mconf

KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin

SRC := $(SRC_DIR)/kernel.c $(SRC_DIR)/boot.S
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SRC)))        $(patsubst $(SRC_DIR)/%.S,$(BUILD_DIR)/%.o,$(filter %.S,$(SRC)))

CFLAGS  := -m64 -ffreestanding -nostdlib -fno-stack-protector -Wall -Wextra -Iinclude -include $(KCONFIG_AUTOHEADER)
LDFLAGS := -T link.ld

.PHONY: all clean iso run menuconfig defconfig config syncconfig dirs_iso

all: $(KCONFIG_AUTOCONFIG) $(KERNEL_ELF) iso

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo CC $@
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S | $(BUILD_DIR)
	@echo AS $@
	$(AS) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(OBJ)
	@echo LD $@
	$(LD) $(LDFLAGS) -o $@ $^

$(KERNEL_BIN): $(KERNEL_ELF)
	@echo OBJCOPY $@
	$(OBJCOPY) -O binary $< $@

# Kconfig frontends
$(KCONFIG): scripts/kconfig/conf.c scripts/kconfig/kconfig_common.c scripts/kconfig/kconfig_common.h
	$(MAKE) -C scripts/kconfig conf

$(MCONF): scripts/kconfig/mconf.c scripts/kconfig/kconfig_common.c scripts/kconfig/kconfig_common.h
	$(MAKE) -C scripts/kconfig mconf

# Keep configuration in sync before building sources
$(KCONFIG_AUTOCONFIG): $(KCONFIG_CONFIG) $(KCONFIG)
	@$(KCONFIG) --syncconfig Kconfig

# Default configuration
$(KCONFIG_CONFIG): $(KCONFIG)
	@$(KCONFIG) --defconfig Kconfig

-include $(KCONFIG_AUTOCONFIG)
-include $(KCONFIG_MK)

# map configuration to build knobs
ifeq ($(CONFIG_HELLO),y)
CFLAGS += -DENABLE_HELLO
endif
ifeq ($(CONFIG_LANG_DE),y)
CFLAGS += -DLANGUAGE_DE
else
CFLAGS += -DLANGUAGE_EN
endif
ifeq ($(CONFIG_ENABLE_DEBUG),y)
CFLAGS += -g
endif
ifneq ($(CONFIG_OPT_LEVEL),)
CFLAGS += -$(patsubst "%",%,$(CONFIG_OPT_LEVEL))
endif
ifneq ($(CONFIG_CUSTOM_CFLAGS),)
CFLAGS += $(patsubst "%",%,$(CONFIG_CUSTOM_CFLAGS))
endif

ALL_BINS := $(KERNEL_ELF)
ifeq ($(CONFIG_ENABLE_BIN),y)
ALL_BINS += $(KERNEL_BIN)
endif

dirs_iso:
	@mkdir -p $(GRUB_DIR)

iso: $(ALL_BINS) dirs_iso
	@echo Creating ISO directory structure...
	cp $(KERNEL_ELF) $(BOOT_DIR)/kernel.elf
	cat > $(GRUB_DIR)/grub.cfg <<'EOFCFG'
	menuentry "Z-Kernel" {
	  multiboot2 /boot/kernel.elf
	  boot
	}
	EOFCFG
	@if [ "$(CONFIG_USE_GRUB)" = "y" ]; then \\
		if command -v grub-mkrescue >/dev/null 2>&1; then \\
			grub-mkrescue -o zkernel.iso $(ISO_DIR) >/dev/null; \\
			echo "Created zkernel.iso"; \\
		else \\
			echo "Error: grub-mkrescue not found. Install grub2-common/grub-efi or set CONFIG_USE_GRUB to n"; exit 1; \\
		fi; \\
	else \\
		echo "Skipping iso creation (CONFIG_USE_GRUB != y)"; \\
	fi
run: all
	qemu-system-x86_64 -cdrom zkernel.iso -m 512M -serial stdio

defconfig: $(KCONFIG)
	$(KCONFIG) --defconfig Kconfig

menuconfig: $(MCONF)
	$(MCONF) Kconfig

syncconfig: $(KCONFIG)
	$(KCONFIG) --syncconfig Kconfig

config:
	@echo "CONFIG_HELLO=$(CONFIG_HELLO)"
	@echo "CONFIG_LANG_DE=$(CONFIG_LANG_DE)"
	@echo "CONFIG_USE_GRUB=$(CONFIG_USE_GRUB)"
	@echo "CONFIG_ENABLE_DEBUG=$(CONFIG_ENABLE_DEBUG)"
	@echo "CONFIG_OPT_LEVEL=$(CONFIG_OPT_LEVEL)"
	@echo "CONFIG_ENABLE_BIN=$(CONFIG_ENABLE_BIN)"
	@echo "CONFIG_CUSTOM_CFLAGS=$(CONFIG_CUSTOM_CFLAGS)"

clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) zkernel.iso $(KCONFIG_CONFIG) $(KCONFIG_AUTOCONFIG) $(KCONFIG_AUTOHEADER) $(KCONFIG_MK) include/config include/generated
	$(MAKE) -C scripts/kconfig clean
