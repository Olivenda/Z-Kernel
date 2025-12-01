/* Multiboot1 + Stivale2 entry points for Z-Kernel. */

    .code32

    /*
     * Minimal Multiboot1 header so QEMU/GRUB recognize the image instead of
     * trying PVH. Keep this section first in the binary so it sits within the
     * first 8 KiB as required by the spec.
     */
    .section .multiboot,"a"
    .align 4
    .globl multiboot_header

multiboot_header:
    .long 0x1BADB002          /* magic */
    .long 0x00000000          /* flags */
    .long 0xE4524FFE          /* checksum = -(magic + flags) */

    .globl mb_entry

mb_entry:
    cli
    mov $stack_top, %esp

    /* Load a 64-bit-capable GDT. */
    lgdt gdt64_descriptor

    /* Enable PAE. */
    mov %cr4, %eax
    or  $0x20, %eax
    mov %eax, %cr4

    /* Point CR3 at our identity-mapped page tables. */
    mov $pml4_table, %eax
    mov %eax, %cr3

    /* Turn on long mode via EFER.LME. */
    mov $0xC0000080, %ecx
    rdmsr
    or  $0x00000100, %eax
    wrmsr

    /* Enable paging (and keep PE set) now that long mode is armed. */
    mov %cr0, %eax
    or  $0x80000001, %eax
    mov %eax, %cr0

    /* Long jump into 64-bit mode. */
    ljmp $0x08, $long_mode_entry

    .code64
long_mode_entry:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss

    movabs $stack_top, %rsp
    xor %rdi, %rdi            /* no Stivale2 info when booted via Multiboot */
    jmp _start

    .code64

/* Stivale2 entry point for Z-Kernel.
   The bootloader hands us a pointer to struct stivale2_struct in RDI.
*/

    .section .stivale2hdr,"a"
    .align 8
    .global stivale2_header

/* Request a simple framebuffer; fall back gracefully to VGA text if unavailable. */
#define STIVALE2_HEADER_TAG_FRAMEBUFFER 0x3ecc1bc43d0f7971

stivale2_header:
    .quad _start             /* entry point for Stivale2 bootloaders */
    .quad stack_top          /* stack */
    .quad 0                  /* flags */
    .quad stivale2_fb_tag    /* first header tag */

stivale2_fb_tag:
    .quad STIVALE2_HEADER_TAG_FRAMEBUFFER
    .quad 0                  /* no further tags */
    .quad 1024               /* preferred width */
    .quad 768                /* preferred height */
    .quad 32                 /* preferred bpp */

    .section .text
    .global _start
    .extern kernel_main

_start:
    /* RDI already holds stivale2_struct* per ABI */
    call kernel_main

.hang:
    hlt
    jmp .hang

    .bss
    .align 16
stack_bottom:
    .skip 16384
stack_top:

    .section .data
    /* Paging structures for the Multiboot path (identity map first 2 MiB). */
    .align 4096
pml4_table:
    .quad pdpt_table + 0x3

    .align 4096
pdpt_table:
    .quad pd_table + 0x3
    .space 8 * 511, 0

    .align 4096
pd_table:
    /* 2 MiB page: present | writable | huge page */
    .quad 0x0000000000000083
    .space 8 * 511, 0

    .align 16
gdt64:
    .quad 0x0000000000000000
    .quad 0x00AF9A000000FFFF /* code */
    .quad 0x00AF92000000FFFF /* data */
gdt64_descriptor:
    .word gdt64_descriptor - gdt64 - 1
    .long gdt64
