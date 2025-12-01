Z-Kernel: Minimal 64-bit kernel

This project demonstrates a minimal 64-bit kernel (not Linux) that prints a "Hello World" style
message to VGA text memory and provides a Makefile with a menuconfig-like interface.

Quick start (prerequisites):
- A cross-compiler toolchain (recommended): x86_64-elf-gcc, x86_64-elf-ld, objcopy
  On Debian/Ubuntu: apt install gcc-multilib nasm grub-pc-bin grub-common xorriso qemu-system-x86
  Alternatively build/install a cross-compiler (recommended for real kernel dev).
- grub-mkrescue to create the ISO
- qemu-system-x86_64 to run

Build and run:
1) Optional: Configure with make menuconfig
   $ make menuconfig
   This creates config.mk; options: ENABLE_HELLO, LANGUAGE (en/de), USE_GRUB

2) Build and create ISO:
   $ make

3) Run in QEMU:
   $ make run

Files of interest:
- src/boot.S   : multiboot2 header + minimal long-mode switch (assembly)
- src/kernel.c : minimal C kernel that writes to VGA memory
- link.ld      : linker script
- Makefile     : build system and ISO creation
- scripts/menuconfig : small shell prompt to write config.mk
