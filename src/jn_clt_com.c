#include "jnet.h"
#include "jer.h"
#include "ecc.h"
#include "debug.h"
#include "jcs.h"
#include "jnet_internals.h"

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

#define _deb1(f, a...)
#define _wdeb1(f, a...)
#define _wdeb2(f, a...)		//_jn_recv_buf
#define _wdeb3(f, a...)		//open_com_prog

void _jn_open_com_progress(jn_cb_entry * cb_entry)
{
	//jn_ucom_entry* ucom;
	void (*callback) (jn_cb_entry *);
	jn_pkt *pkt = cb_entry->pkt;
	//struct _jn_open_com* s=(struct _jn_open_com*)cb_entry->progress_context;
	//jn_conn* conn = (jn_conn*)cb_entry->conn;

	_wdeb3(L"called");

#ifndef NDEBUG
	if (cb_entry->progress_context != cb_entry->if_context) {
		_wdeb(L"WARNING: PCTX != IFCTX");
	}
#endif

	if (pkt->hdr.type != JNT_OK) {

		_wdeb(L"recieved not-ok, type = %x", pkt->hdr.type);

		if (!(pkt->hdr.flags & JNF_ERR)) {
			pkt->hdr.rs = (uint32_t) - JE_UNK;
			pkt->hdr.flags |= JNF_ERR;
		}

		cb_entry->jerr = (int)pkt->hdr.rs;

		if (cb_entry->if_callback) {
			callback = cb_entry->if_callback;
			callback(cb_entry);
		}

		return;
	}

/*	do not join the comod
	ucom = (jn_ucom_entry*)malloc(sizeof(jn_ucom_entry));
	ucom->next = NULL;
	ucom->copyname = s->copyname;
	ucom->objid = s->objid;
	ucom->mod_id = pkt->hdr.comod_id;
	s->modid = ucom->mod_id; //in return

#ifndef JN_SINGLE_CLT
	_lock_mx(&conn->mx);
#endif
	if(conn->ucom_list->first == NULL){
		conn->ucom_list->first = ucom;
		conn->ucom_list->last = ucom;
		conn->ucom_list->cnt = 1;
	} else {
		conn->ucom_list->last->next = ucom;
		conn->ucom_list->last = conn->ucom_list->last->next;
		conn->ucom_list->cnt++;		
	}
#ifndef JN_SINGLE_CLT
	_unlock_mx(&conn->mx);
#endif
*/
	cb_entry->jerr = 0;
	//((struct _jn_open_com*)cb_entry->if_context)->modid = pkt->hdr.comod_id;

	if (cb_entry->if_callback) {
		callback = cb_entry->if_callback;
		callback(cb_entry);
	}

}

int jn_open_com(jn_conn * conn, void *if_callback, void *if_context,
		struct _jn_open_com *open_s)
{
	jn_pkt pkt;
	int ret;
	jn_cb_entry *q;

	if (wcslen(open_s->copyname) >= JN_MAX_COM_NAME) {
		return -JE_FORBID;
	}

	if ((ret =
	     _jn_assm_com_open(&pkt, open_s->flags, open_s->objid,
			       open_s->copyname)) < 0)
		return ret;

	pkt.hdr.trans_id = _jn_transa_get(conn->trans_a);

	q = jn_assm_cb(conn, JN_CB_TID, pkt.hdr.trans_id, 0,
		       (void *)&_jn_open_com_progress, (void *)open_s,
		       if_callback, if_context);

	if ((ret = jn_reg_cb(conn, &pkt, q)) < 0) {
		free(q);
	}

	return ret;
}

