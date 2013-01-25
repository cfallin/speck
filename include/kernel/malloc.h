/*
 * include/kernel/malloc.h
 */

#ifndef _KERNEL_MALLOC_H_
#define _KERNEL_MALLOC_H_

void malloc_init();

void *malloc(int size);
void  free(void *p);

// internal callbacks - registered with bget
void *malloc_bget_acquire_callback(long size);
void  malloc_bget_release_callback(void *buf);

#endif
