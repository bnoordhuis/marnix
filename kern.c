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

struct dt_addr
{
  unsigned short size;
  void *addr;
} __packed;

__align(8) unsigned short gdt[] =
{
  DESCRIPTOR(0, 0x00000, 0x0000), // null selector
  DESCRIPTOR(0, 0xfffff, 0x9ac0), // kernel CS
  DESCRIPTOR(0, 0xfffff, 0x92c0), // kernel DS
  DESCRIPTOR(0, 0xfffff, 0xfac0), // user CS
  DESCRIPTOR(0, 0xfffff, 0xf2c0), // user DS
};

__align(8) unsigned short idt[256][4];

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

static void set_idt_sel(int num, unsigned char flags, void (*handler)(void))
{
  idt[num][0] = (unsigned long) handler >> 0;
  idt[num][1] = 8;           // kernel CS selector
  idt[num][2] = flags << 8;  // lower 8 bits must be zero
  idt[num][3] = (unsigned long) handler >> 16;
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
  unsigned int i;

#define SYSTEM_INTERRUPT(num)                                                 \
  /* present=1 | dpl=0 | type=intr_gate */                                    \
  set_idt_sel((num), 0x8e, sys_intr_##num);
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

  set_idt_sel(0x0d, 0x8e, general_protection_fault);

  for (i = 32; i < ARRAY_SIZE(idt); i++)
    // present=1 | dpl=3 | type=trap_gate
    set_idt_sel(i, 0xbf, ignore_spurious_interrupt);

  asm volatile ("lidt idt_addr");
}

__noreturn void kern_init(void)
{
  pic_init();
  load_idt();
  panic("Halting.");
}
