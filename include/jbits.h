#ifndef __BITS_H
#define __BITS_H

/*
**  Macros to manipulate bits in an array of char.
**  These macros assume CHAR_BIT is one of either 8, 16, or 32.
*/

#define MASK  CHAR_BIT-1
#define SHIFT ((CHAR_BIT==8)?3:(CHAR_BIT==16)?4:5)

#define BitOff(a,x)  ((void)((a)[(x)>>SHIFT] &= ~(1 << ((x)&MASK))))
#define BitOn(a,x)   ((void)((a)[(x)>>SHIFT] |=  (1 << ((x)&MASK))))
#define BitFlip(a,x) ((void)((a)[(x)>>SHIFT] ^=  (1 << ((x)&MASK))))
#define IsBit(a,x)   ((a)[(x)>>SHIFT]        &   (1 << ((x)&MASK)))

/*	10000001
	^      ^
	Bit7   Bit0
*/

#define _8b0	0x01
#define _8b1	0x02
#define _8b2	0x04
#define _8b3	0x08
#define _8b4	0x10
#define _8b5	0x20
#define _8b6	0x40
#define _8b7	0x80

/*
	by jaysi for global bit ops on buffers, damn slow!
*/
#define set_bit(buf, i)	((buf[(i)/8] |= (0x01 << ((i)%8))))
#define test_bit(buf, i) ((buf[(i)/8] & (0x01 << ((i)%8))))
#define unset_bit(buf, i) ((buf[(i)/8] &= ~(0x01 << ((i)%8))))
#define filp_bit(buf , i) ((buf[(i)/8] ^= (0x01 << ((i)%8))))

/*
** Macros for faster bit ops on a 8bit v
*/
#define _8bOff(a,x)	(a&=~x)
#define _8bOn(a,x)	(a|=x)
#define _8bFlip(a,x)	(a^=x)
#define _8bIs(a,x)	(a&x)

#else
#endif
