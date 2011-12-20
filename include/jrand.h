#ifndef _JRAND_H
#define _JRAND_H

#include "jtypes.h"

#ifndef linux
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#endif

int jrand_buf(unsigned char *buf, size_t len);

#endif
