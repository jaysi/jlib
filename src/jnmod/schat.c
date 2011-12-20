#include "jnet.h"
#include "jtypes.h"
#include "jcs.h"
#include "jer.h"
#include "debug.h"

#include <assert.h>

#define _wdeb1(f, a...)

typedef struct SimpleChatStruct{
	int fd;
	uchar* name;//jcs encoded
	struct SimpleChatStruct* next;
}schat_s;

#define SCHAT_MAX_MSG8	1024

#define SCHAT_JOIN_MSG " has joined the COMOD"
#define SCHAT_LEAVE_MSG " has left the COMOD" 
#define SCHAT_SEP_MSG	" Says: "
#define SCHAT_LIST_CMD	"list"
#define SCHAT_FAIL_MSG	"Internal Error"

static inline jn_fifo_entry* _schat_assm_job(jn_comod_entry* com, jn_pkt* pkt){
	jn_fifo_entry* job;
	job = (jn_fifo_entry*)malloc(sizeof(jn_fifo_entry));
	if(!job) return NULL;
	job->flags = JN_FF_FREE_DATA;
	job->type = JN_FT_PKT;
	job->datalen = sizeof(jn_hdr) + pkt->hdr.data_len;
	job->cc_list = com->cc_list;
	job->ptr = (void*)pkt;
	
	return job;
}

static inline jn_fifo_entry* _schat_assm_list_job(jn_conn* conn, jn_pkt* pkt){
	jn_fifo_entry* job;
	job = (jn_fifo_entry*)malloc(sizeof(jn_fifo_entry));
	if(!job) return NULL;
	job->flags = JN_FF_FREE_DATA | JN_FF_FREE_CC_LIST;
	job->type = JN_FT_PKT;
	job->datalen = sizeof(jn_hdr) + pkt->hdr.data_len;
	job->cc_list = _jn_cclist_new();
	if(!job->cc_list){
		free(job);
		return NULL;
	}
	_jn_cclist_add(job->cc_list, conn);
	job->ptr = (void*)pkt;
	
	return job;
}

static inline uchar* _schat_assm_msg(wchar_t* uname, uchar* msg0, uchar* msg1){
	uchar* ret;
	uint16_t uname_jcslen = wtojcs_len(uname, MAX_UNAME);
	
	if(msg1){
		ret = (uchar*)malloc(uname_jcslen + strlen(msg0) + CBYTES(msg1));
	} else {
		ret = (uchar*)malloc(uname_jcslen + CBYTES(msg0));
	}
	
	wtojcs(uname, ret, MAX_UNAME);
	strcat(ret, msg0);
	if(msg1){
		strcat(ret, msg1);
	}
	
	return ret;
	
}

static inline void _schat_assm_rpkt(jn_pkt* req_pkt, jn_pkt** rpkt, uchar* msg){
	*rpkt = (jn_pkt*)malloc(sizeof(jn_pkt));
	
	assert(req_pkt);
	
	memcpy(&(*rpkt)->hdr, &req_pkt->hdr, sizeof(jn_hdr));
	(*rpkt)->hdr.type = JNT_COMREQ;
	(*rpkt)->hdr.rs = 0L;
	if(msg){
		(*rpkt)->hdr.data_len = CBYTES(msg);
		(*rpkt)->data = msg;
		(*rpkt)->hdr.flags = JNF_PLAIN;
	} else {
		(*rpkt)->hdr.flags = 0x00;
		(*rpkt)->hdr.data_len = 0L;
	}
	
	_wdeb1(L"rpkt->data (jcs): %s", (*rpkt)->data);
}

void _schat_assm_list_rpkt(jn_pkt* req_pkt, jn_pkt** rpkt, uchar* msg, size_t msgsize){
	*rpkt = (jn_pkt*)malloc(sizeof(jn_pkt));
	
	assert(req_pkt);
	
	memcpy(&(*rpkt)->hdr, &req_pkt->hdr, sizeof(jn_hdr));
	(*rpkt)->hdr.type = JNT_COMREQ;
	(*rpkt)->hdr.rs = 0L;
	if(msg){
		(*rpkt)->hdr.data_len = msgsize;
		(*rpkt)->data = msg;
		(*rpkt)->hdr.flags = JNF_PLAIN;
	} else {
		(*rpkt)->hdr.flags = 0x00;
		(*rpkt)->hdr.data_len = 0L;
	}
	
	_wdeb1(L"rpkt->data (jcs): %s", (*rpkt)->data);	
}

uint16_t jnmod_magic(){
	//returns magic
	return JNMOD_MAGIC;
}

int jnmod_init(void* vh, void* vcom_h, void* vconn){
	jn_h* h = (jn_h*)vh;
	jn_comod_entry* com = (jn_comod_entry*)vcom_h;

	com->flags = JN_COM_ANNOUNCE_DC | JN_COM_SINGLE_CC_LIST;
	
	com->com_data = NULL;
			
	return 0;
}

