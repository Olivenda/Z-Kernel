/* Stivale2 entry point for Z-Kernel.
   The bootloader hands us a pointer to struct stivale2_struct in RDI.
*/

    .section .stivale2hdr,"a"
    .align 8
    .global stivale2_header

/* Request a simple framebuffer; fall back gracefully to VGA text if unavailable. */
#define STIVALE2_HEADER_TAG_FRAMEBUFFER 0x3ecc1bc43d0f7971

stivale2_header:
    .quad 0                  /* entry point: use ELF entry (_start) */
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
