#include "jnet.h"
#include "jnet_internals.h"
#include "debug.h"
#include "jer.h"
#include "jrand.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <wchar.h>

#define _wdeb1(f, a...)		//trans_a
#define _wdeb2(f, a...)		//dumps pkt.hdr.rsize in _jn_conn_proc_buf
#define _wdeb3 _wdeb		//_jn_conn_proc_buf

jn_trans_a *_jn_transa_new(jtransid_t total)
{
	jtransid_t i;

#ifndef JN_SINGLE_CLT
#ifndef NDEBUG
	jmxattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif
#endif
	_wdeb1(L"initing transa %u, MAX_TRANS %u", total, JN_CONF_MAX_TRANS);

	if (!total) {
		_wdeb(L"initing transa %u", total);
		return NULL;
	}

	jn_trans_a *trans_a = (jn_trans_a *) malloc(sizeof(jn_trans_a));
	if (!trans_a)
		return NULL;

	trans_a->tid = (jtransid_t *) malloc(sizeof(jtransid_t) * total);
	if (!trans_a->tid) {
		free(trans_a);
		return NULL;
	}

	for (i = 0; i < total; i++)
		trans_a->tid[i] = 0;

#ifndef JN_SINGLE_CLT
#ifndef NDEBUG
	_init_mx(&trans_a->mx, &attr);
#else
	_init_mx(&trans_a->mx, NULL);
#endif
#endif

	trans_a->total = total;
	trans_a->cnt = 0;

	return trans_a;

}

void _jn_transa_chg(jn_trans_a * trans_a, jtransid_t total)
{
	jtransid_t *tids, cnt, n;

	assert(trans_a->total != total);

	tids = (jtransid_t *) malloc(total * sizeof(jtransid_t));
	if (!tids)
		return;

#ifndef JN_SINGLE_CLT
	_lock_mx(&trans_a->mx);
#endif

	if (total > trans_a->total) {
		memcpy(tids, trans_a->tid, trans_a->total * sizeof(jtransid_t));
		free(trans_a->tid);
		trans_a->tid = tids;
		trans_a->total = total;
	} else {
		for (n = 0, cnt = 0; cnt < trans_a->total, n < total; cnt++) {
			if (trans_a->tid[cnt]) {
				tids[n] = trans_a->tid[cnt];
				n++;
			}
		}
		trans_a->cnt = n;
		trans_a->total = total;
		free(trans_a->tid);
		trans_a->tid = tids;
	}
#ifndef JN_SINGLE_CLT
	_unlock_mx(&trans_a->mx);
#endif

}

void _jn_transa_destroy(jn_trans_a * trans_a)
{

#ifndef JN_SINGLE_CLT
	_lock_mx(&trans_a->mx);
#endif

	free(trans_a->tid);

#ifndef JN_SINGLE_CLT
	_unlock_mx(&trans_a->mx);

	_destroy_mx(&trans_a->mx);
#endif

}

int _jn_transa_add(jn_trans_a * trans_a, jtransid_t tid)
{
	jtransid_t cnt;
	int ret = -JE_NOTFOUND;

#ifndef JN_SINGLE_CLT
	_lock_mx(&trans_a->mx);
#endif

	for (cnt = 0; cnt < trans_a->total; cnt++) {
		if (!trans_a->tid[cnt]) {
			trans_a->tid[cnt] = tid;
			ret = 0;
			trans_a->cnt++;
			break;
		}
	}

#ifndef JN_SINGLE_CLT
	_unlock_mx(&trans_a->mx);
#endif

	return ret;

}

jtransid_t _jn_transa_get(jn_trans_a * trans_a)
{
	jtransid_t ret, cnt;
	int zeropos = -1;
 calc_again:
	_wdeb1(L"trans_a->total = %u", trans_a->total);
	if (jrand_buf((uchar *) & ret, sizeof(jtransid_t))) {
		_wdeb(L"jrand_buf() fails");
		return 0;
	}
	_wdeb(L"trans_a candid: %u", ret);

#ifndef JN_SINGLE_CLT
	_lock_mx(&trans_a->mx);
#endif
	for (cnt = 0; cnt < trans_a->total; cnt++) {
		if (ret == trans_a->tid[cnt]) {
#ifndef JN_SINGLE_CLT
			_unlock_mx(&trans_a->mx);
#endif
			_wdeb1(L"tid %u already exists in %u, calc again...",
			       ret, cnt);
			goto calc_again;
		}
		if ((!trans_a->tid[cnt]) && (zeropos == -1)) {
			_wdeb1(L"setting zeropos to %u", cnt);
			zeropos = cnt;
		}
	}

	if (zeropos != -1) {
		trans_a->tid[zeropos] = ret;
	}
#ifndef JN_SINGLE_CLT
	_unlock_mx(&trans_a->mx);
#endif
	if (zeropos != -1) {
		_wdeb(L"put on [ %i ]", zeropos);
		return ret;
	} else {
		_wdeb1(L"fails here");
		return 0;
	}

}

