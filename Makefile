# Minimal build system for a tiny 64-bit kernel with a lightweight C-based Kconfig flow

CROSS ?= x86_64-elf-
CC      := $(CROSS)gcc
LD      := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy
AS      := $(CC)

# Fall back to the host toolchain when a cross-compiler is unavailable so
# developers without x86_64-elf-* packages can still build and boot the ELF.
ifeq (,$(shell command -v $(CC) 2>/dev/null))
$(info Cross compiler $(CC) not found, using host compiler instead)
CC      := gcc
LD      := ld
OBJCOPY := objcopy
AS      := $(CC)
endif

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

QEMU ?= qemu-system-x86_64
QEMU_FLAGS ?= -m 512M -serial stdio

KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin

SRC := $(SRC_DIR)/kernel.c $(SRC_DIR)/boot.S \
       $(SRC_DIR)/console.c $(SRC_DIR)/memory.c $(SRC_DIR)/string.c $(SRC_DIR)/rootfs.c \
       $(SRC_DIR)/drivers/serial.c $(SRC_DIR)/drivers/keyboard.c $(SRC_DIR)/drivers/cpu.c
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SRC)))        $(patsubst $(SRC_DIR)/%.S,$(BUILD_DIR)/%.o,$(filter %.S,$(SRC)))

CFLAGS  := -m64 -ffreestanding -nostdlib -fno-stack-protector -Wall -Wextra -Iinclude -include $(KCONFIG_AUTOHEADER)
LDFLAGS := -T link.ld

MAP_FILE := $(BUILD_DIR)/kernel.map

.PHONY: all clean iso run menuconfig defconfig config syncconfig dirs_iso

all: $(KCONFIG_AUTOCONFIG) $(KERNEL_ELF) iso

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo CC $@
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S | $(BUILD_DIR)
	@mkdir -p $(dir $@)
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
ifeq ($(CONFIG_GENERATE_MAP),y)
LDFLAGS += -Map $(MAP_FILE)
endif

ALL_BINS := $(KERNEL_ELF)
ifeq ($(CONFIG_ENABLE_BIN),y)
ALL_BINS += $(KERNEL_BIN)
endif

dirs_iso:
	@mkdir -p $(GRUB_DIR)

iso: $(ALL_BINS) dirs_iso
	@echo Creating ISO directory structure...
	@mkdir -p $(GRUB_DIR)
	cp $(KERNEL_ELF) $(BOOT_DIR)/kernel.elf
	printf '%s\n' \
		"menuentry \"Z-Kernel\" {" \
		"  multiboot2 /boot/kernel.elf" \
		"  boot" \
		"}" > $(GRUB_DIR)/grub.cfg
	@if [ "$(CONFIG_USE_GRUB)" = "y" ]; then \
		if command -v grub-mkrescue >/dev/null 2>&1; then \
			grub-mkrescue -o zkernel.iso $(ISO_DIR) >/dev/null; \
			echo "Created zkernel.iso"; \
		else \
			echo "Error: grub-mkrescue not found. Install grub2-common/grub-efi or set CONFIG_USE_GRUB=n in menuconfig."; \
			rm -f zkernel.iso; \
			exit 1; \
		fi; \
	else \
		echo "Skipping iso creation (CONFIG_USE_GRUB != y)"; \
	fi
run: all
	@if [ -f zkernel.iso ]; then \
	echo "Booting via GRUB ISO..."; \
	$(QEMU) -cdrom zkernel.iso $(QEMU_FLAGS); \
	else \
	if [ "$(CONFIG_USE_GRUB)" = "y" ]; then \
	echo "Cannot boot: zkernel.iso missing and CONFIG_USE_GRUB=y. Install grub-mkrescue or disable CONFIG_USE_GRUB."; \
	exit 1; \
	fi; \
	echo "Booting kernel ELF directly..."; \
	$(QEMU) -kernel $(KERNEL_ELF) $(QEMU_FLAGS); \
	fi

run-elf: $(KERNEL_ELF)
	$(QEMU) -kernel $(KERNEL_ELF) $(QEMU_FLAGS)

run-iso: iso
	$(QEMU) -cdrom zkernel.iso $(QEMU_FLAGS)

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
	@echo "CONFIG_ENABLE_SERIAL_DEBUG=$(CONFIG_ENABLE_SERIAL_DEBUG)"
	@echo "CONFIG_ENABLE_PAGING=$(CONFIG_ENABLE_PAGING)"
	@echo "CONFIG_BOOT_BANNER=$(CONFIG_BOOT_BANNER)"
	@echo "CONFIG_LOG_MEMORY_MAP=$(CONFIG_LOG_MEMORY_MAP)"
	@echo "CONFIG_HEAP_DEMO=$(CONFIG_HEAP_DEMO)"
	@echo "CONFIG_LOG_ROOTFS=$(CONFIG_LOG_ROOTFS)"
	@echo "CONFIG_ENABLE_KEYBOARD_ECHO=$(CONFIG_ENABLE_KEYBOARD_ECHO)"
	@echo "CONFIG_GENERATE_MAP=$(CONFIG_GENERATE_MAP)"
	@echo "CONFIG_FRAMEBUFFER_ENABLE=$(CONFIG_FRAMEBUFFER_ENABLE)"
	@echo "CONFIG_FRAMEBUFFER_TEST_PATTERN=$(CONFIG_FRAMEBUFFER_TEST_PATTERN)"
	@echo "CONFIG_OPT_LEVEL=$(CONFIG_OPT_LEVEL)"
	@echo "CONFIG_ENABLE_BIN=$(CONFIG_ENABLE_BIN)"
	@echo "CONFIG_CUSTOM_CFLAGS=$(CONFIG_CUSTOM_CFLAGS)"

clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) zkernel.iso $(KCONFIG_CONFIG) $(KCONFIG_AUTOCONFIG) $(KCONFIG_AUTOHEADER) $(KCONFIG_MK) include/config include/generated
	$(MAKE) -C scripts/kconfig clean
