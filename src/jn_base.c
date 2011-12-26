#include "jnet.h"
#include "jer.h"
#include "ecc.h"
#include "debug.h"
#include "jcs.h"
#include "jhash.h"

#include "jnet_internals.h"

#ifndef __WIN32
#include "sys/wait.h"
#include "unistd.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/in.h"
#else
#include "winsock2.h"
#include <ws2tcpip.h>
#define close(fd) close_socket(fd)
#endif
#include "sys/types.h"
#include "sys/stat.h"
#include "string.h"
#include "wchar.h"
#include "stdio.h"
#include "stdlib.h"
#include "signal.h"
#include "errno.h"
#ifdef linux
#include "error.h"
#endif
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <wchar.h>
#include <fcntl.h>

#define _wdeb1 _wdeb
#define _wdeb2(f, a...)
#define _wdeb3 _wdeb		//FORBID check for COMREQ
#define _wdeb4 _wdeb		//login debug

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);

}

static inline int _jn_assert_plain(jn_pkt * pkt)
{
	if (pkt->hdr.data_len < sizeof(int16_t))
		return -JE_SIZE;
	if (pkt->hdr.data_len > sizeof(int16_t)) {
		if (pkt->hdr.data_len < sizeof(int16_t) + JN_MIN_TXT_SIZE) {
			return -JE_SIZE;
		} else {
			if (pkt->data[0] == '\0')
				return -JE_INV;
			if (pkt->data[pkt->hdr.data_len - sizeof(int16_t)] !=
			    '\0')
				return -JE_INV;
		}
	}
	return 0;
}

int _jn_data_crc(jn_pkt * pkt)
{
	uint16_t crc = 0UL, old_crc = pkt->hdr.data_crc;

	if (pkt->hdr.data_len) {
		_wdeb1(L"datalen = %u", pkt->hdr.data_len);
		crc = _jn_crc(pkt->data, pkt->hdr.data_len);
		pkt->hdr.data_crc = crc;

	} else {
		pkt->hdr.data_crc = 0;
	}

	if (pkt->hdr.data_len) {
		_wdeb1(L"old_crc = 0x%x, calc_crc = 0x%x", old_crc, crc);
		return ((old_crc == crc) ? 0 : -JE_CRC);
	}

	return 0;
}

int _jn_is_fixed_data(jn_hdr hdr)
{
	switch (hdr.type) {
	case JNT_LOGIN:
	case JNT_TIMEOUT:
	case JNT_DC:
	case JNT_COMLIST:
	case JNT_COMJOIN:
	case JNT_COMLEAVE:
		return 1;
	default:
		return 0;
	}
}