void _jn_transa_rm(jn_trans_a * trans_a, jtransid_t tid)
{

	jtransid_t cnt;

#ifndef JN_SINGLE_CLT
	_lock_mx(&trans_a->mx);
#endif

	if (!tid)
		return;

	for (cnt = 0; cnt < trans_a->total; cnt++) {
		if (trans_a->tid[cnt] == tid) {
			_wdeb1(L"removing %u (rq %u) from slot %u",
			       trans_a->tid[cnt], tid, cnt);
			trans_a->tid[cnt] = 0;
			trans_a->cnt--;
			break;
		}
	}

#ifndef JN_SINGLE_CLT
	_unlock_mx(&trans_a->mx);
#endif

}

jn_ucom_list *_jn_ucom_list_new()
{
	jn_ucom_list *ucom_list;

/*
#ifndef JN_SINGLE_CLT	
# ifndef NDEBUG
	jmxattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
# endif
#endif
*/

	//ucom
	ucom_list = (jn_ucom_list *) malloc(sizeof(jn_ucom_list));
	if (!ucom_list)
		return NULL;
	ucom_list->first = NULL;
/*
#ifndef JN_SINGLE_CLT	
# ifndef NDEBUG
	_init_mx(&ucom_list->mx, &attr);
# else
	_init_mx(&ucom_list->mx, NULL);
# endif
#endif
*/
	ucom_list->cnt = 0L;

	return ucom_list;
}

void _jn_ucom_list_destroy(jn_ucom_list * list)
{
	jn_ucom_entry *entry;
/*
#ifndef JN_SINGLE_CLT	
	_lock_mx(&list->mx);
#endif
*/

	entry = list->first;
	while (entry) {
		list->first = list->first->next;
		if (entry->copyname)
			free(entry->copyname);
		free(entry);
		entry = list->first;
	}

/*
#ifndef JN_SINGLE_CLT	
	_unlock_mx(&list->mx);
	
	_destroy_mx(&list->mx);
#endif
*/
	free(list);
}

void _jn_ucom_list_add(jn_ucom_list * list, jn_ucom_entry * entry)
{

/*
#ifndef JN_SINGLE_CLT
	_lock_mx(&list->mx);
#endif
*/

	if (!list->first) {
		list->first = entry;
		list->last = entry;
		list->cnt = 1L;
	} else {
		list->last->next = entry;
		list->last = list->last->next;
		list->cnt++;
	}

/*	
#ifndef JN_SINGLE_CLT	
	_unlock_mx(&list->mx);
#endif
*/

}

int _jn_ucom_list_find(jn_ucom_list * list, jn_ucom_entry * compare,
		       jn_ucom_entry ** entry, jn_ucom_entry ** prev,
		       uint16_t pattern)
{

	int ret = -JE_NOTFOUND;
	int tofind = 0;
	int found;

	if (pattern & JN_FIND_FLAGS)
		tofind++;
	if (pattern & JN_FIND_OBJID)
		tofind++;
	if (pattern & JN_FIND_MODID)
		tofind++;
	if (pattern & JN_FIND_NAME)
		tofind++;
/*
#ifndef JN_SINGLE_CLT
	_lock_mx(&list->mx);
#endif
*/

	*entry = list->first;
	*prev = list->first;

	while (*entry) {

		found = 0;

		if (pattern & JN_FIND_FLAGS)
			if ((*entry)->flags & compare->flags)
				found++;

		if (pattern & JN_FIND_OBJID)
			if ((*entry)->objid != compare->objid)
				found++;

		if (pattern & JN_FIND_MODID)
			if ((*entry)->mod_id != compare->mod_id)
				found++;

		if (pattern & JN_FIND_NAME)
			if (!wcscmp((*entry)->copyname, compare->copyname))
				found++;

		if (found == tofind) {
			ret = 0;
			goto end;
		} else {
			*prev = *entry;
			(*entry) = (*entry)->next;
		}

	}

 end:

/*
#ifndef JN_SINGLE_CLT
	if(ret == -JE_NOTFOUND)
		_unlock_mx(&list->mx);
#endif
*/

	return ret;

}

