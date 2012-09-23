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

#include "config.h"

OUTPUT_FORMAT (elf32-i386)
OUTPUT_ARCH (i386)

SECTIONS
{
  . = SIZEOF_HEADERS + VIRT_BASE_ADDR;

  .text ALIGN (16) :
  {
    *(.multiboot)
    __text_start = .;
    *(.text)
    __text_end = .;
  }

  .rodata ALIGN (4096) :
  {
    __rodata_start = .;
    *(.rodata*)
    __rodata_end = .;
  }

  .data :
  {
    __data_start = .;
    *(.data*)
    __data_end = .;
  }

  .bss :
  {
    __bss_start = .;
    *(COMMON)
    *(.bss*)
    __bss_end = .;

    . = ALIGN (4096);

    __stack_start = .;
    . += 32768;
    __stack_end = .;
  }
}
