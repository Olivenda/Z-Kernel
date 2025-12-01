Z-Kernel: Minimal 64-bit kernel
================================

Z-Kernel is a tiny 64-bit hobby kernel (not Linux) that drops into long mode, writes a
"Hello World"-style message directly to VGA text memory, and boots via Multiboot 2.
It is intentionally small to keep the focus on the toolchain and boot flow rather than
kernel features.

Prerequisites
-------------
You need the usual bare-metal toolchain components:

- **Cross-compiler**: `x86_64-elf-gcc`, `x86_64-elf-ld`, `objcopy`
  - Debian/Ubuntu example: `apt install gcc-multilib nasm grub-pc-bin grub-common xorriso qemu-system-x86`
  - For serious kernel work, building a dedicated cross-compiler is recommended.
- **ISO builder**: `grub-mkrescue`
- **Emulator**: `qemu-system-x86_64`

Configuring
-----------
Run `make menuconfig` (optional) to generate `config.mk`. Available toggles:

- `ENABLE_HELLO`: display the hello message when the kernel starts.
- `LANGUAGE`: choose `en` or `de` for the greeting.
- `USE_GRUB`: enable GRUB-specific boot flow.

Building
--------

```sh
# Create or update config.mk (optional)
make menuconfig

# Build the kernel and generate an ISO at iso/z-kernel.iso
make
```

Running
-------

```sh
# Launch the ISO in QEMU
make run
```

Key files
---------

- `src/boot.S`: Multiboot 2 header and minimal long-mode switch (assembly).
- `src/kernel.c`: Minimal C kernel that writes to VGA memory.
- `link.ld`: Linker script controlling kernel layout.
- `Makefile`: Build system, ISO creation, and `menuconfig` wiring.
- `scripts/menuconfig`: Small shell prompt that writes `config.mk`.
