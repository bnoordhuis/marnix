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

struct bootinfo
{
  u32 flags;
  u32 mem_lower;
  u32 mem_upper;
  u32 boot_device;
  u32 cmdline;
  u32 mods_count;
  u32 mods_addr;
  u32 syms[4];
  u32 mmap_length;
  u32 mmap_addr;
  u32 drives_length;
  u32 drives_addr;
  u32 config_table;
  u32 boot_loader_name;
  u32 apm_table;
  u32 vbe_control_info;
  u32 vbe_mode_info;
  u16 vbe_mode;
  u16 vbe_interface_seg;
  u16 vbe_interface_off;
  u16 vbe_interface_len;
} __packed;

STATIC_ASSERT(sizeof(struct bootinfo) == 88);

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

void put(const char *s)
{
  while (*s) putc(*s++);
}

void puts(const char *s)
{
  put(s);
  putc('\n');
}

void kprintf(enum loglevel level, const char *fmt, ...)
{
  char buf[256]; // can't be too large, we don't know what stack we're on
  va_list ap;

  static const char levels[][9] = {
    "[fatal] ",
    "[error] ",
    "[warn ] ",
    "[info ] ",
    "[debug] ",
  };

  if (level >= ARRAY_SIZE(levels))
    level = ERROR;

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  put(levels[level]);
  puts(buf);
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

static void parse_initrd(struct bootinfo *info)
{
  struct initrd
  {
    u32 mod_start;
    u32 mod_end;
    u32 cmdline;
    u32 reserved;
  } __packed;

  unsigned int i, n;
  struct initrd *p;

  n = info->mods_count;

  if (n == 0)
    kprintf(INFO, "initrd: no ramdisks found");

  if (n == 0)
    return;

  p = (void *) info->mods_addr;

  for (i = 0; i < n; i++) {
    kprintf(DEBUG,
            "initrd: mod_start=%x, mod_end=%x, cmdline=%s, data=%s",
            p->mod_start,
            p->mod_end,
            (const char *) p->cmdline);
  }
}

static void parse_debugsyms(struct bootinfo *info)
{
  if (info->syms[0] == 0)
    kprintf(DEBUG, "debugsyms: no symbol table found");
}

static void parse_bootinfo(u32 magic, struct bootinfo *info)
{
  if (magic != 0x2badb002)
    panic("bad multiboot magic: %x", magic);

  kprintf(DEBUG, "bootinfo: located at %p physical", info);
  kprintf(DEBUG, "bootinfo: flags=%x", info->flags);
  kprintf(DEBUG, "bootinfo: mem_lower=%d", info->mem_lower);
  kprintf(DEBUG, "bootinfo: mem_upper=%d", info->mem_upper);
  kprintf(DEBUG, "bootinfo: mods_addr=%x", info->mods_addr);
  kprintf(DEBUG, "bootinfo: mods_count=%x", info->mods_count);
  kprintf(DEBUG, "bootinfo: mmap_addr=%x", info->mmap_addr);
  kprintf(DEBUG, "bootinfo: mmap_length=%x", info->mmap_length);

  if (info->flags & 4)
    kprintf(INFO, "bootinfo: cmdline=%s\n", (const char *) info->cmdline);

  if (info->flags & 8)
    parse_initrd(info);

  if (info->flags & 32)
    parse_debugsyms(info);
}

__noreturn void kern_init(u32 magic, struct bootinfo *info)
{
  parse_bootinfo(magic, info);
  serial_init();
  pic_init();
  idt_init();
  pit_init(20); // 20 Hz

  kprintf(INFO, "system shutdown");
  for (;;) asm volatile ("sti; hlt");
}
