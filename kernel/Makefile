# kernel/Makefile

srcroot = ..
include ../make.inc

OBJS = main.o mm.o process.o malloc.o bget.o level.o sem.o mutex.o msg.o irqmsg.o kmod.o exports.o

.PHONY: all
all: kernel.o $(arch)/kernel

.PHONY: $(arch)/kernel
$(arch)/kernel: kernel.o
	$(MAKE) -C $(arch)

kernel.o: $(OBJS)
	$(LD) -r -o $@ $^

.PHONY: setup
setup:
	$(MAKE) -C $(arch) setup
	rm -f ../include/kernel/arch
	ln -s $(arch) ../include/kernel/arch
	rm -f main.o # force rebuild, as this file contains the build time

.PHONY: clean
clean:
	$(MAKE) -C $(arch) clean
	rm -f *.o *~ kernel
	rm -f ../include/kernel/arch
