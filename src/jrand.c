#include "jrand.h"
#include "jer.h"
#include "debug.h"

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DEV_RANDOM	"/dev/urandom"

#define _deb1(f, a...)

int jrand_buf(unsigned char *buf, size_t len)
{
	int fh, r, s;

	_deb1("openning %s ...", DEV_RANDOM);

	if ((fh = open(DEV_RANDOM, O_RDONLY)) < 0)
		return -JE_OPEN;

	_deb1("ok, reading %u bytes", len);

	for (r = 0; r < len; r += s) {
		_deb1("r = %i", r);
		if ((s = read(fh, buf + r, len - r)) <= 0) {
			_deb1("failed at rd = %i, ret = %i", r, s);
			return -JE_READ;
		}
		_deb1("s = %i", s);
	}

	_deb1("ok, closing handle");

	close(fh);

	return 0;
}

#else

#define _CRT_RAND_S

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int jrand_buf(unsigned char *buf, size_t len)
{
	unsigned int num;
	int npos;

	for (npos = 0; npos <= len / sizeof(num); npos++) {
		//rand_s(&num);
		num = rand();
		memcpy(buf, &num, sizeof(num));
		buf += sizeof(num);
	}

	return 0;
}

#endif
