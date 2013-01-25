/*
 * kernel/i386/vid.c
 */

#include <kernel/i386/vid.h>
#include <kernel/i386/string.h>
#include <kernel/i386/io.h>
#include <kernel/i386/lock.h>
#include <kernel/i386/int.h>
#include <kernel/level.h>

//#define VID_SERIAL_DEBUG

#define VIDMEM ((char *)0xb8000)

int vid_curattr = 7, vid_row = 0, vid_col = 0;

char *vid_buf = (char *)0xb8000;

int vid_lock;

#if 1
#define VID_LOCK_ACQUIRE \
	level = level_go(LEVEL_NOPREEMPT); \
	spinlock_grab(&vid_lock);

#define VID_LOCK_RELEASE \
	spinlock_release(&vid_lock); \
	level_return(level);
#else
#define VID_LOCK_ACQUIRE
#define VID_LOCK_RELEASE
#endif

void vid_init()
{
}


void vid_update_vidmem()
{
	if(vid_buf != VIDMEM)
	{
		memcpy(VIDMEM,vid_buf,VID_BUFSIZE);
	}
}

void vid_scrtobuf(char *buf)
{
	memcpy(buf,VIDMEM,VID_BUFSIZE);
}

void vid_usebuf(char *buf)
{
	vid_buf = buf;
	vid_update_vidmem();
}

void vid_clear()
{
	int i;
	int level;

#ifdef VID_SERIAL_DEBUG
	// initialize the serial port (for debugging)
	outb(0,0x3f9);
	outb(0x80,0x3fb);
	outb(0x01,0x3f8);
	outb(0x00,0x3f9);
	outb(0x03,0x3fb);
	outb(0xc7,0x3fa);
	outb(0x0b,0x3fc);
#endif

	VID_LOCK_ACQUIRE;

	// fill vidmem with spaces with the current attributes
	for(i=0;i<VID_ROWS*VID_COLS*2;)
	{
		vid_buf[i] = ' ';
		i++;
		vid_buf[i] = vid_curattr;
		i++;
	}
	vid_update_vidmem();

	VID_LOCK_RELEASE;

	// go back to top left corner
	vid_setpos(0,0);
}

void vid_scrollup(int update)
{
	int i,j;

	for(i=0;i<VID_ROWS-1;i++)
	{
		// copy line up one place
		for(j=0;j<VID_COLS*2;j++)
		{
			vid_buf[(i*VID_COLS*2)+j] = vid_buf[((i+1)*VID_COLS*2)+j];
		}
	}

	// fill bottom line with spaces
	for(i=(VID_ROWS-1)*VID_COLS*2;i<VID_ROWS*VID_COLS*2;)
	{
		vid_buf[i] = ' ';
		i++;
		vid_buf[i] = vid_curattr;
		i++;
	}
	// if this isn't being used internally then commit to vram
	if(update)
		vid_update_vidmem();
}

void vid_setpos_internal(int row, int col)
{
	int pos = row*VID_COLS+col;

	// save position
	vid_row = row;
	vid_col = col;

	// write cursor position to CRTC
	outb(0x0e,0x3d4);
	outb((pos>>8)&0xff, 0x3d5);
	outb(0x0f,0x3d4);
	outb(pos & 0xff, 0x3d5);
}

void vid_setpos(int row, int col)
{
	int level;

	VID_LOCK_ACQUIRE;

	vid_setpos_internal(row,col);

	VID_LOCK_RELEASE;
}

void vid_getpos(int *row, int *col)
{
	*row = vid_row;
	*col = vid_col;
}

void vid_setattr(int attr)
{
	vid_curattr = attr;
}

void vid_getattr(int *attr)
{
	*attr = vid_curattr;
}

void vid_putc_noupdate(char c);

void vid_putc(char c)
{
	int level;

	VID_LOCK_ACQUIRE;

	vid_putc_noupdate(c);

	// update vidmem and cursor
	vid_update_vidmem();

	vid_setpos_internal(vid_row,vid_col);

	VID_LOCK_RELEASE;
}

// internal version without vidmem update (for better double buffering)
void vid_putc_noupdate(char c)
{

#ifdef VID_SERIAL_DEBUG
	outb(c,0x3f8);
#endif

	if(c == '\n')
	{
		vid_col = 0;
		vid_row++;
		if(vid_row >= VID_ROWS)
		{
			vid_scrollup(0);
			vid_row = VID_ROWS - 1;
		}
		return;
	}

	if(c == '\r')
	{
		vid_col = 0;
		return;
	}
	
	vid_buf[((vid_row*VID_COLS)+vid_col)*2] = c;
	vid_buf[(((vid_row*VID_COLS)+vid_col)*2)+1] = vid_curattr;

	vid_col++;
	if(vid_col >= VID_COLS)
	{
		vid_col = 0;
		vid_row++;
		if(vid_row >= VID_ROWS)
		{
			vid_scrollup(0);
			vid_row = VID_ROWS -1;
		}
	}
}

void vid_puts_internal(char *s)
{
	while(*s)
		vid_putc_noupdate(*s++);
}

void vid_puts(char *s)
{
	int level;

	VID_LOCK_ACQUIRE;

	// print string to buffer
	vid_puts_internal(s);

	// update vidmem and cursor
	vid_update_vidmem();

	vid_setpos_internal(vid_row,vid_col);

	VID_LOCK_RELEASE;
}

void vid_puthex_internal(int n, int locks);

void vid_puthex(int n)
{
	vid_puthex_internal(n,1);
}

char *hex_digits = "0123456789abcdef";

void vid_puthex_internal(int n, int locks)
{
	char buf[9];
	int i;

	for(i=0;i<8;i++)
	{
		buf[7-i] = hex_digits[n & 0xf];
		n >>= 4;
	}
	buf[8] = 0;

	if(locks)
		vid_puts(buf);
	else
		vid_puts_internal(buf);
}

void vid_putd(int n)
{
	char buf[11], buf2[11];
	int i, j = 0, neg = 0;

	if(n < 0)
	{
		n = -n;
		neg = 1;
	}

	i = 9;
	do
	{
		buf[i] = (n % 10) + '0';
		n /= 10;
		i--;
	}
	while (n != 0 && i >= 0);

	if(neg) buf2[j++] = '-';
	
	while(i++ < 9)
	{
		buf2[j++] = buf[i];
	}

	buf2[j] = 0;

	vid_puts(buf2);
}

