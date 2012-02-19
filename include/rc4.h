/*
 * $Id: rc4.h, the RC4 stream-cipher
 */

#ifndef RC4_H
#define RC4_H

#include "jtypes.h"

typedef struct {
	uchar state[256];
	uchar x, y;
} rc4_ctx;

extern void rc4_init(uchar * key, uint32_t len, rc4_ctx * ctx);
extern void rc4(uchar * data, uint32_t len, rc4_ctx * ctx);

#endif
