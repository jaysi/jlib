#include "jnet.h"
#include "jer.h"
#include "ecc.h"
#include "debug.h"
#include "jcs.h"
#include "jrand.h"
#include "jhash.h"
#include "sha256.h"

#include "jnet_internals.h"

#ifndef __WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
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
#include "sys/wait.h"
#endif
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

/* _deb1? definitions*/
#define _deb1(f, a...)
#define _wdeb1 _wdeb
#define _deb2(f, a...)
#define _wdeb2(f, a...)
#define _perr(str)		//jn_connect
#define _wdeb3(f, a...)		//jn_connect
#define _wdeb4 _wdeb		//new callback subsystem
#define _wdeb5 _wdeb		//cb tracker

#ifndef NDEBUG
void _jn_dump_cbfifo(jn_cb_fifo * fifo)
{
	jn_cb_entry *entry;
	int i = 0;
	for (entry = fifo->first; entry; entry = entry->next) {

		_wdeb(L"[%i]: tid = %u, comid = %u, flags = %x", i, entry->tid,
		      entry->comid, entry->flags);
		i++;
	}
}
#else
#define _jn_dump_cbfifo(fifo)
#endif

int _jn_clt_handshake(jn_conn * conn)
{
	ecc_keys *key;
	jn_hs_pkt pkt;
	jn_hs_reply_pkt rpkt;
	uint32_t len;
	uchar pwhash[32];

	_wdeb1(L"generating keys");

	key = ECIES_generate_keys();

	len = sizeof(jn_hs_pkt);
	pkt.ver = JN_VER;
	strlcpy(pkt.px, key->px, MAX_PUBKEY_LEN);
	strlcpy(pkt.py, key->py, MAX_PUBKEY_LEN);

	_wdeb1(L"sending pubkey");
#ifndef NDEBUG
	wprintf(L"KPx, PPx: %s, %s\n", key->px, pkt.px);
	wprintf(L"KPy, PPy: %s, %s\n", key->py, pkt.py);
#endif
	if ((send_all(conn->sockfd, (uchar *) & pkt, &len, NULL)) < 0) {
		_wdeb1(L"send_failed");
		free(key);
		return -JE_SEND;
	}

	_wdeb1(L"waiting for reply packet");

	len = sizeof(jn_hs_reply_pkt);
	if ((recv_all(conn->sockfd, (uchar *) & rpkt, &len, NULL)) < 0) {
		_wdeb1(L"recv failed");
		free(key);
		return -JE_RECV;
	}

	if (rpkt.flags & JN_HS_BIG_ENDIAN) {
		_wdeb(L"Big Endian Peer");
		conn->flags |= JN_CF_BIG_ENDIAN_PEER;
	}

	_wdeb1(L"got reply, checking...");

	if (!strncmp(rpkt.ekey, "##Invalid Pub Key##", 32)) {
		_wdeb1(L"invalid public key was sent");
		free(key);
		return -JE_BADPUBKEY;
	}

	_wdeb1(L"decrypting...");

	//conn->key = (uchar*)malloc(JN_CONF_KEY_LEN);
	if (ECIES_decryption
	    ((char *)conn->key, rpkt.ekey, JN_CONF_KEY_LEN, key->priv) < 0) {
		_wdeb1(L"decryption failed");
		//free(conn->key);
		free(key);
		return -JE_UNK;
	}

/*
#ifndef NDEBUG
	for(len = 0; len < JN_CONF_KEY_LEN; len++){
		if(conn->key[len] != 'a'){
			_deb("conn->key[%u] != 'a'", len);
		}
	}
	_wdeb1(L"key checked");
#endif
*/

//      _wdeb1(L"done, setting aes key <%s>", ciphk);

	sha256(conn->key, JN_CONF_KEY_LEN, pwhash);
	//conn->aes = (aes_context*)malloc(sizeof(aes_context));
	aes_set_key(&conn->aes, pwhash, 256);

	_wdeb1(L"ok, handshaking complete");

	free(key);

	return 0;
}

