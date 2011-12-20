#include "jnet.h"
#include "jnet_internals.h"
#include "jer.h"
#include "ecc.h"
#include "jac.h"
#include "debug.h"
#include "jhash.h"
#include "jcs.h"
#include "jrand.h"

#ifndef __WIN32
#include "unistd.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/in.h"
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
#include "sys/wait.h"

#define _deb1(f, a...)
#define _wdeb1(f, a...)
#define _wdeb2 _wdeb		//connection removal
#define _wdeb3 _wdeb		//com req
#define _wdeb4(f, a...)		//dumps plain data
#define _wdeb5 _wdeb		//comod close
#define _wdeb6 _wdeb		//com req cycle
#define _wdeb7 _wdeb		//bcast

jmodid_t _jn_get_free_com_id(jn_h * h)
{
	jmodid_t id;
	jn_comod_entry *entry;
	assert(h->com_list);
 get_id:
	if (jrand_buf((uchar *) & id, sizeof(jmodid_t)))
		return 0;

	_lock_mx(&h->com_list->mx);
	for (entry = h->com_list->first; entry; entry = entry->next) {
		if (entry->com_id == id) {
			_unlock_mx(&h->com_list->mx);
			goto get_id;
		}
	}
	_unlock_mx(&h->com_list->mx);

	return id;
}

jn_cc_list *_jn_cclist_new()
{
	jn_cc_list *list;

#ifndef NDEBUG
	jmxattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif
	list = (jn_cc_list *) malloc(sizeof(jn_cc_list));
	if (!list)
		return NULL;
	list->first = NULL;
	list->cnt = 0L;

#ifndef NDEBUG
	_init_mx(&list->mx, &attr);
#else
	_init_mx(&list->mx, NULL);
#endif

	return list;
}

int _jn_cclist_add(jn_cc_list * list, jn_conn * conn)
{

	jn_cc_entry *entry = (jn_cc_entry *) malloc(sizeof(jn_cc_entry));
	if (!entry)
		return -JE_MALOC;
	entry->next = NULL;

	entry->conn = conn;

	_lock_mx(&list->mx);

	if (!list->first) {
		list->first = entry;
		list->last = entry;
		list->cnt = 1L;
	} else {
		list->last->next = entry;
		list->last = list->last->next;
		list->cnt++;
	}

	_unlock_mx(&list->mx);

	return 0;
}

void _jn_cclist_destroy(jn_cc_list * list)
{
	jn_cc_entry *entry;
	_lock_mx(&list->mx);
	while (list->first) {
		entry = list->first;
		list->first = list->first->next;
		free(entry);
	}
	_unlock_mx(&list->mx);
	_destroy_mx(&list->mx);
	free(list);
}

jn_comod_list *_jn_comod_list_new()
{
	jn_comod_list *list;

#ifndef NDEBUG
	jmxattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif

	list = (jn_comod_list *) malloc(sizeof(jn_comod_list));
	if (!list)
		return NULL;

	list->first = NULL;

#ifndef NDEBUG
	_init_mx(&list->mx, &attr);
#else
	_init_mx(&list->mx, NULL);
#endif

	return list;
}

//comod list functions - BEGIN

int _jn_create_comlist(jn_h * h, uchar ** buf, uint32_t * bufsize)
{
	uint32_t pos;
	jn_comod_entry *entry;
	int ret = 0;

	//lock mutex by hand (h->com_list->mx);

	if (h->com_list->cnt == 0) {
		ret = -JE_EMPTY;
		goto end;
	}

	*bufsize =
	    (h->com_list->cnt * sizeof(jmodid_t)) +
	    (h->com_list->cnt * sizeof(jobjid_t));
	for (entry = h->com_list->first; entry; entry = entry->next) {
		if ((pos =
		     wtojcs_len(entry->copyname,
				JN_MAX_COM_NAME)) == ((size_t) - 1))
			return -JE_INV;
		pos++;		//1 for null termination
		if (pos == ((size_t) - 1)) {
			ret = -JE_INV;
			goto end;
		}
		*bufsize += pos;
	}
	*buf = (uchar *) malloc(*bufsize);
	pos = 0;
	for (entry = h->com_list->first; entry; entry = entry->next) {
		packi32((*buf) + pos, entry->objid);
		pos += sizeof(jobjid_t);
		packi16((*buf) + pos, entry->com_id);
		pos += sizeof(jmodid_t);
		pos += wtojcs(entry->copyname, (*buf) + pos, 0);
		pos++;		//for null termination.
	}
 end:
	return ret;
}

