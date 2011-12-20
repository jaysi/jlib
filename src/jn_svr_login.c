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
#include "unistd.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/in.h"
#include <netdb.h>
#else
#include "winsock.h"
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
#include "sys/wait.h"
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define _deb1(f, a...)
#define _deb2(f, a...)
#define _wdeb1 _wdeb		//old, hs, login
#define _deb3(f, a...)
#define _wdeb3(f, a...)
#define _wdeb4 _wdeb		//com request
#define _wdeb5 _wdeb		//accept, fw test
#define _wdeb6(f, a...)		//new packet queue system
#define _wdeb7(f, a...)		//com open debug
#define _wdeb8(f, a...)		//packet queue
#define _wdeb9(f, a...)		//dumps plain packet contents in jcs
#define _wdeb10 _wdeb		//new conn proc buf code
#define _wdeb11 _wdeb		//qpoolhead

int _jn_handshake(jn_h * h, jn_conn * conn)
{
	jn_hs_pkt pkt;
	jn_hs_reply_pkt rpkt;
	uint32_t len = sizeof(jn_hs_pkt);
	struct timeval tv;
	uchar pwhash[32];

	_wdeb1(L"h = %p, conn = %p", h, conn);

/*
	_wdeb1(L"checking block list...");
	if(!jn_block_chk(h, NULL, conn->addr)){
#ifndef WIN32		
		close(conn->sockfd);
#else
		close_socket(conn->sockfd);
#endif
		_wdeb(L"ip: '%s' blocked", );
		return -JE_FORBID;
	}
*/

	tv = h->conf.hs_tv;

	_wdeb1(L"WARNING: take care of pubkey len!");

	_wdeb1(L"waiting for handshake packet...");

	if (recv_all(conn->sockfd, (uchar *) & pkt, &len, &tv) < 0)
		return -JE_RECV;

	_wdeb1(L"h = %p, conn = %p", h, conn);

	/*
	   if(pkt.ver > JN_VER){
	   _wdeb1(L"bad version");
	   memset(&rpkt, '\0', sizeof(jn_hs_reply_pkt));
	   len = sizeof(jn_hs_reply_pkt);
	   tv = h->conf.hs_tv;
	   send_all(conn->sockfd, (char*)&rpkt, &len, &tv);
	   #ifndef WIN32                
	   close(conn->sockfd);
	   #else
	   close_socket(conn->sockfd);
	   #endif
	   return -JE_VER;
	   }
	 */

	_wdeb1(L"got packet, validating public key, REMOVED");

#ifndef NDEBUG
	wprintf(L"Px: %s\n", pkt.px);
	wprintf(L"Py: %s\n", pkt.py);
#endif
	/*
	   if(ECIES_public_key_validation(pkt.px, pkt.py) < 0){
	   _wdeb1(L"Bad public key");
	   memset(&rpkt, '\0', sizeof(jn_hs_reply_pkt));
	   strcpy(rpkt.ekey, "##Invalid Pub Key##");
	   len = sizeof(jn_hs_reply_pkt);
	   tv = h->conf.hs_tv;
	   send_all(conn->sockfd, (char*)&rpkt, &len, &tv);
	   close(conn->sockfd);
	   return -JE_BADPUBKEY;
	   }
	 */
	_wdeb1(L"h = %p, conn = %p", h, conn);
	_wdeb1(L"valid, creating random aes key");
	//conn->key = (uchar*)malloc(JN_CONF_KEY_LEN);
	if (jrand_buf(conn->key, JN_CONF_KEY_LEN) < 0)
		return -JE_OPEN;
	//memset(conn->key, 'a', JN_CONF_KEY_LEN);

	_wdeb1(L"h = %p, conn = %p", h, conn);

	_wdeb1(L"encrypting aes key");
	ECIES_encryption(rpkt.ekey, (char *)conn->key, JN_CONF_KEY_LEN, pkt.px,
			 pkt.py);

	//accepted
	len = sizeof(jn_hs_reply_pkt);
	_wdeb1(L"sending reply packet, reply size is %u", len);
	_wdeb1(L"h = %p, conn = %p", h, conn);
#if (defined JN_BIG_ENDIAN == 1)
	_wdeb(L"BIG ENDIAN");
	rpkt.flags |= JN_HS_BIG_ENDIAN;
#else
	_wdeb(L"LITTLE ENDIAN");
	rpkt.flags &= ~JN_HS_BIG_ENDIAN;
#endif
	if (send_all(conn->sockfd, (uchar *) & rpkt, &len, &tv) < 0) {
		_wdeb1(L"failed to send answer");
#ifndef WIN32
		close(conn->sockfd);
#else
		close_socket(conn->sockfd);
#endif
		return -JE_SEND;
	}

	_wdeb1(L"sent, setting connection aes key");

//      conn->aes = (aes_context*)malloc(sizeof(aes_context));
	sha256(conn->key, JN_CONF_KEY_LEN, pwhash);
	aes_set_key(&conn->aes, pwhash, 256);

	_wdeb1(L"done");
	return 0;
}

