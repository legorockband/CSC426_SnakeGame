# RISC-V baremetal init.s
# This code is executed first.

    .section .text.init, "ax", @progbits
    .globl entry
entry:
    /* Initialize stack pointer to symbol from link.ld */
    la      sp, __sp

    /* (Optional but safe) initialize gp to linker-defined global pointer */
    la      gp, __global_pointer$

    /* Jump to GCC crt0 start */
    j       _start
