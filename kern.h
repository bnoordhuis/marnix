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

#ifndef KERN_H_
#define KERN_H_

#include "config.h"

#define asm             __asm__
#define volatile        __volatile__

#define __align(n)      __attribute__((aligned(n)))
#define __always_inline __attribute__((always_inline))
#define __noreturn      __attribute__((noreturn))
#define __packed        __attribute__((packed))
#define __unused        __attribute__((unused))

#define ARRAY_SIZE(a)                                                         \
  (sizeof(a) / sizeof((a)[0]))

#define STATIC_ASSERT(expr)                                                   \
  typedef char __static_assert[!-(expr)]

struct dt_addr
{
  unsigned short size;
  void *addr;
} __packed;

struct regs
{
  unsigned long es;
  unsigned long ds;
  unsigned long edi;
  unsigned long esi;
  unsigned long ebp;
  unsigned long esp;
  unsigned long ebx;
  unsigned long edx;
  unsigned long ecx;
  unsigned long eax;
  unsigned long num;
  unsigned long err;
};

typedef void (*interrupt_handler)(struct regs r);

/* kern.c */
__noreturn void panic(const char *errmsg, ...);
void puts(const char *s);

/* idt.c */
void idt_init(void);
void set_interrupt_handler(unsigned char num, interrupt_handler handler);

/* pic.c */
void pic_init(void);
void pit_init(unsigned int freq);

inline static unsigned char inb(unsigned short port)
{
  unsigned char val;
  asm volatile ("inb %1, %0" : "=a" (val) : "Nd" (port));
  return val;
}

inline static void outb(unsigned short port, unsigned char val)
{
  asm volatile ("outb %0, %1" : /* no output */ : "a" (val), "Nd" (port));
}

inline static void io_wait(void)
{
  outb(0x80, 0);
}

inline static unsigned long get_cr0(void)
{
  unsigned long val;
  asm volatile ("mov %%cr0, %0" : "=r" (val));
  return val;
}

inline static void set_cr0(unsigned long val)
{
  asm volatile ("mov %0, %%cr0" : /* no output */ : "r" (val));
}

inline static void set_cr3(unsigned long val)
{
  asm volatile ("mov %0, %%cr3" : /* no output */ : "r" (val));
}

#endif // KERN_H_