int jn_send_comlist(jn_h * h, jn_conn * conn, jtransid_t req_transid)
{
	int ret;
	uchar *buf;
	uint32_t bufsize;
	jmodid_t ncoms;
	jn_pkt pkt;
	uint16_t pos;
	uint16_t to_send;

//      if((ret = _jn_create_comlist(h, &buf, &bufsize, &ncoms)) < 0)
//              return ret;

	_lock_mx(&h->comlist_buf_mx);

	if (h->comlist_bufsize == 0) {
		_unlock_mx(&h->comlist_buf_mx);
		jn_send_plain(conn, req_transid, JNT_FAIL, -JE_EMPTY, NULL);
		return 0;
	}

	bufsize = h->comlist_bufsize;

	_lock_mx(&h->com_list->mx);
	ncoms = h->com_list->cnt;
	_unlock_mx(&h->com_list->mx);

	buf = (uchar *) malloc(enc_data_size(bufsize));
	memcpy(buf, h->comlist_buf, bufsize);

	_unlock_mx(&h->comlist_buf_mx);

	pkt.hdr.trans_id = req_transid;
	pkt.hdr.type = JNT_OK;
	pkt.hdr.flags = 0x00;

	if (bufsize > JN_MAX_PKT_DATA) {
		to_send = JN_MAX_PKT_DATA;
		pkt.hdr.rs = bufsize - to_send;
	} else {
		to_send = bufsize;
		pkt.hdr.rs = 0L;
	}

	pos = 0;
	while (to_send) {
		//malloc
		pkt.hdr.data_len = to_send;
		pkt.data = buf + pos;
		pos += to_send;

		//send async
		if ((ret = jn_send_poll(conn, &pkt, 0)) < 0) {
			free(buf);
			return ret;
		}
		//calc to_send  & rs    
		if (pkt.hdr.rs > JN_MAX_PKT_DATA) {
			to_send = JN_MAX_PKT_DATA;
			pkt.hdr.rs -= JN_MAX_PKT_DATA;
		} else {
			to_send = pkt.hdr.rs;
			pkt.hdr.rs = 0L;
		}
	}

	free(buf);

	_wdeb6(L"comlist sent to < %S >", conn->username);

	return 0;
}

//comod list functions - END

//comod creation, removing functions - BEGIN

