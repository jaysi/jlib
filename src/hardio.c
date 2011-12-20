/*
 * $Id: hard.c, hard* functions, goin' over EINTR
 */

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

ssize_t hard_read(int fd, char *buf, size_t count)
{
	ssize_t red = 0;

	while (red < count) {
		ssize_t i;

		i = read(fd, buf + red, count - red);
		if (i < 0) {
			if ((errno == EAGAIN) || (errno == EINTR))
				continue;
			return red;
		}
		if (i == 0)
			return red;
		red += i;
	}
	return red;
}

ssize_t hard_write(int fd, char *buf, size_t count)
{
	ssize_t red = 0;

	while (red < count) {
		ssize_t i;

		i = write(fd, buf + red, count - red);
		if (i < 0) {
			if ((errno == EAGAIN) || (errno == EINTR))
				continue;
			return red;
		}
		if (i == 0)
			return red;
		red += i;
	}
#ifndef _WIN32
	fsync(fd);
#endif
	return red;
}
