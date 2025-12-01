/* Linker script for tiny 64-bit kernel */
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(start)
SECTIONS {
  . = 0x00100000; /* link at 1MB to avoid low-memory conflicts */
  .text : { *(.multiboot2_header) *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss  : { *(.bss*) }
}