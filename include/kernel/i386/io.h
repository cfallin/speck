/*
 * include/kernel/i386/io.h
 */

#ifndef _KERNEL_I386_IO_H_
#define _KERNEL_I386_IO_H_

#define outb(val,port) {\
	asm volatile \
	( \
	  "outb %%al,%%dx" \
	  :: "d" ((unsigned short)port),"a" ((unsigned char)val) \
	);\
}

#define inb(port) ({\
	unsigned char retval; \
	asm volatile \
	( \
	  "inb %%dx,%%al" \
	  : "=a" (retval) : "d" ((unsigned short)port) \
	);\
	retval;\
})

#define outw(val,port) {\
	asm volatile \
	( \
	  "outw %%ax,%%dx" \
	  :: "d" ((unsigned short)port),"a" ((unsigned short)val) \
	);\
}

#define inw(port) ({\
	unsigned short retval; \
	asm volatile \
	( \
	  "inw %%dx,%%ax" \
	  : "=a" (retval) : "d" ((unsigned short)port) \
	);\
	retval;\
})

#define outl(val,port) {\
	asm volatile \
	( \
	  "outl %%eax,%%dx" \
	  :: "d" ((unsigned short)port),"a" ((unsigned long)val) \
	);\
}

#define inl(port) ({\
	unsigned long retval; \
	asm volatile \
	( \
	  "inl %%dx,%%eax" \
	  : "=a" (retval) : "d" ((unsigned short)port) \
	);\
	retval;\
})


#endif
