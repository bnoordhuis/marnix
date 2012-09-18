CFLAGS	= -Wall -Wextra -std=c99 -ffreestanding -nostartfiles -nostdlib -nodefaultlibs
ASFLAGS	=
LDFLAGS	= -s -static

ifeq ($(shell uname -m),x86_64)
CFLAGS	+= -m32
ASFLAGS += -32
LDFLAGS	+=
endif

CFLAGS	+= $(EXTRA_CFLAGS)
ASFLAGS += $(EXTRA_ASFLAGS)
LDFLAGS	+= $(EXTRA_LDFLAGS)

.PHONY:	all
all:	multiboot.img

.PHONY:	clean
clean:
	rm -f *.o *.img

multiboot.img:	multiboot.ld kern.o
	$(LD) $(LDFLAGS) -o $@ -T $^

.s.o:
	$(AS) $(ASFLAGS) -o $@ $<