int _jn_clt_login(jn_conn * conn, wchar_t * username, wchar_t * pass,
		  uchar flags)
{
	jn_pkt pkt;
	jn_login_payload lin;
	int ret;

	if ((wtojcs(username, lin.username, MAX_UNAME) == ((size_t) - 1))) {
		ret = -JE_INV;
		goto end;
	}
	if ((wtojcs(pass, lin.pass, MAX_PASS) == ((size_t) - 1))) {
		ret = -JE_INV;
		goto end;
	}

	pkt.hdr.flags = 0;
	pkt.hdr.type = JNT_LOGIN;
	pkt.hdr.data_len = sizeof(jn_login_payload);
#ifdef NO_HEAP
	memcpy(pkt.data, (uchar *) & lin, sizeof(jn_login_payload));
#else
	pkt.data = (uchar *) & lin;
#endif
	//pkt.data = uchar*)malloc(pkt.hdr.data_len);
	//memcpy(pkt.data, &lin, pkt.hdr.data_len);
	_wdeb1(L"sending req");

	if ((ret = jn_send(conn, &pkt)) < 0)
		goto end;

	_wdeb1(L"ok, waiting reply");

	if ((ret = jn_recv(conn, &pkt)) < 0)
		goto end;

	_wdeb1(L"got reply");

	if (pkt.hdr.type == JNT_OK)
		ret = 0;
	else
		ret = -JE_NOLOGIN;

	_wdeb1(L"ret = %i", ret);

 end:

	if (ret < 0) {
		//_destroy_cond(&conn->req_cond);
#ifndef _WIN32
		close(conn->sockfd);
#else
		closesocket(conn->sockfd);
#endif
	}

	_wdeb1(L"return %i", ret);

	conn->flags &= ~JN_CF_SVR;

	return ret;

}