int _jn_assert_pkt(jn_pkt * pkt, int chkdata)
{
	int ret;

	if (pkt->hdr.magic != JN_PKT_MAGIC)
		return -JE_MAGIC;

	if (pkt->hdr.ver != JN_VER)
		return -JE_VER;

	if (pkt->hdr.data_len > (JN_MAX_PKT - sizeof(jn_hdr))) {
		_wdeb(L"failing here, pkt->hdr.data_len = %u",
		      pkt->hdr.data_len);
		return -JE_SIZE;
	}

	if (pkt->hdr.flags & JNF_PLAIN) {
		if (pkt->hdr.data_len) {
			switch (pkt->hdr.type) {
			case JNT_OK:
			case JNT_FAIL:
			case JNT_COMREQ:
				break;
			default:
				_wdeb(L"forbidden, PLAIN flag with type = 0x%x",
				      pkt->hdr.type);
				return -JE_FORBID;
			}
		}
	}

	switch (pkt->hdr.type) {
	case JNT_LOGIN:
		if (pkt->hdr.data_len != sizeof(jn_login_payload)) {
			_wdeb(L"failing here");
			return -JE_SIZE;
		}
		break;
	case JNT_OK:		//len is unknown, may have plain text payload                       
		if ((chkdata) && (pkt->hdr.flags & JNF_PLAIN)) {
			if ((ret = _jn_assert_plain(pkt)) < 0) {
				_wdeb(L"plain assert fails: %i", ret);
				return ret;
			}
		}
		break;
	case JNT_TIMEOUT:
		if (pkt->hdr.data_len) {
			_wdeb(L"failing here");
			return -JE_SIZE;
		}
		break;
	case JNT_DC:
		if (pkt->hdr.data_len) {
			_wdeb(L"failing here");
			return -JE_SIZE;
		}
		break;
	case JNT_COMLIST:
		if (pkt->hdr.data_len) {
			_wdeb(L"failing here");
			return -JE_SIZE;
		}
		break;
	case JNT_COMOPEN:	//len is unknown
		if (pkt->hdr.data_len <
		    (sizeof(uchar) + sizeof(jobjid_t) + JN_MIN_TXT_SIZE)) {
			_wdeb(L"failing here");
			return -JE_SIZE;
		}
		if (pkt->hdr.data_len >
		    (sizeof(uchar) + sizeof(jobjid_t) + JN_MAX_COM_NAME)) {
			_wdeb(L"failing here");
			return -JE_SIZE;
		}
		break;
	case JNT_COMJOIN:
		if (pkt->hdr.data_len) {
			_wdeb(L"failing here");
			return -JE_SIZE;
		}
		break;
	case JNT_COMLEAVE:
		if (pkt->hdr.data_len) {
			_wdeb(L"failing here");
			return -JE_SIZE;
		}
		break;
	case JNT_COMCLOSE:
		if (pkt->hdr.data_len) {
			_wdeb(L"failing here");
			return -JE_SIZE;
		}
		break;
	case JNT_COMREQ:	//unknown datalen
		break;
	case JNT_COMPAUSE:
		if (pkt->hdr.data_len) {
			_wdeb(L"failing here");
			return -JE_SIZE;
		}
		break;
	case JNT_COMRESUME:
		if (pkt->hdr.data_len) {
			_wdeb(L"failing here");
			return -JE_SIZE;
		}
		break;
	case JNT_MODLIST:
		if (pkt->hdr.data_len) {
			_wdeb(L"failing here");
			return -JE_SIZE;
		}
		break;
	case JNT_FAIL:		//len is not known, may contain plain text info
		if ((chkdata) && (pkt->hdr.flags & JNF_PLAIN)) {
			if ((ret = _jn_assert_plain(pkt)) < 0) {
				_wdeb(L"failing here");
				return ret;
			}
		}
		break;
	default:
		_wdeb(L"unknown type: %x", pkt->hdr.type);
		return -JE_TYPE;
	}

/*
	if(chkdata){
		
	}
*/

	return 0;
}

int send_all(int fd, uchar * buf, uint32_t * len, struct timeval *tv)
{
	uint32_t total;
	uint32_t bytesleft = *len;
	int n;
	total = 0;
	fd_set writefd;
#ifndef NDEBUG
	int parts = 0;
#endif
#ifdef WIN32
	struct timeval tv1, tv2;
#endif

	_wdeb1(L"called, conn->sockfd = %i", fd);
	_wdeb1(L"len is %i", (int)*len);

	if (!tv) {
		while (total < *len) {
			n = send(fd, buf + total, bytesleft, 0);
			if (n == -1) {
				break;
			}
			total += n;
			bytesleft -= n;
		}
	} else {
		_wdeb1(L"for timed-out write");

		FD_ZERO(&writefd);
		FD_SET(fd, &writefd);
 retryselect:
		while (total < *len) {
			_wdeb1(L"timeout sec = %u, usec = %u\n",
			       (uint16_t) tv->tv_sec, (uint16_t) tv->tv_usec);

			//READ TIME HERE
#ifdef WIN32
			if (gettimeofday(&tv1, NULL) == -1) {
				return -JE_GETTIME;
			}
#endif

			if (select(fd + 1, NULL, &writefd, NULL, tv) == -1) {
				if (errno == EINTR)
					goto retryselect;
				else {
					return -JE_SYSE;
				}
			}
			if (FD_ISSET(fd, &writefd)) {
				n = send(fd, buf + total, bytesleft, 0);
				if ((n == -1) || (n == 0))
					break;

				_wdeb4(L"send() returns %i", n);
#ifndef NDEBUG
				parts++;
#endif
				total += n;
				bytesleft -= n;
				if (bytesleft == 0)
					break;

				//READ TIME HERE, SEE IF THERE'S ANY TIME REMAINED
				//CONTINUE WITH REMAINING TIME
#ifdef WIN32
				if (gettimeofday(&tv2, NULL) == -1) {
					return -JE_GETTIME;
				}
				tv->tv_sec = tv2.tv_sec - tv1.tv_sec;
				tv->tv_usec = tv2.tv_usec - tv1.tv_usec;	//TODO:bad imp.
#endif
				if ((tv->tv_sec == 0) && (tv->tv_usec == 0))
					break;
				else
					goto retryselect;
			} else {	//timed-out
				_wdeb1(L"timed out");
				return -JE_TIMEOUT;
			}
		}
	}

	*len = total;

	_wdeb2(L"sent %i bytes in %i parts", (int)total, parts);

	return bytesleft == 0 ? 0 : -JE_DC;
}

