# bin/Makefile

srcroot = ..
include ../make.inc

BINARIES = sys/kernel/kernel sys/kernel/testmod.o
export BINARIES # pass to sub-make in $(arch)

.PHONY: all
all: $(arch)

.PHONY: $(arch)
$(arch):
	$(MAKE) -C $(arch)

.PHONY: setup
setup:
	$(MAKE) -C $(arch) setup

.PHONY: clean
clean:
	$(MAKE) -C $(arch) clean
