#ifndef _JPACK_H
#define _JPACK_H

#include <sys/types.h>

//actually this is stolen from beej

long long pack754(long double f, unsigned bits, unsigned expbits);
long double unpack754(long long i, unsigned bits, unsigned expbits);
void packi16(unsigned char *buf, unsigned int i);
void packi32(unsigned char *buf, unsigned long i);
void packi64(unsigned char *buf, unsigned long long i);
unsigned int unpacki16(unsigned char *buf);
unsigned long unpacki32(unsigned char *buf);
unsigned long long unpacki64(unsigned char *buf);
size_t pack(unsigned char *buf, char *format, ...);
ssize_t unpack(unsigned char *buf, char *format, ...);

#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

#else

#endif
