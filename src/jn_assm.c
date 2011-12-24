#include "jnet.h"
#include "jer.h"
#include "ecc.h"
#include "debug.h"
#include "jcs.h"
#include "jrand.h"
#include "jhash.h"

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

#define _wdeb1(f, a...)
#define _wdeb2(f, a...)		//pkt->hdr.eno track
#define _wdeb3(f, a...)		//new hdr assm, disassm
#define _wdeb4 _wdeb		//cb tracker
#define _wdeb5 _wdeb		//jn_send_plain() tracker

void _jn_assm_hdr(uchar * buf, uchar * data, uchar flags, uchar type,
		  uint16_t datalen, uint16_t comod_id, uchar trans_id,
		  uint32_t rs)
{

	assert(sizeof(jn_hdr) == AES_BSIZE);

	//if((flags & JNF_PLAIN) && (!datalen)) return -JE_INV;

	pack(buf, "hcchhclhc", JN_PKT_MAGIC,
	     JN_VER | (flags & 0x0f),
	     type, datalen,
	     comod_id, trans_id,
	     rs, (datalen == 0) ? 0 : _jn_crc(data, datalen), 0x00);
#ifndef NDEBUG
	_wdeb1(L"datalen = %u; datacrc = 0x%x", datalen,
	       _jn_crc(data, datalen));
#endif

	buf[AES_BSIZE - 1] = pearson(buf, AES_BSIZE);

//      _wdeb(L"hcrc = 0x%x", buf[AES_BSIZE-1]);

}

int _jn_disassm_hdr(jn_hdr * hdr, uchar * buf)
{

	uchar hcrc;
	uchar calchcrc;
#ifndef NDEBUG
	int ret = 0;
#endif

	assert(sizeof(jn_hdr) == AES_BSIZE);

	_wdeb1(L"hcrc = 0x%x", buf[AES_BSIZE - 1]);

	hcrc = buf[AES_BSIZE - 1];
	buf[AES_BSIZE - 1] = 0x00;

	calchcrc = pearson(buf, AES_BSIZE);

	if (calchcrc != hcrc) {
#ifndef NDEBUG
		ret = -JE_HCRC;
#else
		return -JE_HCRC;
#endif
	}

	hdr->magic = unpacki16(buf);
	hdr->ver = buf[sizeof(uint16_t)] << 4;
	hdr->flags = buf[sizeof(uint16_t)];
	unpack(buf + sizeof(uint16_t) + sizeof(uchar), "chhclh",
	       &(hdr->type), &(hdr->data_len),
	       &(hdr->comod_id), &(hdr->trans_id),
	       &(hdr->rs), &(hdr->data_crc));
	hdr->hdr_crc = hcrc;

	/*
	   _wdeb3(L"M:0x%x, V:0x%x, F:0x%x, T:0x%x, L:0x%x, C:%i, I:%i, R:%l, D:0x%x, H:0x%x",
	   hdr->magic, hdr->ver, hdr->flags, hdr->type, hdr->data_len,
	   hdr->comod_id, hdr->trans_id, hdr->rs, hdr->data_crc,
	   hdr->hdr_crc);
	 */

	_wdeb1(L"datalen = %u, datacrc = 0x%x", hdr->data_len, hdr->data_crc);

#ifndef NDEBUG
	return ret;
#else
	return 0;
#endif

}

jn_cb_entry *jn_assm_cb(jn_conn * conn, uchar flags, jtransid_t tid,
			jmodid_t comid, void *progress_callback,
			void *progress_context, void *if_callback,
			void *if_context)
{

	jn_cb_entry *entry = (jn_cb_entry *) malloc(sizeof(jn_cb_entry));
	if (!entry)
		return NULL;

	entry->flags = flags;
	entry->tid = tid;
	entry->comid = comid;

	entry->nbytes = 0L;
	entry->nremain = 0L;
	entry->buf = NULL;
	entry->conn = (void *)conn;

	entry->jerr = 0;
	entry->progress_callback = progress_callback;
	entry->progress_context = progress_context;
	entry->if_callback = if_callback;
	entry->if_context = if_context;

	entry->next = NULL;

	_wdeb4
	    (L"CB assembled, tid = %u, comid = %u, flags = 0x%x,  pcb = 0x%p, pctx = 0x%p, icb = 0x%p, ictx = 0x%p",
	     tid, comid, flags, progress_callback, progress_context,
	     if_callback, if_context);

	return entry;

}

void _jn_free_cb(jn_cb_entry * cb)
{

	if (cb->pkt) {
		if (cb->pkt->hdr.data_len) {
			free(cb->pkt->data);
		}
		free(cb->pkt);
	}
	if (!(cb->flags & JN_CB_CONTINUE) && !(cb->flags & JN_CB_KEEP) &&
	    !(cb->flags & JN_CB_NOFREE)) {

		free(cb);

	}

}

void _jn_assm_proto1_pkt(jn_pkt * pkt, jmodid_t modid, uchar type)
{
	pkt->hdr.flags &= ~JNF_PLAIN;
	pkt->hdr.type = type;
	pkt->hdr.comod_id = modid;
	pkt->hdr.data_len = 0L;
	pkt->hdr.rs = 0L;
}

