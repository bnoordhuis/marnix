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

#include "kern.h"
#include "klibc.h"

#define DESCRIPTOR(addr, size, type)                                          \
  ((size) & 0xffff),                                                          \
  ((addr) & 0xffff),                                                          \
  (((type) & 0xff00) | (((addr) >> 16) & 0xff)),                              \
  ((((addr) >> 16) & 0xff00) | ((type) & 0xf0) | ((size) >> 16))

__align(8) u16 gdt[] =
{
  DESCRIPTOR(0, 0x00000, 0x0000), // null selector
  DESCRIPTOR(0, 0xfffff, 0x9ac0), // kernel CS
  DESCRIPTOR(0, 0xfffff, 0x92c0), // kernel DS
  DESCRIPTOR(0, 0xfffff, 0xfac0), // user CS
  DESCRIPTOR(0, 0xfffff, 0xf2c0), // user DS
};

__align(8) struct dt_addr gdt_addr =
{
  .size = sizeof(gdt) - 1,
  .addr = PHYS_ADDR((char *) &gdt),
};

__align(4096) u32 pg_dir[1024];
__align(4096) u32 pg_table[1024][1024];

STATIC_ASSERT(sizeof(pg_dir) == 4096);
STATIC_ASSERT(sizeof(pg_table) == 4 * 1024 * 1024);

static void putc(int c)
{
  outb(0xE9, c);
}

void puts(const char *s)
{
  while (*s) putc(*s++);
  putc('\n');
}

__noreturn static void halt(void)
{
  for (;;)
    asm volatile ("cli; hlt");
}

__noreturn void panic(const char *errmsg, ...)
{
  static const char prefix[] = "PANIC: ";
  const int prefix_len = sizeof(prefix) - 1;
  char buf[256]; // can't be too large, we don't know what stack we're on
  va_list ap;

  memcpy(buf, prefix, prefix_len);
  va_start(ap, errmsg);
  vsnprintf(buf + prefix_len, sizeof(buf) - prefix_len, errmsg, ap);
  va_end(ap);
  puts(buf);
  halt();
}

__noreturn void kern_init(void)
{
  pic_init();
  idt_init();
  pit_init(20); // 20 Hz

  for (;;) asm volatile ("sti; hlt");
}