int _jn_create_com(jn_h * h, jn_conn * conn, jn_pkt * req_pkt)
{
//      jn_open_com_req* req = (jn_open_com_req*)req_pkt->data;
	jn_pkt pkt;
	jn_comod_entry *entry;
	//uint16_t *com_ids;
	wchar_t *copyname;
	int ret;
	th_arg *arg;
	jobjid_t objid;
	uchar flags;
	jn_ucom_entry *uentry;
#ifndef NDEBUG
	jmxattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif

	assert(conn->ucom_list);

	//_lock_mx(&conn->mx);
	if (conn->ucom_list->cnt >= h->conf.max_ucom) {
		//_unlock_mx(&conn->mx);
		_wdeb1(L"h->com_list->cnt (%u) > h->conf.max_com (%u)",
		       h->com_list->cnt, h->conf.max_com);
		jn_send_plain(conn, req_pkt->hdr.trans_id, JNT_FAIL, -JE_LIMIT,
			      NULL);
		return -JE_LIMIT;
	}
	//_unlock_mx(&conn->mx);

	if ((ret =
	     _jn_disassm_com_open(req_pkt, &flags, &objid, &copyname)) < 0) {
		_wdeb1(L"failed here, ret = %i, TID = %u", ret,
		       req_pkt->hdr.trans_id);
		jn_send_plain(conn, req_pkt->hdr.trans_id, JNT_FAIL, -JE_INV,
			      NULL);
		return ret;
	}

	_lock_mx(&h->com_list->mx);
	if (h->com_list->cnt >= h->conf.max_com) {
		_unlock_mx(&h->com_list->mx);
		_wdeb1(L"h->com_list->cnt (%u) > h->conf.max_com (%u)",
		       h->com_list->cnt, h->conf.max_com);
		jn_send_plain(conn, req_pkt->hdr.trans_id, JNT_FAIL, -JE_LIMIT,
			      NULL);
		return -JE_LIMIT;
	}
	_wdeb1(L"openning %u, %S", objid, copyname);
	for (entry = h->com_list->first; entry; entry = entry->next) {
		if (!wcscmp(copyname, entry->copyname)) {
			_unlock_mx(&h->com_list->mx);
			_wdeb1(L"already exists");
			jn_send_plain(conn, req_pkt->hdr.trans_id, JNT_FAIL,
				      -JE_EXISTS, NULL);
			return -JE_EXISTS;
		}
	}
	_unlock_mx(&h->com_list->mx);

	_wdeb1(L"Loaing comod");

	if ((ret = _jn_com_load(h, objid, copyname, &entry)) < 0) {
		_wdeb1(L"jn_com_load() fail code: %i", ret);
		jn_send_plain(conn, req_pkt->hdr.trans_id, JNT_FAIL, ret, NULL);
		return ret;
	}

	entry->next = NULL;
	entry->flags = flags;
	entry->objid = objid;
	entry->owner_id = conn->uid;
	entry->bytes_sent = 0L;
	entry->bytes_rcvd = 0L;
	entry->copyname = copyname;
	entry->maxreq = h->conf.maxcomreq;
	entry->nreq = 0L;
	_init_mx(&entry->somx, NULL);

	entry->com_id = _jn_get_free_com_id(h);

	_wdeb1(L"initializing comod");

	//init mod
	_lock_mx(&entry->somx);
	if ((ret =
	     entry->jnmod_init((void *)h, (void *)entry, (void *)conn)) < 0) {
		_unlock_mx(&entry->somx);
		free(entry->copyname);
		free(entry);
		_destroy_mx(&entry->somx);
		_wdeb1(L"failing here, ret = %i, TID = %u", ret,
		       req_pkt->hdr.trans_id);
		return ret;
	}
	_unlock_mx(&entry->somx);

	//create thread
	//init

	_wdeb1(L"creating fifo for thread");

	entry->fifo = _jn_fifo_new();
	if (!entry->fifo) {
		free(entry->copyname);
		free(entry);
		_destroy_mx(&entry->somx);
		return -JE_MALOC;
	}

	entry->reply_fifo = _jn_fifo_new();
	if (!entry->fifo) {
		_jn_fifo_destroy(entry->fifo);
		free(entry->copyname);
		free(entry);
		_destroy_mx(&entry->somx);
		_wdeb1(L"failed here");
		return -JE_MALOC;
	}

	_wdeb1(L"creating com thread");

	arg = (th_arg *) malloc(sizeof(th_arg));

	arg->h = h;
	arg->conn = conn;
	arg->com = entry;
	if (_create_thread(&(entry->th_id), NULL, _jn_comod_thread, (void *)arg)
	    < 0) {
		jn_send_plain(conn, req_pkt->hdr.trans_id, JNT_FAIL, -JE_SYSE,
			      NULL);
		_jn_fifo_destroy(entry->fifo);
		_jn_fifo_destroy(entry->reply_fifo);
		free(entry->copyname);
		free(entry);
		free(arg);
		_destroy_mx(&entry->somx);
		_wdeb1(L"failed to create thread");
		return -JE_SYSE;
	}

	_wdeb1(L"creating reply thread");

	if (_create_thread
	    (&(entry->th_id), NULL, _jn_comod_rthread, (void *)arg) < 0) {
		jn_send_plain(conn, req_pkt->hdr.trans_id, JNT_FAIL, -JE_SYSE,
			      NULL);
		_cancel_thread(entry->th_id);
		_jn_fifo_destroy(entry->fifo);
		_jn_fifo_destroy(entry->reply_fifo);
		free(entry->copyname);
		free(entry);
		free(arg);
		_destroy_mx(&entry->somx);
		_wdeb1(L"failed to create reply thread");
		return -JE_SYSE;
	}

	_wdeb1(L"adding to com list");

	uentry = (jn_ucom_entry *) malloc(sizeof(jn_ucom_entry));
	uentry->next = NULL;

	_lock_mx(&h->com_list->mx);

	if (!h->com_list->first) {
		h->com_list->first = entry;
		h->com_list->last = entry;
	} else {
		h->com_list->last->next = entry;
		h->com_list->last = h->com_list->last->next;
	}
	h->com_list->cnt++;

	uentry->mod_id = entry->com_id;

	_unlock_mx(&h->com_list->mx);

	_wdeb1(L"adding to comlist buffer");

	_lock_mx(&h->comlist_buf_mx);
	if (h->comlist_bufsize)
		free(h->comlist_buf);
	if (_jn_create_comlist(h, &h->comlist_buf, &h->comlist_bufsize) < 0) {
		//buffer not allocated in case of error
		h->comlist_bufsize = 0L;
	}
	_unlock_mx(&h->comlist_buf_mx);

	/* add in join  
	   uentry->objid = objid;
	   uentry->flags = JN_UCF_OWNER;
	   uentry->copyname = (wchar_t*)malloc(WBYTES(copyname));
	   wcscpy(uentry->copyname, copyname);

	   _wdeb1(L"adding to ucom list");

	   _lock_mx(&conn->ucom_list->mx);
	   if(!conn->ucom_list->first){
	   conn->ucom_list->first = uentry;
	   conn->ucom_list->last = uentry;
	   } else {
	   conn->ucom_list->last->next = uentry;
	   conn->ucom_list->last = conn->ucom_list->last->next;
	   }    
	   conn->ucom_list->cnt++;
	   _unlock_mx(&conn->ucom_list->mx);    
	 */
	_wdeb1(L"sending OK");

	pkt.hdr.type = JNT_OK;
	pkt.hdr.flags = 0x00;
	pkt.hdr.trans_id = req_pkt->hdr.trans_id;
	pkt.hdr.comod_id = entry->com_id;
	pkt.hdr.data_len = 0L;
	pkt.hdr.rs = 0L;

	//_jn_conn_req_inc(conn); on join?

	jn_send_poll(conn, &pkt, 0);

	_wdeb6(L"created comod %S, TID: %u by request from < %S >",
	       copyname, pkt.hdr.trans_id, conn->username);

	return 0;
}