int recv_all(int fd, uchar * buf, uint32_t * len, struct timeval *tv)
{

	//_wdeb(L"Code is out-of-order, see recv_pkt_all()");

	uint32_t total;
	uint32_t bytesleft = *len;
	int n;
	total = 0;
	fd_set readfd;
	unsigned long imode;
#ifdef WIN32
	struct timeval tv1, tv2;
#endif

	_wdeb1(L"called, conn->sockfd = %i", fd);
	_wdeb1(L"buf size = %i", (int)*len);
	if (!tv) {

		_wdeb1(L"for blocking-once read");

		n = recv(fd, buf, *len, 0);
		if ((n == -1) || (n == 0)) {
			goto end;
		}
		total = n;
		bytesleft -= n;
	} else {		//timed-out

		_wdeb1(L"for timed-out read");

		FD_ZERO(&readfd);
		FD_SET(fd, &readfd);
 retryselect:
		while (total < *len) {
			_wdeb1(L"timeout sec = %u, usec = %u",
			       (uint16_t) tv->tv_sec, (uint16_t) tv->tv_usec);

			//READ TIME HERE
#ifdef WIN32
			if (gettimeofday(&tv1, NULL) == -1) {
				return -JE_GETTIME;
			}
#endif
			if (select(fd + 1, &readfd, NULL, NULL, tv) == -1) {
				if (errno == EINTR)
					goto retryselect;
				else {
					return -JE_SYSE;
				}
			}
			if (FD_ISSET(fd, &readfd)) {

				imode = JN_MODE_NONBLOCK;
				_jn_block_mode(fd, &imode);

				n = recv(fd, buf + total, bytesleft, 0);

				imode = JN_MODE_BLOCK;
				_jn_block_mode(fd, &imode);

				if ((n == -1) || (n == 0))
					break;
				total += n;
				bytesleft -= n;
				if (bytesleft == 0)
					break;

				//READ TIME HERE, SEE IF THERE'S ANY TIME REMAINED
				//CONTINUE WITH REMAINED TIME
#ifdef WIN32
				if (gettimeofday(&tv2, NULL) == -1) {
					return -JE_GETTIME;
				}
				//to avoid integer overflow
				if (tv2.tv_sec > tv1.tv_sec) {
					tv->tv_sec = tv2.tv_sec - tv1.tv_sec;
				} else
					tv->tv_sec = 0L;
				if (tv2.tv_usec > tv1.tv_usec) {
					tv->tv_usec =
					    tv2.tv_usec - tv1.tv_usec;
				} else
					tv->tv_usec = 0L;
#endif
				if ((tv->tv_sec == 0) && (tv->tv_usec == 0))
					break;
				else
					goto retryselect;
			} else {	//timed-out
				_wdeb1(L"timed-out");
				return -JE_TIMEOUT;
			}
		}
	}
 end:
	*len = total;

	_wdeb1(L"got %i bytes, bytesleft = %i", (int)total, bytesleft);

	return bytesleft == 0 ? 0 : -JE_DC;
}