int _jn_login(jn_h * h, jn_conn * conn)
{
	int ret;
	jn_login_payload lin;
	jn_pkt pkt;
	//jn_conn* entry = NULL;
	wchar_t uname[MAX_UNAME];
	wchar_t pass[MAX_PASS];
	jac_urec urec;
	int old_timeout;
	uchar otp;

#ifndef NDEBUG
	jmxattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif

	_wdeb1(L"called");

	if (h->magic != JN_MAGIC)
		return -JE_NOINIT;
	//if(!(h->jac)) return -JE_NOJAC;

	_wdeb1(L"passed");

	//will stay for the rest of the function

	conn->flags &= ~JN_CF_SVR;	//force jn_send_plain() to use simple jn_send()

	if ((ret = jn_recv(conn, &pkt)) < 0) {
		_wdeb(L"failed here, ret = %i", ret);
		goto end;
	}

	_wdeb1(L"got login");

	if (pkt.hdr.type != JNT_LOGIN)
		return -JE_TYPE;
	if (pkt.hdr.data_len != sizeof(jn_login_payload)) {
#ifndef NO_HEAP
		if (pkt.hdr.data_len)
			free(pkt.data);
#endif
		return -JE_SIZE;
	}

	_wdeb1(L"passed");

	memcpy(lin.username, pkt.data, MAX_UNAME);
	lin.username[MAX_UNAME] = '\0';
	_wdeb1(L"username jcs: %s, CWBYTES: %i", lin.username,
	       CWBYTES(lin.username));
	if (jcstow(lin.username, uname, MAX_UNAME) == (size_t) - 1) {
		ret = -JE_INV;
		goto end;
	}

	memcpy(lin.pass, pkt.data + MAX_UNAME, MAX_PASS);
	lin.pass[MAX_PASS] = '\0';
	_wdeb1(L"password jcs: %s, CWBYTES: %i", lin.pass, CWBYTES(lin.pass));
	if (jcstow(lin.pass, pass, MAX_PASS) == (size_t) - 1) {
		ret = -JE_INV;
		_wdeb1(L"fail");
		goto end;
	}
//      if((ret = _check_name(uname)) < 0)
//              goto end;

	_wdeb1(L"trying login (u:p): %S : %S", uname, pass);

	_wdeb1(L"passed");

/*
	if(!jn_block_chk(h, uname, NULL)){
		ret = -JE_FORBID;
		goto end;
	}
*/

//      if(h->flags&JNF_DO_LOGIN){

	_wdeb1(L"jac_login()");

	_lock_mx(&h->jac_mx);

	_wdeb1(L"checking permissions for ( user ): %S to ( svr ): %S",
	       h->svr_obj_name, uname);

	//check permissions
	if (!jac_find_object_dep
	    (&h->jac, h->svr_obj_name, uname, JAC_USER, &otp)) {

		if ((otp & JOBJ_OWNER) || (otp & JPERM_EX) || (otp & JPERM_RD)) {

		} else {
			ret = -JE_NOPERM;
			_wdeb(L"fails here");
			goto end;
		}
	} else {
		ret = -JE_NOPERM;
		_wdeb(L"fails here");
		goto end;

	}

	if ((ret = jac_login(&h->jac, uname, pass)) < 0) {
		_wdeb(L"could not login %S, ret = %i", uname, ret);
		_unlock_mx(&h->jac_mx);
		goto end;
	}
	if ((ret = jac_find_user(&h->jac, uname, &urec)) < 0) {
		_wdeb(L"could not find user record %S, ret = %i", uname, ret);
		jac_logout(&h->jac, uname);
		_unlock_mx(&h->jac_mx);
		goto end;
	}
	_unlock_mx(&h->jac_mx);

	_wdeb1(L"passed");

/*		
	} else {
		if(!_jn_find_conn(h, uname, &entry)){
			_wdeb(L"failed here");
			jn_send_plain(conn, JNT_FAIL, NULL, -JE_EXISTS);
			ret = -JE_EXISTS;
			goto end;		
		}
	}
*/
	_wdeb1(L"sending ok");

	if ((ret = jn_send_plain(conn, 0, JNT_OK, 0, NULL)) < 0) {
		_wdeb1(L"failing here");
		goto end;
	}

	_wdeb1(L"passed");
 end:

	if (!ret) {

		conn->flags |= JN_CF_SVR;
		conn->username = (wchar_t *) malloc(WBYTES(uname));
		if (!conn->username)
			return -JE_MALOC;
		memcpy(conn->username, uname, WBYTES(uname));
		wtojcs(pass, conn->key, 0);
		conn->uid = urec.uid;

		//will be decreased later at close call
		//_wdeb3(L"logged in");
		//_jn_conn_req_inc(h, conn);

	} else {
		_wdeb3(L"sending fail msg");
		conn->flags &= ~JN_CF_SVR;
		jn_send_plain(conn, 0, JNT_FAIL, ret, NULL);
		close(conn->sockfd);
	}

	_wdeb5(L"ret = %i", ret);
	return ret;

}