int _jn_leave_com(jn_h * h, jn_conn * conn, jn_pkt * pkt)
{

	jn_ucom_entry *ucentry, *prev;
	jn_comod_entry *comentry;
	jn_fifo_entry *job;
	int ret = 0;

	//remove from ucom list
	//_lock_mx(&conn->mx);
	for (ucentry = conn->ucom_list->first; ucentry; ucentry = ucentry->next) {
		if (ucentry->mod_id == pkt->hdr.comod_id) {
			if (ucentry == conn->ucom_list->first) {
				conn->ucom_list->first =
				    conn->ucom_list->first->next;
			} else {
				if (ucentry == conn->ucom_list->last) {
					conn->ucom_list->last = prev;
				}
				prev->next = ucentry->next;
			}
			conn->ucom_list->cnt--;
			break;
		} else {
			prev = ucentry;
		}
	}

	if (!ucentry)
		ret = -JE_NOTFOUND;

	//_unlock_mx(&conn->mx);
	if (ret) {
		jn_send_plain(conn, pkt->hdr.trans_id, JNT_FAIL, ret, NULL);
		return ret;
	}

	job = (jn_fifo_entry *) malloc(sizeof(jn_fifo_entry));

	if (!job) {
		jn_send_plain(conn, pkt->hdr.trans_id, JNT_FAIL, -JE_SYSE,
			      NULL);
		return -JE_MALOC;
	}

	job->next = NULL;
	job->type = JN_FT_RM_CONN;
	job->uid = JACID_INVAL;
	job->conn = conn;

	_lock_mx(&h->com_list->mx);
	for (comentry = h->com_list->first; comentry; comentry = comentry->next) {

		if (comentry->com_id == pkt->hdr.comod_id) {
			//submit RM_CONN queue
			//_jn_com_req_inc(comentry); not needed
			_jn_fifo_enq(comentry->fifo, job);
			break;
		}

	}
	_unlock_mx(&h->com_list->mx);

	if (!comentry) {
		return -JE_NOTFOUND;
	}

	jn_send_plain(conn, pkt->hdr.trans_id, JNT_OK, 0, NULL);

	_jn_conn_req_dec(conn);

	_wdeb(L"user < %S > has left comod < %S >", conn->username,
	      comentry->copyname);

	return 0;

}