void _jn_ucom_list_rm(jn_ucom_list * list, jn_ucom_entry * entry,
		      uint16_t pattern)
{
	jn_ucom_entry *ptr, *prev;

	if (_jn_ucom_list_find(list, entry, &ptr, &prev, pattern) ==
	    -JE_NOTFOUND)
		return;

	if (ptr == list->first) {
		list->first = list->first->next;
	} else {
		prev->next = ptr->next;
	}

/*	
#ifndef JN_SINGLE_CLT
	//remained locked from previous successful search
	_unlock_mx(&list->mx);
#endif
*/

	if (ptr->copyname)
		free(ptr->copyname);
	free(ptr);

}

int _jn_conn_init(jn_conf * conf, jn_conn * conn)
{

#ifndef JN_SINGLE_CLT
#ifndef NDEBUG
	jmxattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif
#endif

	conn->bytes_rcvd = 0L;
	conn->bytes_sent = 0L;
	conn->flags = 0x00;
	conn->prev_len = 0L;

/*
#ifndef JN_SINGLE_CLT
# ifndef NDEBUG
	_init_mx(&conn->mx, &attr);
	_wdeb(L"connection mutex initialized");
# else
	_init_mx(&conn->mx, NULL);
# endif
#endif
*/

	if (conf) {
		conn->cbq_gc_trigger = conf->cbq_gc_trigger;
		conn->cbq_gc_to = conf->cbq_gc_timeout;
		//conn->maxreq = conf->maxconnreq;
	} else {
		conn->cbq_gc_trigger = JN_CONF_CBQ_TRIGGER;
		conn->cbq_gc_to = JN_CONF_CBQ_TIMEOUT;
		//conn->maxreq = JN_CONF_MAX_CONN_REQ;
	}

	conn->ucom_list = _jn_ucom_list_new();
	if (!conn->ucom_list) {

/*
#ifndef JN_SINGLE_CLT
		_destroy_mx(&conn->mx);	
#endif
*/
		return -JE_MALOC;

	}

	return 0;
}

jn_conn *_jn_conn_new(jn_conf * conf)
{
	jn_conn *conn;

	conn = (jn_conn *) malloc(sizeof(jn_conn));
	if (!conn)
		return NULL;

	if (_jn_conn_init(conf, conn) < 0) {
		free(conn);
		return NULL;
	}

	return conn;
}

void _jn_conn_destroy(jn_conn * conn)
{

	close(conn->sockfd);

#ifndef JN_SINGLE_CLT
	_cancel_thread(conn->cbthid);
	//_destroy_mx(&conn->mx);
#endif

	_jn_ucom_list_destroy(conn->ucom_list);
	_jn_cb_fifo_destroy(conn->cbfifo);

/*
#ifndef JN_SINGLE_CLT
	_destroy_mx(&conn->sendmx);
	_destroy_mx(&conn->recvmx);
#endif
*/

}

jn_conn_list *_jn_conn_list_new()
{
	jn_conn_list *list;
	list = (jn_conn_list *) malloc(sizeof(jn_conn_list));
	if (!list)
		return NULL;
	list->first = NULL;
#ifndef JN_SINGLE_CLT
	_init_mx(&list->mx, NULL);
#endif
	list->cnt = 0L;
	return list;
}

void _jn_conn_list_destroy(jn_conn_list * list)
{
	jn_conn *entry;

#ifndef JN_SINGLE_CLT
	_lock_mx(&list->mx);
#endif

	entry = list->first;

	while (list->first) {
		list->first = list->first->next;
		_jn_conn_destroy(entry);
		entry = list->first;

	}

#ifndef JN_SINGLE_CLT
	_unlock_mx(&list->mx);

	_destroy_mx(&list->mx);
#endif

	free(list);
}

int _jn_conn_find(jn_conn_list * list, jn_conn * compare, jn_conn ** entry,
		  jn_conn ** prev, uint16_t pattern)
{

	int ret = -JE_NOTFOUND;
	int tofind = 0;
	int found;

	if (pattern & JN_FIND_FLAGS)
		tofind++;
	if (pattern & JN_FIND_FD)
		tofind++;
	if (pattern & JN_FIND_UID)
		tofind++;
	if (pattern & JN_FIND_NAME)
		tofind++;

#ifndef JN_SINGLE_CLT
	_lock_mx(&list->mx);
#endif

	*entry = list->first;
	*prev = list->first;

	while (*entry) {

		found = 0;

		if (pattern & JN_FIND_FLAGS)
			if ((*entry)->flags & compare->flags)
				found++;

		if (pattern & JN_FIND_FD)
			if ((*entry)->sockfd != compare->sockfd)
				found++;

		if (pattern & JN_FIND_UID)
			if ((*entry)->uid != compare->uid)
				found++;

		if (pattern & JN_FIND_NAME)
			if (!wcscmp((*entry)->username, compare->username))
				found++;

		if (found == tofind) {
			ret = 0;
			goto end;
		} else {
			*prev = *entry;
			(*entry) = (*entry)->next;
		}

	}

 end:

#ifndef JN_SINGLE_CLT
	if (ret == -JE_NOTFOUND)
		_unlock_mx(&list->mx);
#endif

	return ret;

}

