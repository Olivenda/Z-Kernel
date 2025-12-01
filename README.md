Z-Kernel: Minimal 64-bit kernel
================================

This project demonstrates a minimal 64-bit kernel (not Linux) that prints a "Hello World" style
message to VGA text memory. A small Kconfig-inspired frontend (with ncurses `mconf`) powers
`make defconfig` and `make menuconfig` targets so you can tweak the build without touching the
Makefile manually.

Quick start (prerequisites):
- A cross-compiler toolchain (recommended): x86_64-elf-gcc, x86_64-elf-ld, objcopy
  On Debian/Ubuntu: apt install gcc-multilib nasm grub-pc-bin grub-common xorriso qemu-system-x86
  Alternatively build/install a cross-compiler (recommended for real kernel dev).
- grub-mkrescue to create the ISO
- qemu-system-x86_64 to run
- ncurses development headers (for building the bundled `mconf` helper)

Build and run:
1) Configure
   $ make defconfig     # write defaults to .config + generated headers
   $ make menuconfig    # interactively change settings from Kconfig (curses UI)

2) Build and create ISO:
   $ make

3) Run in QEMU:
   $ make run

Files of interest:
- src/boot.S   : multiboot2 header + minimal long-mode switch (assembly)
- src/kernel.c : minimal C kernel that writes to VGA memory
- src/drivers/ : placeholder for future drivers to keep hardware logic organized
- link.ld      : linker script
- Makefile     : build system and ISO creation
- scripts/kconfig/* : tiny Kconfig parser + `conf`/`mconf` style helpers
