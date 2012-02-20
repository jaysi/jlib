
#ifndef _JER_H
#define _JER_H

#include "jbits.h"

#define MAX_ERR_STR	30

/*file*/
#define JE_OPEN		1
#define JE_STAT		2
#define JE_READ		3
#define JE_WRITE	4
#define JE_SEEK		5
#define JE_CRC		6
#define JE_NOOPEN	7
#define JE_ISOPEN	8
#define JE_LASTOP	9
#define JE_NOJAC	10
#define JE_NOFILE	11
#define JE_PIPE		12
#define JE_HCRC		13	//header crc
#define JE_CORRUPT	14	//corrupted
#define JE_JOURNAL	15
#define JE_NOJOURNAL	16

/*user management*/
#define JE_NOGROUP	20
#define JE_NOUSER	21
#define JE_NOOBJECT	22
#define JE_NOCONF	23
#define JE_NOLOGIN	24
#define JE_PASS		25
#define JE_UIDFULL	26
#define JE_GIDFULL	27
#define JE_NOPERM	28
#define JE_TIKS		29
#define JE_MAXLOGINS	30
#define JE_SIZE		31
#define JE_BADCONF	32
#define JE_BUFOVER	33

/*network*/
#define JE_ACCEPT	40
#define JE_SEND		41
#define JE_RECV		42
#define JE_BADPUBKEY	43
#define JE_SOCKET	44
#define JE_SETSOCKOPT	45
#define JE_BIND		46
#define JE_LISTEN	47
#define JE_CONNECT	48
#define JE_HDR		49	//malformed header
#define JE_DC		50	//disconnected
#define JE_TID		51	//incompatible transmission id
#define JE_BCAST	52	//broadcast reply packet to the list, see _jn_proc_pkt()
#define JE_PKTLOST	53

/*shared, common*/
#define JE_EXISTS	60
#define JE_NOTFOUND	61
#define JE_UNK		62
#define JE_IMPLEMENT	63
#define JE_TYPE		64
#define JE_NOINIT	65
#define JE_ALREADYINIT	66
#define JE_LIMIT	67
#define JE_DISABLE	68
#define JE_SYSE		69	//system error
#define JE_VER		70
#define JE_FORBID	71
#define JE_TIMEOUT	72
#define JE_GETTIME	73
#define JE_CONTINUE	74
#define JE_NEXT_I	75
#define JE_NEXT_BLOCK	76
#define JE_MAGIC	77
#define JE_END		78
#define JE_INV		79	//invalid
#define JE_EMPTY	80	//empty entry
#define JE_FUL		81
#define JE_FLAG		82
#define JE_FAIL		83
#define JE_PAUSE	84	//service paused, used in server code
#define JE_LOWRES	85	//low resources
#define JE_LOWBUF	86
#define JE_TMPUNAV	87	//temporary unavailable
#define JE_AGAIN	88
#define JE_TOOLONG	89
#define JD_NOTSET	90

/*memmory*/
#define JE_NOMEM	100
#define JE_MALOC	101
#define JE_NULLPTR	102

/*threads*/
#define JE_THREAD	110
#define JE_THREADJOIN	111

/*modules*/
#define JE_BADMOD	120
#define JE_RESOLV	121

/*parser*/
#define JE_SYNTAX	130
#define JE_TOK		131

/*

*/
#define JE_NOJCS	140

/*

*/
#define JE_CRYPT	150

#define JEF_FILELOG	_8b0
#define JEF_TIMELOG	_8b1
#define JEF_DATELOG	_8b2

/*
typedef struct JErrorHandle{
	char flags;
	int fd;//file to log
	int sys;//sys code
	int* errs;//chain of errors
	char* strings;//chain of strings, so i can keep track of errors in functions
}jer_h;
*/

int jer_lookup_str(int err, char *dest);
/*
int jer_log(int
//static int jer_err_msg(int err, char*/

#endif
