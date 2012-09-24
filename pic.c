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

// ports 0x20 and 0x21 are the master's command and data ports
// ports 0xa0 and 0xa1 the slave's

static void remap_pic(void)
{
  unsigned char mask[2];

  mask[0] = inb(0x21);
  mask[1] = inb(0xa1);

  outb(0x20, 0x11);
  outb(0xa0, 0x11);
  io_wait();

  outb(0x21, 0x20); // map to interrupt range 0x20-0x27
  outb(0xa1, 0x28); // map to interrupt range 0x28-0x2f
  io_wait();

  outb(0x21, 4);    // have slave at IRQ2
  outb(0xa1, 2);    // ping master through IRQ2
  io_wait();

  outb(0x21, 1);    // 8086 mode
  outb(0xa1, 1);    // ditto
  io_wait();

  outb(0x21, mask[0]);
  outb(0xa1, mask[1]);
  io_wait();
}

void pic_init(void)
{
  remap_pic();
}

static void irq0(struct regs r)
{
  (void) r;
  outb(0x20, 0x20); // ack IRQ
}

void pit_init(unsigned int freq)
{
  unsigned int divisor = 1193180 / freq;

  set_interrupt_handler(0x20, irq0);

  outb(0x43, 0x36); // repeat mode
  io_wait();

  outb(0x40, divisor & 255);
  io_wait();

  outb(0x40, divisor >> 8);
  io_wait();
}
