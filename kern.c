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

__attribute__((packed))
struct descriptor
{
  unsigned short r0;
  unsigned short r1;
  unsigned short r2;
  unsigned short r3;
};

__attribute__((aligned(8)))
struct descriptor gdt[] =
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

__attribute__((aligned(8)))
struct descriptor idt[256];

__attribute__((aligned(8)))
unsigned char idt_addr[6];

asm (
  ".globl mb_init;"
  "mb_init:"
  "mov $__kern_stack, %esp;"
  "push $0;"
  "popf;"
  "push %ebx;"
  "push %eax;"
  "jmp kern_init;"
);

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

static void store_addr(unsigned char dst[6], void *addr, unsigned short size)
{
  dst[0] = size >> 0;
  dst[1] = size >> 8;
  dst[2] = (unsigned long) addr >> 0;
  dst[3] = (unsigned long) addr >> 8;
  dst[4] = (unsigned long) addr >> 16;
  dst[5] = (unsigned long) addr >> 24;
}

static void load_gdt(void)
{
  store_addr((unsigned char *) &gdt, &gdt, sizeof(gdt) - 1);

  asm volatile (
    "lgdt gdt;"
    "jmpl $8, $.reload;"
    ".reload:"
  );
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

  store_addr(idt_addr, &idt, sizeof(idt) - 1);
  asm volatile ("lidt idt_addr");
}

__attribute__((aligned(4096)))
unsigned long page_dir[1024];

__attribute__((aligned(4096)))
unsigned long page_table[1024][1024];

static void enable_paging(void)
{
  unsigned int i, k;

  puts(__func__);

  for (i = 0; i < ARRAY_SIZE(page_dir); i++)
    page_dir[i] = (unsigned long) (page_table + i) | 3; // read/write | present

  // each page table maps 4 MB
  for (i = 0; i < ARRAY_SIZE(page_table); i++)
    for (k = 0; k < ARRAY_SIZE(page_table[0]); k++)
      page_table[i][k] = (i * 4194304 + k * 4096) | 3; // read/write | present

  set_cr3((unsigned long) &page_dir);
  set_cr0(0x80000000 | get_cr0());
}

__attribute__((noreturn))
void kern_init(void)
{
  load_gdt();
  load_idt();
  enable_paging();
  panic("Halting.");
}