uint16_t _jn_conn_req_dec(jn_conn * conn)
{

	uint16_t n;

#ifndef JN_SINGLE_CLT
	_lock_mx(&((jn_poll_buck *) conn->poll_buck)->conn_a_mx);
#endif

	if (((jn_poll_buck *) conn->poll_buck)->nreq[conn->poll_pos] > 0) {
		((jn_poll_buck *) conn->poll_buck)->nreq[conn->poll_pos]--;
	}
	n = ((jn_poll_buck *) conn->poll_buck)->nreq[conn->poll_pos];

#ifndef JN_SINGLE_CLT
	_unlock_mx(&((jn_poll_buck *) conn->poll_buck)->conn_a_mx);
#endif

	return n;
}

uint16_t _jn_conn_req_inc(jn_conn * conn)
{

	uint16_t n;

#ifndef JN_SINGLE_CLT
	_lock_mx(&((jn_poll_buck *) conn->poll_buck)->conn_a_mx);
#endif

	if (((jn_poll_buck *) conn->poll_buck)->nreq[conn->poll_pos] <
	    ((jn_poll_buck *) conn->poll_buck)->maxconnreq) {
		((jn_poll_buck *) conn->poll_buck)->nreq[conn->poll_pos]++;
	}

	n = ((jn_poll_buck *) conn->poll_buck)->nreq[conn->poll_pos];

#ifndef JN_SINGLE_CLT
	_unlock_mx(&((jn_poll_buck *) conn->poll_buck)->conn_a_mx);
#endif

	return n;
}

int _jn_conn_req_chkinc(jn_conn * conn)
{

#ifndef JN_SINGLE_CLT
	_lock_mx(&((jn_poll_buck *) conn->poll_buck)->conn_a_mx);
#endif

	if (((jn_poll_buck *) conn->poll_buck)->nreq[conn->poll_pos] <
	    ((jn_poll_buck *) conn->poll_buck)->maxconnreq) {
		((jn_poll_buck *) conn->poll_buck)->nreq[conn->poll_pos]++;

#ifndef JN_SINGLE_CLT
		_unlock_mx(&((jn_poll_buck *) conn->poll_buck)->conn_a_mx);
#endif

		return 0;
	} else {

#ifndef JN_SINGLE_CLT
		_unlock_mx(&((jn_poll_buck *) conn->poll_buck)->conn_a_mx);
#endif

		return -JE_LOWRES;
	}
}

uint16_t _jn_conn_req_chk(jn_conn * conn)
{

	uint16_t n;

#ifndef JN_SINGLE_CLT
	_lock_mx(&((jn_poll_buck *) conn->poll_buck)->conn_a_mx);
#endif

	n = ((jn_poll_buck *) conn->poll_buck)->nreq[conn->poll_pos];

#ifndef JN_SINGLE_CLT
	_unlock_mx(&((jn_poll_buck *) conn->poll_buck)->conn_a_mx);
#endif

	return n;
}

static inline int _jn_conn_extract_pkt_from_buf0(jn_conn * conn,
						 uchar * buf, uint16_t bufsize,
						 uint16_t * pos, jn_pkt * pkt)
{

	uint16_t esize;		//encrypted size
	uint16_t i;
	int ret;
	uchar hbuf[AES_BSIZE];

	/*
	   if(*pos < bufsize - AES_BSIZE + 1){
	   return -JE_SIZE;
	   }
	 */

	_wdeb3(L"pos = %u", *pos);

	aes_decrypt(&conn->aes, buf + (*pos), hbuf);
	if (unpacki16(hbuf) == JN_PKT_MAGIC) {
		ret = _jn_disassm_hdr(&pkt->hdr, hbuf);
		if (!ret) {
			esize = enc_pkt_size(pkt->hdr.data_len);
			_wdeb3(L"enc_pkt_size = %u, bufsize = %u", esize,
			       bufsize);
			if (esize <= bufsize) {	//enough data, try to extract
				esize -= AES_BSIZE;	//for data
				pkt->data = (uchar *) malloc(esize);
				for (i = 0; i < esize; i += AES_BSIZE) {
					aes_decrypt(&conn->aes,
						    buf + AES_BSIZE + i +
						    (*pos), pkt->data + i);
				}

				//avoid being attacked this way
				//remove this flag if you are at server side
				if (conn->flags & JN_CF_SVR) {
					pkt->hdr.flags &= ~JNF_LOST;
				}

				*pos += esize + AES_BSIZE;
				_wdeb3(L"packet OK, pos = %u", *pos);
				return 0;
			} else if (ret == -JE_HCRC) {
				*pos += AES_BSIZE;
				pkt->hdr.flags |= JNF_LOST;
				_wdeb3
				    (L"header CRC failed, trans_id = %u, pos = %u",
				     pkt->hdr.trans_id, *pos);
				return -JE_HCRC;

			} else {
				*pos = esize - bufsize;
				_wdeb3(L"_jn_assert_pkt() fails, pos = %u",
				       *pos);
				return -JE_LOWBUF;
			}
		}

		_wdeb3(L"_jn_disassm_hdr() returns %i", ret);
	}

	return -JE_NOTFOUND;
}