int _jn_join_com(jn_h * h, jn_conn * conn, jn_pkt * pkt)
{
	jn_comod_entry *entry;
	uchar flags;
	jmodid_t mod_id;
	jn_cc_entry *cc_entry = NULL;
	jn_ucom_entry *uentry = NULL;

	_wdeb1(L"joining com... trans_id = %u, username = %S",
	       pkt->hdr.trans_id, conn->username);

	//_lock_mx(&conn->mx);
	if (conn->ucom_list->cnt >= h->conf.max_ucom) {
		//_unlock_mx(&conn->mx);
		_wdeb(L"h->com_list->cnt (%u) > h->conf.max_ucom (%u)",
		      h->com_list->cnt, h->conf.max_ucom);
		jn_send_plain(conn, pkt->hdr.trans_id, JNT_FAIL, -JE_LIMIT,
			      NULL);
		return -JE_LIMIT;
	}
	////_unlock_mx(&conn->mx);      

	//_jn_conn_req_inc(conn);//for comod
	//conn->nreq++; //conn->mx already locked
	_jn_conn_req_inc(conn);

	mod_id = pkt->hdr.comod_id;

	_wdeb1(L"got join request, tid = %u", pkt->hdr.trans_id);

	////_lock_mx(&conn->mx);
	for (uentry = conn->ucom_list->first; uentry; uentry = uentry->next) {
		if (uentry->mod_id == mod_id) {
			//conn->nreq--;//for comod
			_jn_conn_req_dec(conn);
			//_unlock_mx(&conn->mx);
			_wdeb1(L"already exists");
			jn_send_plain(conn, pkt->hdr.trans_id, JNT_FAIL,
				      -JE_EXISTS, NULL);

			//_jn_conn_req_dec(conn);//for comod

			return -JE_EXISTS;
		}
	}

	_lock_mx(&h->com_list->mx);
	for (entry = h->com_list->first; entry; entry = entry->next) {
		if (entry->com_id == mod_id) {
			if (entry->flags & JN_COM_PAUSED) {
				//conn->nreq--;//for comod
				_jn_conn_req_dec(conn);
				//_unlock_mx(&conn->mx);
				_unlock_mx(&h->com_list->mx);
				jn_send_plain(conn, pkt->hdr.trans_id, JNT_FAIL,
					      -JE_PAUSE, NULL);

				//_jn_conn_req_dec(conn);//for comod

				return -JE_PAUSE;
			}	//else {
			//_jn_com_req_inc(entry);
//                      }
			break;
		}
	}
	_unlock_mx(&h->com_list->mx);

	if (!entry) {
		_wdeb1(L"not found! sending failure reply tid = %u",
		       pkt->hdr.trans_id);
		jn_send_plain(conn, pkt->hdr.trans_id, JNT_FAIL, -JE_NOTFOUND,
			      NULL);
		//conn->nreq--;//for comod
		_jn_conn_req_dec(conn);
		//_unlock_mx(&conn->mx);
		//_jn_conn_req_dec(conn);//for comod

		return -JE_NOTFOUND;
	}

	cc_entry = (jn_cc_entry *) malloc(sizeof(jn_cc_entry));
	cc_entry->next = NULL;
	cc_entry->conn = conn;

	uentry = (jn_ucom_entry *) malloc(sizeof(jn_ucom_entry));
	uentry->next = NULL;

	_lock_mx(&entry->cc_list->mx);
	if (entry->cc_list->first == NULL) {
		entry->cc_list->first = cc_entry;
		entry->cc_list->last = cc_entry;
		entry->cc_list->cnt = 0L;
	} else {
		entry->cc_list->last->next = cc_entry;
		entry->cc_list->last = entry->cc_list->last->next;
	}
	entry->cc_list->cnt++;
	_unlock_mx(&entry->cc_list->mx);

	uentry->objid = entry->objid;
	uentry->copyname = (wchar_t *) malloc(WBYTES(entry->copyname));
	wcscpy(uentry->copyname, entry->copyname);

	uentry->mod_id = entry->com_id;

	_lock_mx(&entry->somx);

	entry->jnmod_handle_join((void *)h, (void *)entry, (void *)conn,
				 (void *)pkt, (void *)&_jn_fifo_enq);

	_unlock_mx(&entry->somx);

	_jn_com_req_dec(entry);

	uentry->flags = 0;

	//_lock_mx(&conn->ucom_list->mx); already locked above
	if (!conn->ucom_list->first) {
		conn->ucom_list->first = uentry;
		conn->ucom_list->last = uentry;
	} else {
		conn->ucom_list->last->next = uentry;
		conn->ucom_list->last = conn->ucom_list->last->next;
	}
	conn->ucom_list->cnt++;
	//_unlock_mx(&conn->mx);

	jn_send_plain(conn, pkt->hdr.trans_id, JNT_OK, 0, NULL);

	_wdeb6(L"user < %S > has joined comod < %S >", conn->username,
	       entry->copyname);

	return 0;
}

