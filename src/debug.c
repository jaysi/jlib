#include "debug.h"

#ifndef NDEBUG

void _dbuf(unsigned char *buf, int len)
{
	int pos;
	for (pos = 0; pos < len; pos++) {
		printf("%x", buf[pos]);
		if ((buf[pos] < 32) || (buf[pos] > 126)) {
			printf("[.] ");
		} else {
			printf("[%c] ", buf[pos]);
		}

		if ((pos) && !(pos % 10))
			printf("\n");
	}
	printf("\n");
}

#endif
