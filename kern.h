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

#define KB 1024
#define MB 1048576
#define GB 1073741824

#define ARRAY_SIZE(a)                                                         \
  (sizeof(a) / sizeof((a)[0]))

#define __STATIC_ASSERT0(expr, where)                                         \
  enum { __static_assert_ ## where = 1 / !!(expr) }

#define __STATIC_ASSERT1(expr, where)                                         \
  __STATIC_ASSERT0(expr, where)

#define STATIC_ASSERT(expr)                                                   \
  __STATIC_ASSERT1(expr, __LINE__)

typedef signed char         s8;
typedef signed short        s16;
typedef signed long         s32;
typedef signed long long    s64;

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned long       u32;
typedef unsigned long long  u64;

enum loglevel
{
  FATAL,
  ERROR,
  WARN,
  INFO,
  DEBUG
};

struct dt_addr
{
  u16 size;
  void *addr;
} __packed;

struct regs
{
  u32 es;
  u32 ds;
  u32 edi;
  u32 esi;
  u32 ebp;
  u32 esp;
  u32 ebx;
  u32 edx;
  u32 ecx;
  u32 eax;
  u32 num;
  u32 err;
  u32 eip;
  u32 cs;
  u32 eflags;
};

typedef void (*interrupt_handler)(struct regs r);

/* kern.c */
__noreturn void panic(const char *errmsg, ...);
void kprintf(enum loglevel level, const char *fmt, ...);
void puts(const char *s);

/* idt.c */
void idt_init(void);
void set_interrupt_handler(u8 num, interrupt_handler handler);

/* pic.c */
void pic_init(void);
void pit_init(unsigned int freq);

/* serial.c */
void serial_init(void);
int getc(void);
void putc(int c);

inline static u8 inb(u16 port)
{
  u8 val;
  asm volatile ("inb %1, %0" : "=a" (val) : "Nd" (port));
  return val;
}

inline static void outb(u16 port, u8 val)
{
  asm volatile ("outb %0, %1" : /* no output */ : "a" (val), "Nd" (port));
}

inline static void io_wait(void)
{
  outb(0x80, 0);
}

inline static u32 get_cr0(void)
{
  u32 val;
  asm volatile ("mov %%cr0, %0" : "=r" (val));
  return val;
}

inline static void set_cr0(u32 val)
{
  asm volatile ("mov %0, %%cr0" : /* no output */ : "r" (val));
}

inline static void set_cr3(u32 val)
{
  asm volatile ("mov %0, %%cr3" : /* no output */ : "r" (val));
}

#endif // KERN_H_
