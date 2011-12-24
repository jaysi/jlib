#include "jnet.h"
#include "jer.h"
#include "ecc.h"
#include "debug.h"
#include "jcs.h"
#include "jrand.h"
#include "jhash.h"
#include "hardio.h"

#include "jnet_internals.h"

#ifndef __WIN32
#include "sys/wait.h"
#include "unistd.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/in.h"
#include <netdb.h>
#else
#include "winsock2.h"
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

/* _deb1? definitions*/
#define _deb1(f, a...)
#define _wdeb1(f, a...)
#define _deb2(f, a...)
#define _wdeb2(f, a...)
#define _wdeb4 _wdeb		//send_io_completion_handler

void jn_default_conf(jn_conf * s)
{

	assert(s);

	s->flags =
	    (JN_IF_BIND | JN_IF_SOCKOPT | JN_IF_LISTEN | JN_IF_LOGIN |
	     JN_IF_CRYPT);

	s->so_level = SOL_SOCKET;
	s->so_optname = SO_REUSEADDR;

	s->port = JN_CONF_PORT;

	s->nlisten = JN_CONF_LISTEN_BACKLOG;

	s->mod_path = JN_CONF_MOD_PATH;

	s->nfds_buck = JN_CONF_NFDS_BUCK;
	s->max_com = JN_CONF_MAX_COM;
	s->max_mod = JN_CONF_MAX_MODS;
	s->max_users = JN_CONF_MAX_USERS;
	s->maxconnreq = JN_CONF_MAX_CONN_REQ;
	s->max_ucom = JN_CONF_MAX_UCOM;
	s->maxcomreq = JN_CONF_MAX_COM_REQ;
	s->max_conn_trans = JN_CONF_MAX_TRANS;
	s->poll_wr_req = JN_CONF_POLL_WR_REQ;

	s->poll_q = JN_CONF_POLL_Q;
	s->svr_q = JN_CONF_SVR_Q;

	s->cbq_gc_trigger = JN_CONF_CBQ_TRIGGER;
	s->cbq_gc_timeout = JN_CONF_CBQ_TIMEOUT;

	s->pkt_tv.tv_sec = JN_CONF_PKT_TIMEOUT;
	s->pkt_tv.tv_usec = 0L;
	s->hs_tv.tv_sec = JN_CONF_HANDSHAKE_TIMEOUT;
	s->hs_tv.tv_usec = 0L;
	s->login_tv.tv_sec = JN_CONF_LOGIN_TIMEOUT;
	s->login_tv.tv_usec = 0L;

	s->nmaxfifoq = JN_CONF_FIFO_Q_POOL_N;
	s->fifoqbufsize = JN_CONF_FIFO_QBUF_POOL_SIZE;

}

//limmited to the login
int jn_send(jn_conn * conn, jn_pkt * pkt)
{
	uint32_t len = enc_pkt_size(pkt->hdr.data_len);	//+sizeof(jn_hdr));
	int i;
	struct timeval tv;
	uchar buf[JN_MAX_PKT];

	if (len > JN_MAX_PKT)
		return -JE_SIZE;

	_wdeb1(L"sending packet, tid = %u", pkt->hdr.trans_id);

	assert(!(len % 16));

	_jn_assm_hdr(buf, pkt->data, pkt->hdr.flags, pkt->hdr.type,
		     pkt->hdr.data_len, pkt->hdr.comod_id,
		     pkt->hdr.trans_id, pkt->hdr.rs);

	aes_encrypt(&conn->aes, buf, buf);

	for (i = 0; i < pkt->hdr.data_len; i += AES_BSIZE) {
		aes_encrypt(&conn->aes, pkt->data + i,
			    buf + i + sizeof(jn_hdr));
	}

	_milli2tv(JN_CONF_LOGIN_TIMEOUT, &tv);

	i = send_all(conn->sockfd, buf, &len, &tv);

	conn->bytes_sent += len;

	return i;
}