static inline int _jn_assm_plain(jn_pkt * pkt, uchar type, wchar_t * msg,
				 int16_t err_code)
{
	if (err_code) {
		//_wdeb(L"setting error flag");
		pkt->hdr.flags |= (JNF_ERR | JNF_PLAIN);
		pkt->hdr.rs = err_code;
	} else {
		pkt->hdr.flags |= JNF_PLAIN;
		pkt->hdr.flags &= ~JNF_ERR;
		pkt->hdr.rs = 0;
	}
	pkt->hdr.type = type;
	pkt->hdr.data_len = 0;

	if (msg) {
		_wdeb1(L"allocating for msg");
		pkt->hdr.data_len = wtojcs_len(msg, 0) + 1;
		if (pkt->hdr.data_len > JN_MAX_PKT - sizeof(jn_hdr))
			return -JE_SIZE;
#ifndef NO_HEAP
		pkt->data = (uchar *) pmalloc(pkt->hdr.data_len);
		if (!pkt->data)
			return -JE_MALOC;
#endif
		wtojcs(msg, pkt->data, 0);
	}

	return 0;
}

int _jn_disassm_plain(uchar * buf, wchar_t * msg, uint16_t * msg_len)
{
	*msg_len = strlen((char *)buf);
	if (jcstow(buf, msg, JN_MAX_PLAIN_MSG) == ((size_t) - 1)) {
		return -JE_INV;
	}

	return 0;
}

#ifndef JN_SINGLE_CLT
int jn_send_plain(jn_conn * conn, jtransid_t trans_id, uchar type,
		  int16_t err_code, wchar_t * msg)
{
	jn_pkt pkt;
	int ret;

	switch (type) {
	case JNT_OK:		//plain & complex
	case JNT_TIMEOUT:	//plain
	case JNT_DC:		//plain
	case JNT_FAIL:		//plain
		break;
	default:
		return -JE_TYPE;
	}

	ret = _jn_assm_plain(&pkt, type, msg, err_code);
	if (ret < 0)
		return ret;

	pkt.hdr.trans_id = trans_id;
	if (conn->flags & JN_CF_SVR) {
		ret = jn_send_poll(conn, &pkt, JN_SEND_FREE);
	} else {		//@client side or in login phase
		ret = jn_send(conn, &pkt);
	}

	if (msg) {
		free(pkt.data);
	}
	return ret;
}
#endif

int _jn_assm_com_open(jn_pkt * pkt, uchar flags, jobjid_t objid, wchar_t * name)
{
	pkt->hdr.flags &= ~JNF_PLAIN;
	pkt->hdr.type = JNT_COMOPEN;
	pkt->hdr.data_len =
	    sizeof(uchar) + sizeof(jobjid_t) + wtojcs_len(name, 0) + 1;

	if (pkt->hdr.data_len > JN_MAX_PKT - sizeof(jn_hdr))
		return -JE_SIZE;

#ifndef NO_HEAP
	pkt->data = (uchar *) pmalloc(pkt->hdr.data_len);
	if (!pkt->data)
		return -JE_MALOC;
#endif

	pack(pkt->data, "cl", flags, objid);

	if (wtojcs
	    (name, pkt->data + sizeof(uchar) + sizeof(jobjid_t),
	     JN_MAX_COM_NAME) == ((size_t) - 1))
		return -JE_INV;
	return 0;

}

int _jn_disassm_com_open(jn_pkt * pkt, uchar * flags, jobjid_t * objid,
			 wchar_t ** name)
{
	unpack(pkt->data, "cl", flags, objid);

	if (pkt->hdr.data_len >=
	    (JN_MAX_COM_NAME + sizeof(uchar) + sizeof(jobjid_t)))
		return -JE_LIMIT;

	if (pkt->data[sizeof(uchar) + sizeof(jobjid_t)] == '\0')
		return -JE_INV;

	pkt->data[pkt->hdr.data_len] = '\0';
	*name = (wchar_t *)
	    malloc(CWBYTES(pkt->data + sizeof(uchar) + sizeof(jobjid_t)));
	if (!(*name))
		return -JE_MALOC;
	if (jcstow
	    (pkt->data + sizeof(uchar) + sizeof(jobjid_t), *name,
	     JN_MAX_COM_NAME) == ((size_t) - 1))
		return -JE_INV;
	return 0;
}

int jn_assm_plain(jn_pkt * pkt, jtransid_t trans_id, uchar type,
		  int16_t err_code, wchar_t * msg)
{
	int ret;
	switch (type) {
	case JNT_OK:		//plain & complex           
	case JNT_TIMEOUT:	//plain
	case JNT_DC:		//plain
	case JNT_FAIL:		//plain
		break;
	default:
		return -JE_TYPE;
	}

	ret = _jn_assm_plain(pkt, type, msg, err_code);
	if (ret < 0)
		return ret;

	pkt->hdr.trans_id = trans_id;

	return 0;
}