int _jn_close_com(jn_h * h, jn_conn * conn, jn_pkt * pkt)
{

	jn_comod_entry *com, *comprev;
	jn_fifo_entry *fifoentry, *job;
	jn_cc_entry *cc_entry;
	int ret = -JE_NOTFOUND;

	_lock_mx(&h->com_list->mx);
	for (com = h->com_list->first; com; com = com->next) {
		if (com->com_id == pkt->hdr.comod_id) {

			//_jn_com_req_inc(com);//not needed, i have 
			//increase from jn_open_com()

			if (conn) {
				if (conn->uid == com->owner_id) {
					if (com == h->com_list->first) {
						h->com_list->first =
						    h->com_list->first->next;
					} else {
						if (com == h->com_list->last) {
							h->com_list->last =
							    comprev;
						}
						comprev->next = com->next;
					}

					h->com_list->cnt--;
					ret = 0;
					break;
				} else {
					ret = -JE_NOPERM;
					break;
				}
			} else {
				if (com == h->com_list->first) {
					h->com_list->first =
					    h->com_list->first->next;
				} else {
					if (com == h->com_list->last) {
						h->com_list->last = comprev;
					}
					comprev->next = com->next;
				}

				h->com_list->cnt--;
				ret = 0;
				break;
			}
		} else {
			comprev = com;
		}
	}
	_unlock_mx(&h->com_list->mx);

	_lock_mx(&h->comlist_buf_mx);
	if (h->comlist_bufsize)
		free(h->comlist_buf);
	if (_jn_create_comlist(h, &h->comlist_buf, &h->comlist_bufsize) < 0) {
		//buffer not allocated in case of error
		h->comlist_bufsize = 0L;
	}
	_unlock_mx(&h->comlist_buf_mx);

	if (!ret) {

		//decrease cc_list members by one ( from join );
		for (cc_entry = com->cc_list->first; cc_entry;
		     cc_entry = cc_entry->next) {
			job = (jn_fifo_entry *) malloc(sizeof(jn_fifo_entry));
			if (job) {
				job->next = NULL;
				job->conn = cc_entry->conn;
				job->type = JN_FT_RM_CONN;
				job->uid = JACID_INVAL;

				_jn_fifo_enq(com->fifo, job);
			}
		}

		_lock_mx(&com->somx);
		com->jnmod_cleanup((void *)h, (void *)com);
		_unlock_mx(&com->somx);
		_wdeb5(L"comod req = %u", com->nreq);
		_lock_mx(&com->fifo->mx);
		fifoentry = com->fifo->first;
		while (fifoentry) {
			com->fifo->first = com->fifo->first->next;

			switch (fifoentry->type) {
			case JN_FT_PKT:
				free(fifoentry->ptr);
				_jn_com_req_dec(com);	//from submitting
				//the request
				break;
			case JN_FT_RM_CONN:
				break;
			default:
				break;
			}

			fifoentry = com->fifo->first;
		}
		_unlock_mx(&com->fifo->mx);

		_jn_com_req_dec(com);	//from jn_open_com()

		while (com->nreq) {
			sleep(JN_COM_CLOSE_WAIT_TIME);
		}

		_cancel_thread(com->rth_id);
		_cancel_thread(com->th_id);

		_wdeb6(L"user < %S > closing comod < %S >", conn->username,
		       com->copyname);

		free(com->copyname);
		free(com->modname);
		_jn_fifo_destroy(com->fifo);
		_jn_fifo_destroy(com->reply_fifo);

	}

	_wdeb6(L"DONE");

	return ret;

}

//comod creation, removing functions - END