void _jn_join_com_progress(jn_cb_entry * cb_entry)
{
	jn_ucom_entry *ucom;
	void (*callback) (jn_cb_entry *);
	jn_pkt *pkt = cb_entry->pkt;
	struct _jn_open_com *s =
	    (struct _jn_open_com *)cb_entry->progress_context;
	jn_conn *conn = (jn_conn *) cb_entry->conn;

	if (pkt->hdr.type != JNT_OK) {

		_wdeb(L"recieved not-ok, type = %x, err_no = %i", pkt->hdr.type,
		      pkt->hdr.rs);

		if (!(pkt->hdr.flags & JNF_ERR)) {
			pkt->hdr.rs = (uint32_t) - JE_UNK;
			pkt->hdr.flags |= JNF_ERR;
		}

		cb_entry->jerr = (int)pkt->hdr.rs;
		if (cb_entry->if_callback) {
			callback = cb_entry->if_callback;
			callback(cb_entry);
		}

		return;

	}

	ucom = (jn_ucom_entry *) malloc(sizeof(jn_ucom_entry));
	ucom->next = NULL;
	ucom->copyname = s->copyname;
	ucom->objid = s->objid;
	ucom->mod_id = s->modid;

/*
#ifndef JN_SINGLE_CLT
	_lock_mx(&conn->mx);
#endif
*/
	if (conn->ucom_list->first == NULL) {
		conn->ucom_list->first = ucom;
		conn->ucom_list->last = ucom;
		conn->ucom_list->cnt = 1;
	} else {
		conn->ucom_list->last->next = ucom;
		conn->ucom_list->last = conn->ucom_list->last->next;
		conn->ucom_list->cnt++;
	}
/*	
#ifndef JN_SINGLE_CLT
	_unlock_mx(&conn->mx);
#endif
*/
	cb_entry->jerr = 0;

	if (cb_entry->if_callback) {
		callback = cb_entry->if_callback;
		callback(cb_entry);
	}

}

int jn_join_com(jn_conn * conn, void *if_callback, void *if_context,
		struct _jn_open_com *open_s)
{
	jn_pkt pkt;
	int ret;
	jn_cb_entry *q;

	_jn_assm_proto1_pkt(&pkt, open_s->modid, JNT_COMJOIN);

	pkt.hdr.trans_id = _jn_transa_get(conn->trans_a);

	q = jn_assm_cb(conn, JN_CB_TID, pkt.hdr.trans_id, 0,
		       (void *)&_jn_join_com_progress, (void *)open_s,
		       if_callback, if_context);

	if ((ret = jn_reg_cb(conn, &pkt, q)) < 0) {
		free(q);
	}

	return ret;
}

void _jn_leave_com_progress(jn_cb_entry * cb_entry)
{
	jn_ucom_entry *ucom, *prev;
	void (*callback) (jn_cb_entry *);
	jn_pkt *pkt = cb_entry->pkt;
	jn_conn *conn = (jn_conn *) cb_entry->conn;

	if (pkt->hdr.type != JNT_OK) {

		_wdeb(L"recieved not-ok, type = %x", pkt->hdr.type);

		if (!(pkt->hdr.flags & JNF_ERR)) {
			pkt->hdr.rs = (uint32_t) - JE_UNK;
			pkt->hdr.flags |= JNF_ERR;
		}

		cb_entry->jerr = (int)pkt->hdr.rs;
		if (cb_entry->if_callback) {
			callback = cb_entry->if_callback;
			callback(cb_entry);
		}

		return;

	}
/*
#ifndef JN_SINGLE_CLT
	_lock_mx(&conn->mx);
#endif
*/
	prev = conn->ucom_list->first;
	for (ucom = conn->ucom_list->first; ucom; ucom = ucom->next) {
		if (ucom->mod_id == pkt->hdr.comod_id) {
			break;
		} else {
			prev = ucom;
		}
	}

	if (ucom) {
		if (ucom == conn->ucom_list->first) {
			conn->ucom_list->first = conn->ucom_list->first->next;
		} else {
			if (ucom == conn->ucom_list->last) {
				conn->ucom_list->last = prev;
			}
			prev->next = ucom->next;
		}
		conn->ucom_list->cnt--;

		cb_entry->jerr = 0;
	} else {
		cb_entry->jerr = -JE_NOTFOUND;
	}
/*
#ifndef JN_SINGLE_CLT
	_unlock_mx(&conn->mx);
#endif
*/

	if (cb_entry->if_callback) {
		callback = cb_entry->if_callback;
		callback(cb_entry);
	}

}

int jn_leave_com(jn_conn * conn, void *if_callback, void *if_context,
		 jmodid_t com_id)
{
	jn_pkt pkt;
	int ret;
	jn_cb_entry *q;

	_jn_assm_proto1_pkt(&pkt, com_id, JNT_COMLEAVE);

	pkt.hdr.trans_id = _jn_transa_get(conn->trans_a);

	q = jn_assm_cb(conn, JN_CB_TID, pkt.hdr.trans_id, 0,
		       (void *)&_jn_leave_com_progress, NULL,
		       if_callback, if_context);

	if ((ret = jn_reg_cb(conn, &pkt, q)) < 0) {
		free(q);
	}

	return ret;
}

