# MARNIX

This is MARNIX, a UNIX clone.

## BUILD INSTRUCTIONS

Easy as pie but make sure that you're on a GNU/Linux system:

    make clean all

## HOW TO RUN

I would recommend against running MARNIX on real hardware for now. :-)

Install [qemu](https://github.com/qemu/qemu) and run it like this:

    qemu-system-i386 -m 16 -no-reboot -kernel multiboot.img -display curses -nographic -chardev stdio,mux=on,id=char0 -mon chardev=char0,mode=readline -serial chardev:char0

## KNOWN BUGS

Too numerous to list. It's a work in progress.