//comod thread and queue functions - BEGIN
void *_jn_comod_thread(void *arg)
{

	th_arg *ca = (th_arg *) arg;
	jn_h *h = ca->h;
	jn_conn *conn = ca->conn;
	jn_comod_entry *com = ca->com;
	jn_cc_entry *del, *prev;
	jn_fifo_entry *nextq, *curq;
	com->owner_id = ca->uid;

	com->cc_list = _jn_cclist_new();
	//_jn_cclist_add(com->cc_list, conn);

	for (;;) {
		//extract queue
		nextq = _jn_fifo_wait(com->fifo);

		switch (nextq->type) {
		case JN_FT_PKT:
			_wdeb3(L"packet");

			//process it
			//_jn_conn_req_inc(nextq->conn); not needed,
			//the conn is already locked on open/join com request
			_lock_mx(&com->somx);

			_wdeb6(L"calling packet handler 0x%p",
			       com->jnmod_handle_packet);

			com->jnmod_handle_packet((void *)h, (void *)com,
						 (void *)nextq,
						 (void *)&_jn_fifo_enq);

			_wdeb6(L" packet handler returns 0x%p",
			       com->jnmod_handle_packet);

			_unlock_mx(&com->somx);
			free(nextq);

			_jn_conn_req_dec(nextq->conn);	//from poll,
			//the module code must increase
			//it if there is lock needed on
			//connection, depending on the
			//module design

			//_jn_com_req_dec(com); //don't put it here, leave
			//it to the module

			break;

		case JN_FT_RM_CONN:
			_wdeb2(L"rm conn %S", nextq->conn->username);

			//remove from cc_list

			_lock_mx(&com->cc_list->mx);
			_wdeb2(L"searching for uid: %u", nextq->conn->uid);
			del = com->cc_list->first;
			prev = del;
			while (del) {
				if (del->conn->uid == nextq->conn->uid) {
					_wdeb2(L"found");
					if (del->conn->uid ==
					    com->cc_list->first->conn->uid) {
						com->cc_list->first =
						    com->cc_list->first->next;
					} else {
						prev->next = del->next;
					}
					//don't remove inc_req from _create_comod or
					//_join_comod calls here!
					//but after handle_leave
					//_jn_dec_conn_req(del->conn);
					break;
				} else {
					prev = del;
					del = del->next;
				}
			}
			_unlock_mx(&com->cc_list->mx);
			if (del)
				free(del);

			if (!(com->flags & JN_COM_SINGLE_CC_LIST)) {
				//remove all conn reply queue from cc_lists of all queues
				_wdeb2
				    (L"removing all conn reply queues from cc_lists of all queues");

				_lock_mx(&com->reply_fifo->mx);
				for (curq = com->reply_fifo->first; curq;
				     curq = curq->next) {
					if (curq->cc_list) {
						_lock_mx(&curq->cc_list->mx);
						del = curq->cc_list->first;
						prev = del;
						while (del) {
							if (del->conn->uid ==
							    nextq->conn->uid) {
								if (del->conn->uid == curq->cc_list->first->conn->uid) {	//first entry
									curq->cc_list->first
									    =
									    curq->cc_list->first->next;
								} else {
									if (del->conn->uid == curq->cc_list->last->conn->uid) {
										curq->cc_list->last
										    =
										    prev;
									}
									prev->next
									    =
									    del->next;
								}
								break;
							} else {
								prev = del;
								del = del->next;
							}
						}
						_unlock_mx(&curq->cc_list->mx);
						free(del);
					}
				}
				_unlock_mx(&com->reply_fifo->mx);
			}

			_lock_mx(&com->somx);
			com->jnmod_handle_leave((void *)h, (void *)com,
						(void *)nextq->conn,
						NULL, (void *)&_jn_fifo_enq);
			_unlock_mx(&com->somx);
			//do not free nextq, it may be used in several
			//comods, normally this will be freed in
			//jn_close()

			_jn_conn_req_dec(nextq->conn);	//from join

			_jn_com_req_dec(com);	//from jn_close() call

			break;
		default:
			break;
		}

		//
	}
	return NULL;
}

void _jn_cclist_to_pblist(jn_cc_list * cclist, jn_pb_list ** pblist,
			  jn_pkt * pkt)
{
	buflist *bentry;
	jn_cc_entry *ccentry;
	jn_pb_list *first, *last, *entry, *prevlast;
	int newentry;
	uint32_t i;
	uint32_t bufsize = enc_pkt_size(pkt->hdr.data_len);

	_wdeb7(L"called");

	first = NULL;

	_lock_mx(&cclist->mx);

	newentry = 0;
	ccentry = cclist->first;
	for (i = 0; i < cclist->cnt; i++) {

		if (!first) {
			first = (jn_pb_list *) malloc(sizeof(jn_pb_list));
			if (!first) {
				*pblist = NULL;
				return;
			}
			first->buck = (jn_poll_buck *) ccentry->conn->poll_buck;
			first->next = NULL;
			first->bfirst = NULL;
			last = first;
			entry = first;
		} else {
			for (entry = first; entry; entry = entry->next) {
				if (entry->buck == ccentry->conn->poll_buck) {
					newentry = 0;
					break;
				}
			}
			if (!entry) {
				entry =
				    (jn_pb_list *) malloc(sizeof(jn_pb_list));

				if (!entry) {
					return;
				}
				entry->buck =
				    (jn_poll_buck *) ccentry->conn->poll_buck;
				entry->next = NULL;
				entry->bfirst = NULL;
				last->next = entry;

				prevlast = last;
				newentry = 1;

				last = last->next;
			}
		}

		//entry has the ptr to the jn_pb_list

		bentry = (buflist *) malloc(sizeof(buflist));
		if (!bentry) {
			if (newentry) {
				last = prevlast;
				free(entry);
			}
			return;
		}
		bentry->buf = (uchar *) malloc(bufsize);
		if (!bentry->buf) {
			free(bentry);
			if (newentry) {
				last = prevlast;
				free(entry);
			}
			return;
		}
		bentry->bufsize = bufsize;
		bentry->fd = ccentry->conn->sockfd;
		bentry->next = NULL;

		_jn_prepare_pkt(ccentry->conn, pkt, bentry->buf);

		if (!entry->bfirst) {
			entry->bfirst = bentry;
			entry->blast = bentry;
		} else {
			entry->blast->next = bentry;
			entry->blast = entry->blast->next;
		}

		ccentry = ccentry->next;

	}

	*pblist = first;

	_unlock_mx(&cclist->mx);

	_wdeb7(L"returning");
}