void _jn_proto1_com_progress(jn_cb_entry * cb_entry)
{
	//jn_ucom_entry* ucom, *prev;
	void (*callback) (jn_cb_entry *);
	jn_pkt *pkt = cb_entry->pkt;
	//jn_conn* conn = (jn_conn*)cb_entry->conn;

	if (pkt->hdr.type != JNT_OK) {

		_wdeb(L"recieved not-ok, type = %x", pkt->hdr.type);

		if (!(pkt->hdr.flags & JNF_ERR)) {
			pkt->hdr.rs = (uint32_t) - JE_UNK;
			pkt->hdr.flags |= JNF_ERR;
		}

		cb_entry->jerr = (int)pkt->hdr.rs;

		if (cb_entry->if_callback) {
			callback = cb_entry->if_callback;
			callback(cb_entry);
		}

		return;

	}

	if (cb_entry->if_callback) {
		callback = cb_entry->if_callback;
		callback(cb_entry);
	}

}

static inline int _jn_proto1_com(jn_conn * conn, void *if_callback,
				 void *if_context, jmodid_t com_id, uchar type)
{
	jn_pkt pkt;
	int ret;
	jn_cb_entry *q;

	_jn_assm_proto1_pkt(&pkt, com_id, type);

	pkt.hdr.trans_id = _jn_transa_get(conn->trans_a);

	q = jn_assm_cb(conn, JN_CB_TID, pkt.hdr.trans_id, 0,
		       (void *)&_jn_proto1_com_progress, NULL,
		       if_callback, if_context);

	if ((ret = jn_reg_cb(conn, &pkt, q)) < 0) {
		free(q);
	}

	return ret;
}

int jn_pause_com(jn_conn * conn, void *if_callback, void *if_context,
		 jmodid_t com_id)
{
	return _jn_proto1_com(conn, if_callback, if_context, com_id,
			      JNT_COMPAUSE);
}

int jn_resume_com(jn_conn * conn, void *if_callback, void *if_context,
		  jmodid_t com_id)
{
	return _jn_proto1_com(conn, if_callback, if_context, com_id,
			      JNT_COMRESUME);
}

int jn_close_com(jn_conn * conn, void *if_callback, void *if_context,
		 jmodid_t com_id)
{
	return _jn_proto1_com(conn, if_callback, if_context, com_id,
			      JNT_COMCLOSE);
}

int jn_parse_comlist(uchar * buf, uint32_t bufsize, jn_comod_entry ** list)
{
	uint32_t pos;
	size_t len;
	jmodid_t id;
	jobjid_t objid;
	jn_comod_entry *entry, *last = NULL;
	int ret = 0;

	if (buf[bufsize - 1] != '\0')
		return -JE_INV;

	*list = NULL;
	pos = 0L;
	while (pos < bufsize) {
		objid = unpacki32(buf + pos);
		pos += sizeof(jobjid_t);
		id = unpacki16(buf + pos);
		pos += sizeof(jmodid_t);
		len = strlen((char *)buf + pos);
		len++;

		entry = (jn_comod_entry *) malloc(sizeof(jn_comod_entry));
		entry->next = NULL;
		entry->copyname = (wchar_t *) malloc(len * sizeof(wchar_t));
		jcstow(buf + pos, entry->copyname, 0);
		entry->com_id = id;
		entry->objid = objid;

		pos += len;

		if (!(*list)) {
			*list = entry;
			last = *list;
		} else {
			last->next = entry;
			last = last->next;
		}
	}

	if (ret < 0) {
		if (*list) {
			while (*list) {
				entry = *list;
				*list = (*list)->next;
				free(entry->copyname);
				free(entry);
			}
		}
	}

	return ret;
}

int jn_parse_modlist(uchar * buf, uint32_t bufsize, jn_mod_entry ** list)
{
	uint32_t pos;
	size_t len;
	jobjid_t id;
	jn_mod_entry *entry, *last = NULL;
	int ret = 0;

	if (buf[bufsize - 1] != '\0')
		return -JE_INV;

	_wdeb1(L"parsing mod list, bufsize = %u", bufsize);

	*list = NULL;
	pos = 0L;

	while (pos < bufsize) {

		id = unpacki32(buf + pos);
		pos += sizeof(jobjid_t);
		len = strlen((char *)buf + pos);
		len++;

		entry = (jn_mod_entry *) malloc(sizeof(jn_mod_entry));
		entry->next = NULL;
		entry->pathname = (wchar_t *) malloc(len * sizeof(wchar_t));
		jcstow(buf + pos, entry->pathname, 0);
		entry->objid = id;

		pos += len;

		if (!(*list)) {
			*list = entry;
			last = *list;
		} else {
			last->next = entry;
			last = last->next;
		}
	}

	if (ret < 0) {
		if (*list) {
			while (*list) {
				entry = *list;
				*list = (*list)->next;
				free(entry->pathname);
				free(entry);
			}
		}
	}
	return ret;
}

