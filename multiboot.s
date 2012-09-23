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

.section ".multiboot", "ax"

#
# The kernel is loaded at physical address 0x100000. Turn on paging and map it
# at virtual address 0xc0000000.
#
__mb_init:
  # set up page directory
  mov   $(PHYS_ADDR(pg_table)), %eax
  or    $3, %eax                        # read/write | present
  mov   $(PHYS_ADDR(pg_dir)), %ebx
  mov   $1024, %ecx
.L1:
  mov   %eax, (%ebx)
  add   $4096, %eax
  add   $4, %ebx
  dec   %ecx
  jnz   .L1

  # set up identity paging in the individual page tables, each table maps 4 MB
  mov   $3, %eax
  mov   $(PHYS_ADDR(pg_table)), %ebx
  mov   $(1024 * 1024), %ecx
.L2:
  mov   %eax, (%ebx)
  add   $4096, %eax
  add   $4, %ebx
  dec   %ecx
  jnz   .L2

  # map physical range 0x100000 to virtual range 0xc0000000
  mov   $(PHYS_BASE_ADDR | 3), %eax
  mov   $(PHYS_ADDR(pg_table) + VIRT_BASE_ADDR / 1024), %ebx
  mov   $(__stack_end - VIRT_BASE_ADDR), %ecx   # end of kernel
  shr   $12, %ecx
.L3:
  mov   %eax, (%ebx)
  add   $4096, %eax
  add   $4, %ebx
  dec   %ecx
  jnz   .L3

  # enable paging
  mov   $(PHYS_ADDR(pg_dir)), %eax
  mov   %eax, %cr3
  mov   %cr0, %eax
  or    $0x80000000, %eax
  mov   %eax, %cr0

  # reload GDT and jump to virtual address
  lgdt  PHYS_ADDR(gdt_addr)
  jmpl  $8, $.reload0

.reload0:
  mov   $16, %ax
  mov   %ax, %ds
  mov   %ax, %es
  mov   %ax, %fs
  mov   %ax, %gs
  mov   %ax, %ss
  mov   $__stack_end, %esp
  call kern_init

.hang:
  cli
  hlt
  jmp .hang

.align 4
.magic:
.long 0x1badb002
.long 0x10003                 # align | meminfo | address
.long -(0x1badb002 + 0x10003) # checksum
.long PHYS_ADDR(.magic)
.long PHYS_BASE_ADDR
.long PHYS_ADDR(__data_end)
.long PHYS_ADDR(__bss_end)
.long PHYS_ADDR(__mb_init)
