/*
 * The stack starts at the top of memory, defined by __sp, and grows down.
 * Unfortunately this means we must hardcode the size of memory.
 * Here we choose 64K. Change this value to change the working memory size.
 */
/* 64 KiB RAM, stack at top */
__sp = 0x10000;

SECTIONS
{
  . = 0x00000000;

  .text.init : { *(.text.init) }
  .text      : { *(.text*) }
  .rodata    : { *(.rodata*) *(.srodata*) }

  /* gp for small-data */
  __global_pointer$ = . + 0x800;

  .data : {
    *(.sdata*)
    *(.data*)
  }
  _edata = .; PROVIDE(edata = .);

  .bss (NOLOAD) : {
    __bss_start = .;
    *(.sbss*)
    *(.bss*)
    *(COMMON)
    __bss_end = .;
  }

  _end = .; PROVIDE(end = .);
}
