/*
 * include/kernel/i386/string.h
 */

#ifndef _KERNEL_I386_STRING_H_
#define _KERNEL_I386_STRING_H_

typedef unsigned int size_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

extern void  *memcpy (void *to, const void *from, size_t size);
extern void  *memset (void *block, int c, size_t size);
extern int    strcmp (const char *s1, const char *s2);
extern int    strncmp(const char *s1, const char *s2, size_t n);
extern char  *strcpy (char *dst, const char *src);
extern char  *strncpy(char *dst, const char *src, size_t n);
extern size_t strlen (const char *str);

#endif