//limmited to the login
int jn_recv(jn_conn * conn, jn_pkt * pkt)
{
	uint32_t len;
	size_t enc_data_len;
	int i;
	struct timeval tv;
	uchar buf[JN_MAX_PKT];
	int ret;

	len = JN_MAX_PKT;
	_milli2tv(JN_CONF_PKT_TIMEOUT, &tv);

	ret = recv_pkt_all(conn->sockfd, buf, &len, &tv, &conn->aes, pkt);
	_wdeb1(L"got %u bytes, data_len = %u, ret = %i", len, ret);

	if (ret < 0) {
		if (len < sizeof(jn_hdr)) {
			pkt->hdr.trans_id = 0L;
		}
		goto end;
	}

	assert(!(len % 16));

	//no need to call this, already called in recv_pkt_all
	//if((ret = _jn_disassm_hdr(&pkt->hdr, buf)) < 0) goto end;

	enc_data_len = enc_data_size(pkt->hdr.data_len);
	_wdeb1(L"data_len = %u", pkt->hdr.data_len);

	if (pkt->hdr.data_len) {
		pkt->data = (uchar *) pmalloc(enc_data_len);
		_wdeb1(L"allocating %u bytes for data @ 0x%p", enc_data_len,
		       pkt->data);
		if (!pkt->data)
			return -JE_MALOC;
		for (i = 0; i < enc_data_len; i += 16) {
			aes_decrypt(&conn->aes, buf + i + sizeof(jn_hdr),
				    pkt->data + i);
		}
	}

 end:
	_wdeb1(L"got packet, tid: %u", pkt->hdr.trans_id);
	if (!ret && pkt->hdr.data_len) {
		return _jn_data_crc(pkt);
	}

	return ret;
}

ssize_t _jn_prepare_pkt(jn_conn * conn, jn_pkt * pkt, uchar * buf)
{

	int i;
	ssize_t len = enc_pkt_size(pkt->hdr.data_len);

	if (len > JN_MAX_PKT)
		return -JE_SIZE;

	_jn_assm_hdr(buf, pkt->data, pkt->hdr.flags, pkt->hdr.type,
		     pkt->hdr.data_len, pkt->hdr.comod_id,
		     pkt->hdr.trans_id, pkt->hdr.rs);

	//encrypt header + chunk of data, if exists

	aes_encrypt(&conn->aes, buf, buf);

	for (i = 0; i < pkt->hdr.data_len; i += AES_BSIZE) {
		aes_encrypt(&conn->aes, pkt->data + i,
			    buf + i + sizeof(jn_hdr));
	}

	return len;

}

size_t jn_pkt_array(uchar * buf, size_t bufsize, jn_pkt ** pkt,
		    uchar * datapool)
{
	size_t npkt, i, remain;
	npkt = bufsize / JN_MAX_PKT_DATA;
	if (bufsize % JN_MAX_PKT_DATA)
		npkt++;

	*pkt = (jn_pkt *) malloc(sizeof(jn_pkt) * npkt);
	if (!(*pkt))
		return ((size_t) - 1);

	remain = bufsize;

	for (i = 0; i < npkt; i++) {

		if (remain >= JN_MAX_PKT_DATA) {
			pkt[i]->hdr.data_len = JN_MAX_PKT_DATA;
			remain -= JN_MAX_PKT_DATA;
		} else {
			pkt[i]->hdr.data_len = remain;
			remain = 0L;
		}

		pkt[i]->hdr.rs = remain;

		if (datapool) {
			pkt[i]->data = datapool + (i * JN_MAX_PKT_DATA);
		} else {
			pkt[i]->data = (uchar *) malloc(pkt[i]->hdr.data_len);
			memcpy(pkt[i]->data,
			       buf + (bufsize + (i * JN_MAX_PKT_DATA)),
			       pkt[i]->hdr.data_len);
		}
	}

	return npkt;
}

void jn_free_pkt_array(jn_pkt ** pkt, size_t npkt, uchar * datapooladdr)
{
	size_t i;
	if (datapooladdr) {
		free(datapooladdr);
	}
	for (i = 0; i < npkt; i++) {
		if (!datapooladdr) {
			free(pkt[i]->data);
		}
		free(pkt[i]);
	}
}