void jn_bcast_pkt(jn_cc_list * list, jn_pkt * pkt)
{

	jn_pb_list *pb_list, *pb_entry, *pb_prev;
	buflist *bentry, *bprev;

	_wdeb(L"called, cclist->cnt = %u, pkt->trans_id = %u",
	      list->cnt, pkt->hdr.trans_id);

	_jn_cclist_to_pblist(list, &pb_list, pkt);

	pb_entry = pb_list;
	while (pb_entry) {

		_jn_begin_send_requests(pb_entry->buck);

		bentry = pb_entry->bfirst;
		while (bentry) {

			_jn_add_send_request(pb_entry->buck, bentry->fd,
					     bentry->buf, bentry->bufsize,
					     JN_SEND_FREE);

			bprev = bentry;
			bentry = bentry->next;
			free(bprev);
		}

		_jn_finish_send_requests(pb_entry->buck);

		pb_prev = pb_entry;
		pb_entry = pb_entry->next;
		free(pb_prev);
	}
}

void *_jn_comod_rthread(void *arg)
{

	struct aiocb *cb, **cblist;

	th_arg *ca = (th_arg *) arg;
	jn_comod_entry *com = ca->com;
	jn_fifo_entry *nextq;
	jn_pkt *pkt;
	jn_cc_entry *cc_entry;
	uint32_t cnt;
	uint32_t buf_size;

	struct sigevent sig;

	_wdeb(L"Temporary implementation, got to handle time-outs better");

	for (;;) {
		//extract queue
		nextq = _jn_fifo_wait(com->reply_fifo);

		pkt = (jn_pkt *) (nextq->ptr);

#ifndef NDEBUG
		if (pkt->hdr.flags & JNF_PLAIN) {
			_wdeb4(L"plain data (jcs): %s", pkt->data);
		}
#endif

		//send reply to the list
		if (nextq->cc_list) {
			_wdeb3(L"has cc_list, cnt = %u", nextq->cc_list->cnt);

			jn_bcast_pkt(nextq->cc_list, pkt);

			if (nextq->flags & JN_FF_FREE_CC_LIST) {
				_wdeb3(L"freeing cc_list");
				_jn_cclist_destroy(nextq->cc_list);
			}
		}
		//free some memmory
		if (nextq->type == JN_FT_PKT) {
			_wdeb3(L"q type : PKT");
			if (nextq->flags & JN_FF_FREE_DATA) {
				_wdeb3(L"need to free packet data");
				if (pkt->hdr.data_len) {
					free(pkt->data);
				}
			}
			free(nextq->ptr);	//pkt

		}
		free(nextq);

	}
	return NULL;
}

//comod thread and queue functions - END

uint32_t _jn_com_req_dec(jn_comod_entry * com)
{
	_lock_mx(&com->reqmx);

	if (com->nreq)
		com->nreq--;

	//_signal_cond(&com->req_cond);

	_unlock_mx(&com->reqmx);

	return com->nreq;
}

uint32_t _jn_com_req_inc(jn_comod_entry * com)
{
	_lock_mx(&com->reqmx);
	/*
	   while(com->nreq >= com->maxreq){
	   _wdeb(L"entering wait cond, nreq = %u", com->nreq);
	   _wait_cond(&com->req_cond, &com->reqmx);
	   }
	 */
	com->nreq++;

	_unlock_mx(&com->reqmx);

	return com->nreq;
}