int jnmod_handle_join(void* vh, void* vcom_h, void* vconn, void* req_pkt, void* vreply_fn){
	jn_h* h = (jn_h*)vh;
	jn_comod_entry* com = (jn_comod_entry*)vcom_h;
	jn_conn* conn = (jn_conn*)vconn;
	void (*reply)(jn_fifo*, jn_fifo_entry*) = vreply_fn;
	jn_pkt* pkt;
	jn_pkt* rpkt;
	jn_fifo_entry* job;
	uchar* join_msg = _schat_assm_msg(conn->username, SCHAT_JOIN_MSG, NULL);;
	jn_cc_entry* entry;
	int mpkt = 0;

	if(req_pkt){
		pkt = (jn_pkt*)req_pkt;		
	} else {
		pkt = (jn_pkt*)malloc(sizeof(jn_pkt));
		pkt->hdr.trans_id = 0L;
		mpkt = 1;
	}
	
	pkt->hdr.comod_id = com->com_id;

	/* already added to cc_list, anyway you may handle it here
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
	
	_wdeb1(L"JOIN MSG (jcs): %s", join_msg);
	
	_schat_assm_rpkt(pkt, &rpkt, join_msg);
	if(mpkt) free(pkt);
	
	job = _schat_assm_job(com, rpkt);
	
	reply(com->reply_fifo, job);
			
	return 0;
}

int jnmod_handle_leave(void* vh, void* vcom_h, void* vconn, void* req_pkt, void* vreply_fn){
	jn_h* h = (jn_h*)vh;
	jn_comod_entry* com = (jn_comod_entry*)vcom_h;
	jn_conn* conn = (jn_conn*)vconn;
	void (*reply)(jn_fifo*, jn_fifo_entry*) = vreply_fn;
	jn_pkt* pkt;
	jn_pkt* rpkt;
	jn_fifo_entry* job;
	int mpkt = 0;
	uchar* join_msg = _schat_assm_msg(conn->username, SCHAT_LEAVE_MSG, NULL);
	
	_wdeb(L"called, %S is leaving", conn->username);
	
	if(req_pkt) pkt = (jn_pkt*)req_pkt;
	else{
		pkt = (jn_pkt*)malloc(sizeof(jn_pkt));
		pkt->hdr.trans_id = 0L;
		mpkt = 1;
		
	}
	
	pkt->hdr.comod_id = com->com_id;

	_schat_assm_rpkt(pkt, &rpkt, join_msg);	
	if(mpkt) free(pkt);
	
	job = _schat_assm_job(com, rpkt);	
	
	reply(com->reply_fifo, job);
	
	return 0;
}

int jnmod_handle_packet(void* vh, void* vcom_h, void* vjob, void* vreply_fn){
	jn_h* h = (jn_h*)vh;
	jn_comod_entry* com = (jn_comod_entry*)vcom_h;
	jn_fifo_entry* job = (jn_fifo_entry*)vjob;
	jn_fifo_entry* rjob;
	void (*reply)(jn_fifo*, jn_fifo_entry*) = vreply_fn;
	jn_pkt* pkt = (jn_pkt*)job->ptr;	
	jn_pkt* rpkt;
	uchar* msg;
	size_t msgsize;
	size_t npkt, i;
	jcslist_entry* jcslist;
	
	_wdeb1(L"packet->data (jcs): %s", pkt->data);
	
	//assert packet
	if(job->type != JN_FT_PKT){
		_wdeb1(L"failing here, bad job type");
		return -JE_TYPE;
	}
	if(!isjcs(pkt->data, SCHAT_MAX_MSG8)){
		_wdeb1(L"failing here, bad format");
		return -JE_INV;
	}
	
	if(!strcmp(pkt->data, SCHAT_LIST_CMD)){ //create a list of users and
						//send to the conn
		jn_conn_cclist_to_jcslist(com->cc_list, &jcslist);
		jcslist_buf(jcslist, &msg, &msgsize, MAX_UNAME);
		
		_schat_assm_list_rpkt(pkt, &rpkt, msg, msgsize);
		
		rjob = _schat_assm_list_job(job->conn, rpkt);
		
		reply(com->reply_fifo, rjob);
		
		return 0;
	}
		
	//assemble rpkt (sender name + message)
	msg = _schat_assm_msg(job->conn->username, SCHAT_SEP_MSG, pkt->data);
	
	_wdeb1(L"reply message (jcs): %s", msg);
	
	_schat_assm_rpkt(pkt, &rpkt, msg);
	
	rjob = _schat_assm_job(com, rpkt);
	
	reply(com->reply_fifo, rjob);
	
	_jn_com_req_dec(com); //from _jn_proc_comreq() call
		
	return 0;	
}

int jnmod_cleanup(void* vh, void* vcom_h, void* vreply_fn){
	
	return 0;
}