int _jn_conn_proc_buf(jn_conn * conn, jn_pkt_list ** list, uint16_t * npkt,
		      uchar * buf, uint16_t bufsize)
{

	uint16_t pos;
	int ret;
	jn_pkt_list *first, *last = NULL, *entry;
	uchar *total_buf;
	uint32_t total_size;
	jn_pkt *cur_pkt;

	if (conn->prev_len) {
		_wdeb3(L"prev_len = %u, bufsize = %u", conn->prev_len, bufsize)
		    if (bufsize <= JN_MAX_PKT - conn->prev_len) {

			assert(conn->prev_len + bufsize <= JN_MAX_PKT);
			memcpy(conn->prev + conn->prev_len, buf, bufsize);
			conn->prev_len += bufsize;

			total_buf = conn->prev;
			total_size = conn->prev_len;

		} else {

			total_size = conn->prev_len + bufsize;
			total_buf = (uchar *) malloc(bufsize);

		}
	} else {

		total_buf = buf;
		total_size = bufsize;

	}

	if (total_size < AES_BSIZE) {
		_wdeb3(L"total_size = %u, prev_len = %u",
		       total_size, conn->prev_len);
		if (!conn->prev_len) {
			memcpy(conn->prev, total_buf, total_size);
			conn->prev_len = total_size;
		}

		return -JE_LOWBUF;
	}

	cur_pkt = (jn_pkt *) malloc(sizeof(jn_pkt));
	*npkt = 0;
	pos = 0;
	first = NULL;

	while (pos < total_size - AES_BSIZE + 1) {

		ret = _jn_conn_extract_pkt_from_buf0(conn, total_buf + pos,
						     total_size - pos, &pos,
						     cur_pkt);

		switch (ret) {
		case -JE_CRC:
		case 0:
			entry = (jn_pkt_list *) malloc(sizeof(jn_pkt_list));
			entry->next = NULL;
			entry->pkt = cur_pkt;
			if (!first) {
				first = entry;
				last = first;
			} else {
				last->next = entry;
				last = last->next;
			}
			(*npkt)++;
			//the ending1 protects from DC-REQ-... sequence
			//which may cause resource-leak
			if (cur_pkt->hdr.type == JNT_DC)
				goto ending1;
			cur_pkt = (jn_pkt *) malloc(sizeof(jn_pkt));
			break;
		case -JE_LOWBUF:	//pos reports needed bytes here

			if (pos) {
				conn->prev_len = total_size - pos;
				memcpy(conn->prev, total_buf + pos,
				       total_size - pos);
			}

			free(cur_pkt);
			*list = first;

			return 0;
			break;
		case -JE_NOTFOUND:
			pos++;
			break;
		}
	}

	free(cur_pkt);
 ending1:
	*list = first;
	return 0;
}

int jn_conn_cclist_to_jcslist(jn_cc_list * cc_list, jcslist_entry ** jcslist)
{
	jn_cc_entry *cc_entry;
	jcslist_entry *entry, *last = NULL;
	*jcslist = NULL;

	for (cc_entry = cc_list->first; cc_entry; cc_entry = cc_entry->next) {

		entry = (jcslist_entry *) malloc(sizeof(jcslist_entry));
		entry->next = NULL;

		entry->wcs =
		    (wchar_t *) malloc(WBYTES(cc_entry->conn->username));
		if (!entry->wcs) {

			return -JE_MALOC;
		}
		wcscpy(entry->wcs, cc_entry->conn->username);

		if (!(*jcslist)) {
			*jcslist = entry;
			last = entry;
		} else {
			last->next = entry;
			last = entry;
		}
	}

	return 0;
}
