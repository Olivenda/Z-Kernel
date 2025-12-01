/* Minimal multiboot2 header + long mode switch
   This file contains a small multiboot2 header so GRUB recognizes the kernel as multiboot2.
   After GRUB loads us, we run small code to enable long mode (paging + PAE + LME) and
   jump to the C kernel_main entry point.

   This is written in GAS (AT&T) syntax.
*/

    .section .multiboot2_header
    .align 8
    /* multiboot2 header: magic, architecture, header_length, checksum */
    .long 0xE85250D6
    .long 0x00000000
    .long header_end - .
    .long -(0xE85250D6 + 0 + (header_end - .))

    /* end tag */
    .long 0
    .long 0

header_end:

    .section .text
    .global start
    .extern kernel_main

start:
    /* We assume GRUB loads us in 32-bit protected mode with a flat identity mapping. */
    /* Build minimal identity mapped page tables for long mode. */

    /* Zero registers */
    xor %rax, %rax

    /* Setup page tables in a small static area just below 1MB to be safe */
    lea 1f(%rip), %rbx    /* RBX -> page tables area */

    /* PML4 (1 entry) */
    movq %rbx, (%rbx)         /* PML4[0] -> address of PDPT (rbx) + 0x100 */
    addq $0x100, %rbx

    /* PDPT (1 entry) */
    movq %rbx, (%rbx)         /* PDPT[0] -> address of PD (rbx) + 0x100 */
    addq $0x100, %rbx

    /* PD (1 entry) map 1GB using 2MB pages would be simpler but we'll map using 2MB pages if supported. */
    /* Create a single 2MB page mapping for the first 2MB (that's enough for our kernel). */
    movq $0x0000000000000003, (%rbx) /* present + writable - will patch with address below */

    /* Patch PD entry with physical addr 0x0 (identity) | flags */
    movq $0, %rax
    movq %rax, 8(%rbx) /* ensure clearing next entries */

    /* Now enable PAE and Long Mode */
    /* Enable PAE: set CR4.PAE (bit 5) */
    mov %cr4, %rax
    or $0x20, %rax
    mov %rax, %cr4

    /* Load PML4 physical address into CR3 */
    lea 1f(%rip), %rax
    mov %rax, %cr3

    /* Enable syscall/sysret (not needed) */

    /* Enable long mode by setting EFER.LME (MSR 0xC0000080) */
    mov $0xC0000080, %ecx
    rdmsr
    or $0x00000100, %eax
    wrmsr

    /* Enable paging (set CR0.PG and CR0.PE bits) */
    mov %cr0, %rax
    or $0x80000000, %rax
    mov %rax, %cr0

    /* Far jump to 64-bit label */
    jmp $0x08, $1f

    .align 4096
1:
    /* 64-bit code continues here */
    /* Call C kernel_main */
    extern kernel_main
    call kernel_main
    /* Halt if returned */
1_loop:
    hlt
    jmp 1_loop

    .data
    .balign 4096
    .space 8192