int jn_connect(jn_conn * conn, char *hostname, char *port, wchar_t * user,
	       wchar_t * pass)
{
	int ret;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	//char s[INET6_ADDRSTRLEN];

	_wdeb1(L"connecting to %s %s as %S %S", hostname, port, user, pass);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		_wdeb3(L"getaddrinfo: %s\n", gai_strerror(rv));
		return -JE_SYSE;
	}
	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((conn->sockfd = socket(p->ai_family, p->ai_socktype,
					   p->ai_protocol)) == -1) {
			_perr("client: socket");
			continue;
		}

		if (connect(conn->sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(conn->sockfd);
			_perr("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		_wdeb3(L"client: failed to connect\n");
		return -JE_CONNECT;
	}

	_deb1("net init passed, now going to hs & lin stuff");

	if (!(ret = _jn_clt_handshake(conn)))
		ret = _jn_clt_login(conn, user, pass, 0x00);

	_deb1("ret = %i", ret);

	if (ret < 0)
		return ret;

	_jn_conn_init(NULL, conn);

	conn->cbfifo = _jn_cb_fifo_new();
	if (!conn->cbfifo) {
		_jn_ucom_list_destroy(conn->ucom_list);
		return -JE_MALOC;
	}

	conn->trans_a = _jn_transa_new(JN_CONF_MAX_TRANS);
	if (!conn->trans_a) {
		_jn_ucom_list_destroy(conn->ucom_list);
		_jn_cb_fifo_destroy(conn->cbfifo);
		return -JE_MALOC;
	}
#ifndef JN_SINGLE_CLT
	if (_create_thread
	    (&(conn->cbthid), NULL, _jn_clt_callback_thread,
	     (void *)conn) < 0) {
		_wdeb(L"failed to create thread");
//              conn->flags &= ~JN_CF_PS;
		ret = -JE_THREAD;
	} else {
		pthread_detach(conn->cbthid);
	}
#endif

	return 0;
}

static inline void _jn_clt_handle_err(jn_conn * conn, jn_pkt * err_pkt, int ret)
{
	void (*progress_cb) (jn_cb_entry *);
	jn_cb_entry *q;

	progress_cb = NULL;

	q = _jn_cb_fifo_search_flags(conn->cbfifo, JN_CB_ERR);

	if (q) {
		_wdeb4(L"found error q");
		progress_cb = q->progress_callback;
		q->pkt = err_pkt;
		q->jerr = ret;
	}

	if (progress_cb) {
		_wdeb4(L"calling error handler");
		progress_cb(q);

	} else {
		if (err_pkt) {
			if (err_pkt->hdr.data_len)
				free(err_pkt->data);
			free(err_pkt);
		}
	}
}

#ifndef JN_SINGLE_CLT

void *_jn_clt_callback_thread(void *args)
{

	jn_conn *conn = (jn_conn *) args;
	jn_pkt *pkt, *err_pkt;
	jn_cb_entry *q;
	int ret;
	int found;
	jthread_t cbthid;
	void (*progress_cb) (jn_cb_entry *);
	uchar buf[JN_MAX_PKT];
	unsigned long imode;
	jn_pkt_list *plist, *entry, *delete;
	uint16_t npkt, cnt;

	conn->flags |= JN_CF_BLOCK_TIME;

	_wdeb5(L"CB thread started for < %S >", conn->username);

	for (;;) {

		err_pkt = NULL;
		found = 0;
 aloc_again2:
		pkt = (jn_pkt *) malloc(sizeof(jn_pkt));
		pkt->hdr.data_len = 0L;

		if (!pkt) {
			found++;
			sleep(found);
			if (found >= JN_MAX_MALOC_RETRIES) {
				_jn_clt_handle_err(conn, NULL, -JE_DC);
			}
			goto aloc_again2;
		}

 recv_again:
		ret = recv(conn->sockfd, buf, JN_MAX_PKT, 0);
		if (ret <= 0) {
			if (errno == EINTR)
				goto recv_again;
			else if (errno == EAGAIN) {
				imode = JN_MODE_BLOCK;
				_jn_block_mode(conn->sockfd, &imode);
				goto recv_again;
			} else {
				//disconnected
				_jn_clt_handle_err(conn, NULL, -JE_DC);
				if (pkt)
					free(pkt);
				_jn_conn_destroy(conn);
				return NULL;
			}
		} else {

			ret = _jn_conn_proc_buf(conn, &plist, &npkt, buf, ret);
			if (ret) {
				_jn_clt_handle_err(conn, NULL, ret);
				goto recv_again;
			}
			//_wdeb(L"npkt = %u", npkt);
			entry = plist;

			for (cnt = 0; cnt < npkt; cnt++) {
				//_wdeb(L"cnt = %i", cnt);
				_wdeb5
				    (L"CB searching for pkt with tid = %u, comid = %u",
				     entry->pkt->hdr.trans_id,
				     entry->pkt->hdr.comod_id);

				q = _jn_cb_fifo_search(conn->cbfifo,
						       entry->pkt);

				if (!q) {
					_wdeb5(L"CB q not found");
					_jn_transa_rm(conn->trans_a,
						      entry->pkt->hdr.trans_id);
					_jn_clt_handle_err(conn, entry->pkt,
							   -JE_NOTFOUND);
				} else {

					progress_cb = q->progress_callback;
					q->conn = conn;
					q->pkt = entry->pkt;
					q->jerr = 0;

					if (q->flags & JN_CB_THREAD &
					    !(JN_CB_CONTINUE & q->flags)) {
						_wdeb
						    (L"WARNING! NOT COMPLETE YET!")
						    if (_create_thread
							(&cbthid, NULL,
							 (void*)progress_cb,
							 (void *)q))
							goto call_non_thread;

					} else {
 call_non_thread:
						progress_cb(q);
						/*
						   if(  (!(q->flags & JN_CB_KEEP)) &&
						   (!(q->flags & JN_CB_CONTINUE)) &&
						   (!(q->flags & JN_CB_NOFREE))
						   ){
						   free(q);
						   }
						 */
					}

				}
				delete = entry;
				entry = entry->next;
				free(delete);
			}
		}
	}

	return NULL;

}

#endif

int jn_reg_cb(jn_conn * conn, jn_pkt * pkt, jn_cb_entry * newq)
{

	int ret;
	jn_cb_entry *q, *prev, *delete;
	struct timeval tv;

#ifndef NDEBUG
	uint16_t cnt = 0;

	if (newq->flags & JN_CB_KEEP) {
		_wdeb4(L"has KEEP flags");
	}
	if (newq->flags & JN_CB_CONTINUE) {
		_wdeb4(L"has CONTINUE flags");
	}
#endif

	_wdeb4(L"registering tid = %u, comid = %u", newq->tid, newq->comid);

	_jn_dump_cbfifo(conn->cbfifo);

#ifndef JN_SINGLE_CLT
	_lock_mx(&conn->cbfifo->mx);
#endif

	if (conn->cbfifo->cnt >= conn->cbq_gc_trigger) {
		_wdeb(L"entering garbage collector");
		gettimeofday(&tv, NULL);
		q = conn->cbfifo->first;
		prev = q;
		while (q) {

			if ((tv.tv_sec - q->reg_time) > conn->cbq_gc_to) {
				if (q == conn->cbfifo->first) {
					conn->cbfifo->first =
					    conn->cbfifo->first->next;
				} else if (q == conn->cbfifo->last) {
					conn->cbfifo->last = prev;
				} else {
					prev->next = q->next;
				}
				conn->cbfifo->cnt--;

				delete = q;
				q = q->next;
				free(delete);
			} else {
				prev = q;
				q = q->next;
			}

		}
	}

	q = conn->cbfifo->first;
	while (q) {
		_wdeb4(L"q->tid = %u", q->tid);
		if ((newq->flags & JN_CB_TID) && (newq->tid == q->tid)) {
#ifndef JN_SINGLE_CLT
			_unlock_mx(&conn->cbfifo->mx);

#endif
			_wdeb4(L"exists @ %u, tid = %u, newtid = %u", cnt,
			       q->tid, newq->tid);

			_jn_dump_cbfifo(conn->cbfifo);

			return -JE_EXISTS;
		} else if ((newq->flags & JN_CB_COMID)
			   && (newq->comid == q->comid)) {
#ifndef JN_SINGLE_CLT
			_unlock_mx(&conn->cbfifo->mx);

#endif
			_wdeb4(L"exists @ %u, comid = %u, newcomid = %u", cnt,
			       q->comid, newq->comid);

			_jn_dump_cbfifo(conn->cbfifo);

			return -JE_EXISTS;
		}
		q = q->next;
#ifndef NDEBUG
		cnt++;
#endif
	}

	gettimeofday(&tv, NULL);
	newq->reg_time = tv.tv_sec;
	newq->next = NULL;

	if (!conn->cbfifo->first) {
		_wdeb4(L"registering as first");
		conn->cbfifo->first = newq;
		conn->cbfifo->last = conn->cbfifo->first;
	} else {
		_wdeb4(L"registering as last, current last->tid = %u",
		       conn->cbfifo->last->tid);
		conn->cbfifo->last->next = newq;
		conn->cbfifo->last = conn->cbfifo->last->next;
	}
	conn->cbfifo->cnt++;

#ifndef JN_SINGLE_CLT
	_unlock_mx(&conn->cbfifo->mx);
#endif

	if (pkt) {
		if ((ret = jn_send(conn, pkt)) < 0)
			return ret;
	}

	return 0;

}

void _jn_clt_sigchld_handler(int s)
{
	while (wait(NULL) > 0) ;
}

int jn_init_clt(jn_h * h, jn_conn * conn)
{
	int ret = 0;
#ifndef _WIN32	
	struct sigaction sa;
#endif	

	if (h->magic == JN_MAGIC)
		return -JE_ALREADYINIT;

	h->flags = 0;
	h->sockfd = -1;
	//h->conn_list = NULL;

	jn_default_conf(&h->conf);

#ifndef _WIN32
	sa.sa_handler = _jn_clt_sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		return -1;
	}
#endif
	h->magic = JN_MAGIC;

	return ret;
}

void jn_shutdown_clt(jn_h * h)
{

	if (h->magic != JN_MAGIC)
		return;

	close(h->sockfd);

	h->magic = 0x0000;
}

int jn_send_poll(jn_conn * conn, jn_pkt * pkt, uchar flags)
{
	return -JE_IMPLEMENT;
}
