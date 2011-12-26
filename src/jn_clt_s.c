#define JN_CLT_SINGLE
#include "jnet.h"
#include "jer.h"
#include "ecc.h"
#include "debug.h"
#include "jcs.h"
#include "jrand.h"
#include "jhash.h"

#include "jnet_internals.h"

#ifndef __WIN32
#include "unistd.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/in.h"
#include <netdb.h>
#else
#include "winsock.h"
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
#include "sys/wait.h"
#endif
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

int jn_run_cb(jn_conn * conn, jn_pkt * pkt)
{

	return -JE_IMPLEMENT;
/*
	int ret = -JE_NOTFOUND;
	jn_cb_entry *cb;
	void (*fn) (void *arg, jn_conn * conn, jn_pkt * pkt);

	cb = _jn_cb_fifo_search(conn->cbfifo, pkt);

	if (cb) {
		if (cb->flags & JN_CB_CONTINUE)
			ret = JE_CONTINUE;
		else
			ret = 0;

		fn = cb->fn;
		fn(cb->context, conn, pkt);
		if ((!(cb->flags & JN_CB_KEEP)) &&
		    (!(cb->flags & JN_CB_CONTINUE)) &&
		    (!(cb->flags & JN_CB_NOFREE))
		    ) {
			free(cb);
		}

	}

	return ret;
*/
}
