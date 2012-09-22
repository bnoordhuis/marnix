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

struct dt_addr
{
  unsigned short size;
  void *addr;
} __packed;

struct descriptor
{
  unsigned short r0;
  unsigned short r1;
  unsigned short r2;
  unsigned short r3;
} __packed;

__align(8) struct descriptor gdt[] =
{
  // null descriptor
  {
    .r0 = 0,
    .r1 = 0,
    .r2 = 0,
    .r3 = 0,
  },

  // 4 GB execute/read code segment, nonconforming
  {
    .r0 = 0xFFFF, // limit
    .r1 = 0x0000, // base
    .r2 = 0x9A00, // type | non-system | present
    .r3 = 0x00CF, // limit | 32 bits | granularity
  },

  // 4 GB read/write data segment
  {
    .r0 = 0xFFFF, // limit
    .r1 = 0x0000, // base
    .r2 = 0x9200, // type | non-system | present
    .r3 = 0x00CF, // limit | 32 bits | granularity
  },
};

__align(8) struct descriptor idt[256];

__align(8) struct dt_addr gdt_addr =
{
  .size = sizeof(gdt) - 1,
  .addr = PHYS_ADDR((char *) &gdt),
};

__align(8) struct dt_addr idt_addr =
{
  .size = sizeof(idt) - 1,
  .addr = PHYS_ADDR((char *) &idt),
};

__align(4096) unsigned long pg_dir[1024];
__align(4096) unsigned long pg_table[1024][1024];

STATIC_ASSERT(sizeof(pg_dir) == 4096);
STATIC_ASSERT(sizeof(pg_table) == 4 * 1024 * 1024);

static void putc(int c)
{
  outb(0xE9, c);
}

static void puts(const char *s)
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

#define INTERRUPT_HANDLER(name)                                               \
  __asm__ (                                                                   \
    ".global __" #name ";"                                                    \
    "__" #name ":"                                                            \
    "pusha;"                                                                  \
    "call " #name ";"                                                         \
    "popa;"                                                                   \
  );                                                                          \
  void __##name(void);                                                        \
  void name(void)

#define SET_INTERRUPT_HANDLER(d, h)                                           \
  {                                                                           \
    (d)->r0 = (unsigned long) (__ ##h) >> 0;                                  \
    (d)->r3 = (unsigned long) (__ ##h) >> 16;                                 \
  }

#define SYSTEM_INTERRUPT(num)                                                 \
  INTERRUPT_HANDLER(sys_intr_##num)                                           \
  {                                                                           \
    puts("Unexpected interrupt " #num ".");                                   \
  }
  SYSTEM_INTERRUPT(0x00)
  SYSTEM_INTERRUPT(0x01)
  SYSTEM_INTERRUPT(0x02)
  SYSTEM_INTERRUPT(0x03)
  SYSTEM_INTERRUPT(0x04)
  SYSTEM_INTERRUPT(0x05)
  SYSTEM_INTERRUPT(0x06)
  SYSTEM_INTERRUPT(0x07)
  SYSTEM_INTERRUPT(0x08)
  SYSTEM_INTERRUPT(0x09)
  SYSTEM_INTERRUPT(0x0A)
  SYSTEM_INTERRUPT(0x0B)
  SYSTEM_INTERRUPT(0x0C)
  SYSTEM_INTERRUPT(0x0D)
  SYSTEM_INTERRUPT(0x0E)
  SYSTEM_INTERRUPT(0x0F)
  SYSTEM_INTERRUPT(0x10)
  SYSTEM_INTERRUPT(0x11)
  SYSTEM_INTERRUPT(0x12)
  SYSTEM_INTERRUPT(0x13)
  SYSTEM_INTERRUPT(0x14)
  SYSTEM_INTERRUPT(0x15)
  SYSTEM_INTERRUPT(0x16)
  SYSTEM_INTERRUPT(0x17)
  SYSTEM_INTERRUPT(0x18)
  SYSTEM_INTERRUPT(0x19)
  SYSTEM_INTERRUPT(0x1A)
  SYSTEM_INTERRUPT(0x1B)
  SYSTEM_INTERRUPT(0x1C)
  SYSTEM_INTERRUPT(0x1D)
  SYSTEM_INTERRUPT(0x1E)
  SYSTEM_INTERRUPT(0x1F)
#undef SYSTEM_INTERRUPT

INTERRUPT_HANDLER(ignore_spurious_interrupt)
{
  puts(__func__);
}

INTERRUPT_HANDLER(general_protection_fault)
{
  puts(__func__);
  halt();
}

INTERRUPT_HANDLER(syscall_entry)
{
  puts(__func__);
}

static void load_idt(void)
{
  struct descriptor *d;
  unsigned int i;

  for (i = 0; i < 32; i++) {
    d = idt + i;
    d->r1 = 8;       // kernel CS selector
    d->r2 = 0x8E00;  // present=1 | dpl=0 | type=intr_gate
    SET_INTERRUPT_HANDLER(d, ignore_spurious_interrupt);
  }

#define SYSTEM_INTERRUPT(num)                                                 \
  SET_INTERRUPT_HANDLER(idt + num, sys_intr_ ##num);
  SYSTEM_INTERRUPT(0x00)
  SYSTEM_INTERRUPT(0x01)
  SYSTEM_INTERRUPT(0x02)
  SYSTEM_INTERRUPT(0x03)
  SYSTEM_INTERRUPT(0x04)
  SYSTEM_INTERRUPT(0x05)
  SYSTEM_INTERRUPT(0x06)
  SYSTEM_INTERRUPT(0x07)
  SYSTEM_INTERRUPT(0x08)
  SYSTEM_INTERRUPT(0x09)
  SYSTEM_INTERRUPT(0x0A)
  SYSTEM_INTERRUPT(0x0B)
  SYSTEM_INTERRUPT(0x0C)
  SYSTEM_INTERRUPT(0x0D)
  SYSTEM_INTERRUPT(0x0E)
  SYSTEM_INTERRUPT(0x0F)
  SYSTEM_INTERRUPT(0x10)
  SYSTEM_INTERRUPT(0x11)
  SYSTEM_INTERRUPT(0x12)
  SYSTEM_INTERRUPT(0x13)
  SYSTEM_INTERRUPT(0x14)
  SYSTEM_INTERRUPT(0x15)
  SYSTEM_INTERRUPT(0x16)
  SYSTEM_INTERRUPT(0x17)
  SYSTEM_INTERRUPT(0x18)
  SYSTEM_INTERRUPT(0x19)
  SYSTEM_INTERRUPT(0x1A)
  SYSTEM_INTERRUPT(0x1B)
  SYSTEM_INTERRUPT(0x1C)
  SYSTEM_INTERRUPT(0x1D)
  SYSTEM_INTERRUPT(0x1E)
  SYSTEM_INTERRUPT(0x1F)
#undef SYSTEM_INTERRUPT

  for (i = 32; i < ARRAY_SIZE(idt); i++) {
    d = idt + i;
    d->r1 = 8;       // kernel CS selector
    d->r2 = 0xBF00;  // present=1 | dpl=3 | type=trap_gate
    SET_INTERRUPT_HANDLER(d, ignore_spurious_interrupt);
  }

  SET_INTERRUPT_HANDLER(idt + 0x0D, general_protection_fault);
  SET_INTERRUPT_HANDLER(idt + 0x80, syscall_entry);
  asm volatile ("lidt idt_addr");
}

__noreturn void kern_init(void)
{
  load_idt();
  panic("Halting.");
}
