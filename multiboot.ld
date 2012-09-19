/*
 * Copyright (c) 2012, Ben Noordhuis <info@bnoordhuis.nl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

OUTPUT_FORMAT (elf32-i386)
OUTPUT_ARCH (i386)

SECTIONS
{
  . = SIZEOF_HEADERS;

  .multiboot ALIGN(16) :
  {
    LONG(0x1BADB002);               /* multiboot magic */
    LONG(0x10003);                  /* align | meminfo | address */
    LONG(-(0x1BADB002 + 0x10003));  /* checksum */

    /* address */
    LONG(ADDR(.multiboot));
    LONG(ADDR(.text));
    LONG(ADDR(.data) + SIZEOF(.data));
    LONG(ADDR(.bss) + SIZEOF(.bss));
    LONG(mb_init);
  }

  .text 0x100000 :
  {
    *(.text)
  }

  .rodata ALIGN (4096) :
  {
    *(.rodata*)
  }

  .data :
  {
    *(.data*)
  }

  .bss :
  {
    *(COMMON)
    *(.bss*)

    . = ALIGN(4096);
    __kern_stack = . + 32768;
  }
}
