#include "jnet.h"
#include "jtypes.h"
#include "jcs.h"
#include "jer.h"
#include "debug.h"
#include "jnet_internals.h"

#define _wdeb1(f, a...)
#define _wdeb2(f, a...) //plain packet data dump

#define ECHO_MAX_MSG8	256

static inline int _echo_assm_plain(jn_pkt* pkt,uchar type, wchar_t* msg, int16_t err_code){
	pkt->hdr.flags |= JNF_PLAIN;
	pkt->hdr.type = type;
	pkt->hdr.data_len = 0;
	pkt->hdr.rs = 0L;
	
	if(err_code){
		_wdeb1(L"has error code");
		if(msg){
			_wdeb1(L"allocating for msg");
			pkt->hdr.data_len = sizeof(int16_t)+wtojcs_len(msg, 0)+1;
			if(pkt->hdr.data_len > JN_MAX_PKT - sizeof(jn_hdr)) return -JE_SIZE;
#ifndef NO_HEAP
			pkt->data = (uchar*)pmalloc(pkt->hdr.data_len);
#endif
			packi16(pkt->data, err_code);
			wtojcs(msg, pkt->data+sizeof(int16_t), 0);
		} else {
			_wdeb1(L"allocating for err code");
			pkt->hdr.data_len = sizeof(int16_t);
#ifndef NO_HEAP
			pkt->data = (uchar*)pmalloc(pkt->hdr.data_len);
#endif
			packi16(pkt->data, err_code);
		}
	}
	
	return 0;
}

static inline jn_cc_list* _echo_prepare_cc(jn_conn* conn){
	jn_cc_list* cc_list;
	
	cc_list = (jn_cc_list*)malloc(sizeof(jn_cc_list));
	_init_mx(&cc_list->mx, NULL);
	cc_list->cnt = 1L;
	cc_list->first = (jn_cc_entry*)malloc(sizeof(jn_cc_entry));
	cc_list->first->conn = conn;
	cc_list->last = cc_list->first;
	cc_list->first->next = NULL;
	
	return cc_list;
}

static inline jn_fifo_entry* _echo_assm_job(jn_comod_entry* com, jn_conn* conn, jn_pkt* pkt){
	jn_fifo_entry* job;
	job = (jn_fifo_entry*)malloc(sizeof(jn_fifo_entry));
	job->flags = JN_FF_FREE_CC_LIST;
	
	if(pkt->hdr.type != JNT_FAIL){
		job->flags |= JN_FF_FREE_DATA;
	}
	
	job->type = JN_FT_PKT;
	job->datalen = sizeof(jn_hdr) + pkt->hdr.data_len;
	job->cc_list = _echo_prepare_cc(conn);
	job->ptr = pkt;
	
	return job;
}

uint16_t jnmod_magic(){
	return JNMOD_MAGIC;
}

int jnmod_init(void* vh, void* vcom_h, void* vconn){
	jn_h* h = (jn_h*)vh;
	jn_comod_entry* com = (jn_comod_entry*)vcom_h;

	com->flags = 0x00;//JN_COM_ANNOUNCE_DC | JN_COM_SINGLE_CC_LIST;
	
	com->com_data = NULL;
			
	return 0;
}

int jnmod_handle_join(void* vh, void* vcom_h, void* vconn, void* req_pkt, void* vreply_fn){
	jn_h* h = (jn_h*)vh;
	jn_comod_entry* com = (jn_comod_entry*)vcom_h;
	jn_conn* conn = (jn_conn*)vconn;
	jn_cc_entry* entry;

/*
	com->cc_list->cnt++;
	entry = (jn_cc_entry*)malloc(sizeof(jn_cc_entry));
	entry->conn = conn;
	entry->next = NULL;
	
	if(!(com->cc_list->first)){
		com->cc_list->first = entry;
		com->cc_list->last = entry;
	} else {
		com->cc_list->last->next = entry;
		com->cc_list->last = com->cc_list->last->next;
	}
*/
	return 0;
}

int jnmod_handle_leave(void* vh, void* vcom_h, void* vconn, void* req_pkt, void* vrqueue_fn){
	
	return 0;
}

int jnmod_handle_packet(void* vh, void* vcom_h, void* vjob, void* vreply_fn){
	jn_h* h = (jn_h*)vh;
	jn_comod_entry* com = (jn_comod_entry*)vcom_h;
	jn_fifo_entry* job = (jn_fifo_entry*)vjob;
	jn_fifo_entry* rjob;
	void (*reply)(jn_fifo*, jn_fifo_entry*) = vreply_fn;
	jn_pkt* pkt = (jn_pkt*)(job->ptr);
	jn_pkt* rpkt;
	uchar* msg;
	jn_cc_entry* entry;
	int ret;

/*
#ifndef NDEBUG
	if(pkt->hdr.flags & JNF_PLAIN){
		_wdeb2(L"jcs data: %s", pkt->data);
	}
#endif
*/

	
	//assert packet
	if(job->type != JN_FT_PKT){
		_wdeb1(L"bad job type 0x%x", job->type);
		rpkt = (jn_pkt*)malloc(sizeof(jn_pkt));
		_echo_assm_plain(rpkt, JNT_FAIL, NULL, -JE_TYPE);
		rpkt->hdr.trans_id = pkt->hdr.trans_id;		
		ret = -JE_TYPE;
	}
	
	
	if(!isjcs(pkt->data, ECHO_MAX_MSG8)){
		_wdeb1(L"bad packet data, jcs dump: %s", pkt->data);
		rpkt = (jn_pkt*)malloc(sizeof(jn_pkt));
		_echo_assm_plain(rpkt, JNT_FAIL, NULL, -JE_INV);
		rpkt->hdr.trans_id = pkt->hdr.trans_id;
		ret = -JE_INV;
	} else {
		rpkt = pkt;
		ret = 0;
	}	
	
	rjob = _echo_assm_job(com, job->conn, rpkt);
	
	reply(com->reply_fifo, rjob);
	
	_jn_com_req_dec(com); //from _jn_proc_comreq() call
	
	return ret;	
}

int jnmod_cleanup(void* vh, void* vcom_h, void* vrqueue_fn){
	
	return 0;
}

