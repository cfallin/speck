# kmod/make.inc

.PHONY: all
all: $(KMOD).o

$(KMOD).o: $(OBJS)
	$(LD) -T $(srcroot)/kmod/$(arch).ld $(OBJS) -r -o $(KMOD).o
	cp $(KMOD).o $(srcroot)/bin/$(arch)/sys/kernel/$(KMOD).o

.PHONY: setup
setup:

.PHONY: clean
clean:
	rm -f *.o *~