void _jn_recv_buflist_progress(jn_cb_entry * cb_entry)
{

	void (*if_callback) (jn_cb_entry *);
	jn_pkt *pkt = cb_entry->pkt;
	//jn_conn* conn = (jn_conn*)cb_entry->conn;

	/*
	   if(cb_entry->nbytes == 0L){
	   cb_entry->buf = NULL;
	   }
	 */

	if ((cb_entry->nbytes == 0L) && (pkt->hdr.data_len == 0L)) {
		_wdeb(L"Expected Data!!!");
		pkt->hdr.type = JNT_FAIL;
		if (!(pkt->hdr.flags & JNF_ERR)) {
			pkt->hdr.rs = (uint32_t) - JE_FORBID;
			pkt->hdr.flags |= JNF_ERR;
		}
	}

	if (pkt->hdr.type != JNT_OK) {
		/*
		   if(pkt->hdr.data_len){
		   _wdeb(L"freeing data");
		   free(pkt->data);
		   }
		   _wdeb(L"freeing packet");
		   free(pkt);
		 */
		//_wdeb(L"recieved not-ok, type = %x", pkt->hdr.type);
		if (!(pkt->hdr.flags & JNF_ERR)) {
			if (pkt->hdr.rs != (uint32_t) - JE_FORBID)
				pkt->hdr.rs = (uint32_t) - JE_UNK;
			pkt->hdr.flags |= JNF_ERR;
		}

		cb_entry->jerr = (int)pkt->hdr.rs;
		if (cb_entry->if_callback) {
			if_callback = cb_entry->if_callback;
			if_callback(cb_entry);
		}

		return;

	}

	assert(pkt->hdr.data_len);

	cb_entry->nremain = pkt->hdr.rs;

	if ((cb_entry->nbytes == 0L) && (pkt->hdr.data_len)) {
		_wdeb2(L"allocating %u bytes", pkt->hdr.rs + pkt->hdr.data_len);
		cb_entry->buf = (char *)malloc(pkt->hdr.rs + pkt->hdr.data_len);
	}

	if (pkt->hdr.data_len) {
		_wdeb2(L"nbytes = %u, data_len = %u", cb_entry->nbytes,
		       pkt->hdr.data_len);
		memcpy(cb_entry->buf + cb_entry->nbytes, pkt->data,
		       pkt->hdr.data_len);
		cb_entry->nbytes += pkt->hdr.data_len;
		free(pkt->data);
	}

	if (!pkt->hdr.rs && cb_entry->if_callback) {
		//remove call back request on completion                
		cb_entry->jerr = 0;
		if_callback = cb_entry->if_callback;
		if_callback(cb_entry);
	}

	free(pkt);
}

int jn_recv_list(jn_conn * conn, void *if_callback, void *if_context,
		 uchar type)
{
	jn_pkt pkt;
	//jtransid_t transid;
	int ret;
	jn_cb_entry *q;

	_jn_assm_proto1_pkt(&pkt, 0, type);

	//_jn_dump_cbfifo(conn->cbfifo);

	pkt.hdr.trans_id = _jn_transa_get(conn->trans_a);

	//_jn_dump_cbfifo(conn->cbfifo);

	//_wdeb(L"interface callback addr: 0x%x", buf_s->if_cb);

	//buf_s->buf = NULL;

	q = jn_assm_cb(conn, JN_CB_TID, pkt.hdr.trans_id, 0,
		       (void *)&_jn_recv_buflist_progress,
		       NULL, if_callback, if_context);

	_jn_dump_cbfifo(conn->cbfifo);

	if (!q) {
		_jn_transa_rm(conn->trans_a, pkt.hdr.trans_id);
		return -JE_MALOC;
	}

	if ((ret = jn_reg_cb(conn, &pkt, q)) < 0) {
		free(q);
		return ret;
	}

	_jn_dump_cbfifo(conn->cbfifo);

	return 0;
}