int jn_accept(jn_h * h, jn_conn * conn)
{
	int ret;
	socklen_t len;
	//char s[INET6_ADDRSTRLEN];

	if (h->magic != JN_MAGIC)
		return -JE_NOINIT;

	len = sizeof(struct sockaddr);
 retry:
	_wdeb5(L"waiting...");
	if ((conn->sockfd =
	     accept(h->sockfd, (struct sockaddr *)&conn->addr, &len)) == -1) {
		_wdeb5(L"accept() failed %i ...", conn->sockfd);
		switch (errno) {
		case EAGAIN:
			goto retry;

		case ENETDOWN:
		case EPROTO:
		case ENOPROTOOPT:
		case EHOSTDOWN:
		case ENONET:
		case EHOSTUNREACH:
		case EOPNOTSUPP:
		case ENETUNREACH:
			h->syse = errno;
			return -JE_SYSE;

		default:
			return -JE_ACCEPT;
		}
	}
	_wdeb5(L"accept() returns sockfd = %i ...", conn->sockfd);

	if (h->nusers >= h->conf.max_users) {
		close(conn->sockfd);
		_wdeb("ERROR: user limit reached");
		return -JE_LIMIT;
	}

	if (!(ret = _jn_handshake(h, conn)))
		ret = _jn_login(h, conn);

	if (!ret)
		h->nusers++;

	_wdeb5(L"ret = %i", ret);

	return ret;
}

void *_jn_accept_thread(void *arg)
{
//      th_arg* ua = (th_arg*)arg;
	jn_h *h = (jn_h *) arg;
	jn_conn conn;
	jn_poll_buck *buck;
	jthread_t thid;
	th_arg tharg;
	tharg.h = h;
	int cnt;
	int ret;
	jn_ppkt ppkt;

	_wdeb1(L"accept thread started");

	cnt = 0;

	for (;;) {
 do_wait:
		_wdeb1(L"waiting...");
		ret = jn_accept(h, &conn);
		if (!ret) {	//add it to a poll thread

			_wdeb1(L"got something %S ...", conn.username);

 poll_fd_set:
			_lock_mx(&h->poll_list->mx);
			for (buck = h->poll_list->first; buck;
			     buck = buck->next) {
				_wdeb3(L"comparing %u, %u", buck->nfds,
				       h->conf.nfds_buck);
				if (buck->nfds < h->conf.nfds_buck) {
					_wdeb1(L"found empty buck");
					_jn_add_poll_fd(h, buck, &conn);
					_unlock_mx(&h->poll_list->mx);
					goto do_wait;
				}
			}
			_unlock_mx(&h->poll_list->mx);
			if (!buck) {
				_wdeb1(L"allocating bucket");
				buck = _jn_alloc_poll_buck(h);
				_wdeb1(L"done");
				if (buck) {
					_wdeb1(L"creating new buck thread");

					tharg.buck = buck;

					if (!_create_thread
					    (&thid, NULL, _jn_poll_thread,
					     (void *)&tharg)) {
						_jn_read_pipe(buck->fds[0].fd,
							      &ppkt);
						goto poll_fd_set;
					}
				}
			}
		} else if (ret == -JE_SYSE || ret == -JE_ACCEPT) {
			return NULL;
		}
	}

	return NULL;
}
