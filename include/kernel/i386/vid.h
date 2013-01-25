/*
 * include/kernel/i386/vid.h
 */

#ifndef _KERNEL_I386_VID_H_
#define _KERNEL_I386_VID_H_

#define VID_ROWS 25
#define VID_COLS 80

extern void vid_init();

extern void vid_clear();

extern void vid_scrollup(int update);

extern void vid_putc(char  c);
extern void vid_puts(char *s);
extern void vid_puthex(int i);
extern void vid_putd(int i);

extern void vid_getpos(int *row, int *col);
extern void vid_setpos(int  row, int  col);

extern void vid_setattr(int  attr);
extern void vid_getattr(int *attr);

#define VID_BUFSIZE (VID_ROWS*VID_COLS*2)

extern void vid_scrtobuf(char *buf);
extern void vid_usebuf(char *buf);

#define VID_BLACK   0
#define VID_BLUE    1
#define VID_GREEN   2
#define VID_CYAN    3
#define VID_RED     4
#define VID_MAGENTA 5
#define VID_YELLOW  6
#define VID_WHITE   7

#define VID_BRIGHT  8

#define VID_FGBG(fg,bg) ((bg<<4) | fg)

#endif
