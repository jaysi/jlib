//#include "jnet_internals.h"
//#include "jnet.h"
#include "jpack.h"
#include "jer.h"
#include "debug.h"
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "jpacki.c"

#define _wdeb_crc

/*
	from beej
*/

/*
** pack754() -- pack a floating point number into IEEE-754 format
*/
long long pack754(long double f, unsigned bits, unsigned expbits)
{
	long double fnorm;
	int shift;
	long long sign, exp, significand;
	unsigned significandbits = bits - expbits - 1;	// -1 for sign bit

	if (f == 0.0)
		return 0;	// get this special case out of the way

	// check sign and begin normalization
	if (f < 0) {
		sign = 1;
		fnorm = -f;
	} else {
		sign = 0;
		fnorm = f;
	}

	// get the normalized form of f and track the exponent
	shift = 0;
	while (fnorm >= 2.0) {
		fnorm /= 2.0;
		shift++;
	}
	while (fnorm < 1.0) {
		fnorm *= 2.0;
		shift--;
	}
	fnorm = fnorm - 1.0;

	// calculate the binary form (non-float) of the significand data
	significand = fnorm * ((1LL << significandbits) + 0.5f);

	// get the biased exponent
	exp = shift + ((1 << (expbits - 1)) - 1);	// shift + bias

	// return the final answer
	return (sign << (bits - 1)) | (exp << (bits - expbits - 1)) |
	    significand;
}

/*
** unpack754() -- unpack a floating point number from IEEE-754 format
*/
long double unpack754(long long i, unsigned bits, unsigned expbits)
{
	long double result;
	long long shift;
	unsigned bias;
	unsigned significandbits = bits - expbits - 1;	// -1 for sign bit

	if (i == 0)
		return 0.0;

	// pull the significand
	result = (i & ((1LL << significandbits) - 1));	// mask
	result /= (1LL << significandbits);	// convert back to float
	result += 1.0f;		// add the one back on

	// deal with the exponent
	bias = (1 << (expbits - 1)) - 1;
	shift = ((i >> significandbits) & ((1LL << expbits) - 1)) - bias;
	while (shift > 0) {
		result *= 2.0;
		shift--;
	}
	while (shift < 0) {
		result /= 2.0;
		shift++;
	}

	// sign it
	result *= (i >> (bits - 1)) & 1 ? -1.0 : 1.0;

	return result;
}

/*
** packi16() -- store a 16-bit int into a char buffer (like htons())

void packi16(unsigned char *buf, unsigned int i)
{
	*buf++ = i >> 8;
	*buf++ = i;
}

/*
** packi32() -- store a 32-bit int into a char buffer (like htonl())

void packi32(unsigned char *buf, unsigned long i)
{
	*buf++ = i >> 24;
	*buf++ = i >> 16;
	*buf++ = i >> 8;
	*buf++ = i;
}

void packi64(unsigned char *buf, unsigned long long i)
{
	*buf++ = i >> 56;
	*buf++ = i >> 48;
	*buf++ = i >> 40;
	*buf++ = i >> 32;
	*buf++ = i >> 24;
	*buf++ = i >> 16;
	*buf++ = i >> 8;
	*buf++ = i;
}

/*
** unpacki16() -- unpack a 16-bit int from a char buffer (like ntohs())

unsigned int unpacki16(unsigned char *buf)
{
	return (buf[0] << 8) | buf[1];
}

/*
** unpacki32() -- unpack a 32-bit int from a char buffer (like ntohl())

unsigned long unpacki32(unsigned char *buf)
{
	return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

unsigned long long unpacki64(unsigned char *buf)
{
	return (buf[0] << 56) | (buf[1] << 48) | (buf[2] << 40) | (buf[3] << 32)
	    | (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
}


/*
** pack() -- store data dictated by the format string in the buffer
**
**  h - 16-bit              l - 32-bit
**  c - 8-bit char          f - float, 32-bit
**	t - 64-bit
**  s - string (16-bit length is automatically prepended)
** changed by jaysi to meet the requirements of my lib
*/
size_t pack(unsigned char *buf, char *format, ...)
{
	va_list ap;
	int h;
	int l;
	long long t;
	char c;
	float f;
	char *s;
	size_t size = 0, len = 0;

	va_start(ap, format);

	for (; *format != '\0'; format++) {
		switch (*format) {
		case 'h':	// 16-bit
			size += 2;
			h = va_arg(ap, int);	// promoted
			packi16(buf, h);
			buf += 2;
			break;

		case 'l':	// 32-bit
			size += 4;
			l = va_arg(ap, int);
			packi32(buf, l);
			buf += 4;
			break;
		case 't':	// 64-bit
			size += 8;
			t = va_arg(ap, long long);
			packi64(buf, t);
			buf += 8;
			break;

		case 'c':	// 8-bit
			size += 1;
			c = va_arg(ap, int);	// promoted
			*buf++ = (c >> 0) & 0xff;
			break;

		case 'f':	// float
			size += 4;
			f = va_arg(ap, double);	// promoted
			l = pack754_32(f);	// convert to IEEE 754
			packi32(buf, l);
			buf += 4;
			break;

		case 's':	// string
			if (len <= 0)
				return 0;
			s = va_arg(ap, char *);
			//len = strlen(s);
			size += len + 2;
			packi16(buf, len);
			buf += 2;
			//size += len + 4;
			//packi32(buf, len);
			//buf += 4;
			memcpy(buf, s, len);
			buf += len;
			break;
		default:
			if (isdigit(*format)) {	// track str len
				len = len * 10 + (*format - '0');
			}
		}

		if (!isdigit(*format))
			len = 0;
	}

	va_end(ap);

	return size;
}

/*
** unpack() -- unpack data dictated by the format string into the buffer
** changed by me (jaysi) to meet the requirements of my network code
*/
ssize_t unpack(unsigned char *buf, char *format, ...)
{
	va_list ap;
	short *h;
	int *l;
	int pf;
	long long *t;
	char *c;
	float *f;
	char *s;
	size_t len, maxstrlen = 0;
	ssize_t size = 0;

	va_start(ap, format);

	for (; *format != '\0'; format++) {
		switch (*format) {
		case 'h':	// 16-bit
			h = va_arg(ap, short *);
			*h = unpacki16(buf);
			buf += 2;
			size += 2;
			break;

		case 'l':	// 32-bit
			l = va_arg(ap, int *);
			*l = unpacki32(buf);
			buf += 4;
			size += 4;
			break;

		case 't':	// 64-bit
			t = va_arg(ap, long long *);
			*t = unpacki64(buf);
			buf += 8;
			size += 8;
			break;

		case 'c':	// 8-bit
			c = va_arg(ap, char *);
			*c = *buf++;
			size++;
			break;

		case 'f':	// float
			f = va_arg(ap, float *);
			pf = unpacki32(buf);
			buf += 4;
			*f = unpack754_32(pf);
			size += 4;
			break;

		case 's':	// string
			s = va_arg(ap, char *);
			len = unpacki16(buf);
			buf += 2;
			//len = unpacki32(buf);
			//buf += 4;
			if (maxstrlen > 0 && len > maxstrlen)
				return -JE_LOWBUF;	//count = maxstrlen - 1;
			memcpy(s, buf, len);
			//s[count] = '\0';
			buf += len;
			size += len;
			break;

		default:
			if (isdigit(*format)) {	// track max str len
				maxstrlen = maxstrlen * 10 + (*format - '0');
			}
		}

		if (!isdigit(*format))
			maxstrlen = 0;
	}

	va_end(ap);

	return size;
}