int recv_pkt_all(int fd, uchar * buf, uint32_t * len, struct timeval *tv,
		 aes_context * aes, jn_pkt * pkt)
{

	//_wdeb(L"Code is out-of-order, see recv_pkt_all()");

	uint32_t total;
	uint32_t bytesleft = *len;
	int n;
	total = 0;
	fd_set readfd;
	unsigned long imode;
	int got_hdr;
	int ret;
#ifdef WIN32
	struct timeval tv1, tv2;
#endif

	_wdeb1(L"for timed-out read");

	FD_ZERO(&readfd);
	FD_SET(fd, &readfd);
	got_hdr = 0;
 retryselect:
	while (bytesleft) {
		_wdeb1(L"timeout sec = %u, usec = %u, bytesleft = %u",
		       (uint16_t) tv->tv_sec, (uint16_t) tv->tv_usec,
		       bytesleft);

		//READ TIME HERE
#ifdef WIN32
		if (gettimeofday(&tv1, NULL) == -1) {
			return -JE_GETTIME;
		}
#endif
		if (select(fd + 1, &readfd, NULL, NULL, tv) == -1) {
			if (errno == EINTR)
				goto retryselect;
			else {
				return -JE_SYSE;
			}
		}
		if (FD_ISSET(fd, &readfd)) {

			imode = JN_MODE_NONBLOCK;
			_jn_block_mode(fd, &imode);

			n = recv(fd, buf + total, bytesleft, 0);

			imode = JN_MODE_BLOCK;
			_jn_block_mode(fd, &imode);

			if ((n == -1) || (n == 0))
				break;

			total += n;

			_wdeb1(L"got %i bytes, total = %u", n, total);

			if (!got_hdr) {

#ifdef USE_AES
				aes_decrypt(aes, buf, buf);
#endif

				if ((ret = _jn_disassm_hdr(&pkt->hdr, buf)) < 0)
					return ret;

				if ((ret = _jn_assert_pkt(pkt, 0)) < 0)
					return ret;

				if (pkt->hdr.data_len > ((*len) - total))
					return -JE_LOWBUF;

				bytesleft = enc_data_size(pkt->hdr.data_len);

				got_hdr = 1;

				_wdeb1(L"got hdr, need %u bytes",
				       pkt->hdr.data_len);

			} else {

				bytesleft -= n;

			}

			if (total >= bytesleft) {
				return 0;
			}
			//READ TIME HERE, SEE IF THERE'S ANY TIME REMAINED
			//CONTINUE WITH REMAINED TIME
#ifdef WIN32
			if (gettimeofday(&tv2, NULL) == -1) {
				return -JE_GETTIME;
			}
			//to avoid integer overflow
			if (tv2.tv_sec > tv1.tv_sec) {
				tv->tv_sec = tv2.tv_sec - tv1.tv_sec;
			} else
				tv->tv_sec = 0L;
			if (tv2.tv_usec > tv1.tv_usec) {
				tv->tv_usec = tv2.tv_usec - tv1.tv_usec;
			} else
				tv->tv_usec = 0L;
#endif
			if ((tv->tv_sec == 0) && (tv->tv_usec == 0))
				break;
			else
				goto retryselect;
		} else {	//timed-out
			_wdeb1(L"timed-out");
			return -JE_TIMEOUT;
		}
	}

	*len = total;

	_wdeb1(L"got %i bytes, bytesleft = %i", (int)total, bytesleft);

	return bytesleft == 0 ? 0 : -JE_DC;
}

uint32_t jn_calc_rs(uint32_t data_size)
{
	uint32_t npkt;
	uint32_t enc_size;
	enc_size = enc_data_size(data_size);
	npkt = enc_size / JN_MAX_PKT_DATA;
	return enc_size + (npkt * sizeof(jn_hdr));
}
