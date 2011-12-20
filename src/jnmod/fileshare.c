#include "jnet.h"
#include "jtypes.h"
#include "jcs.h"
#include "jer.h"
#include "debug.h"

#include <assert.h>
#include <wchar.h>
#include <string.h>

#ifndef _SVID_SOURCE
#define _SVID_SOURCE
#endif
#include <dirent.h>

#define FS_PATH	"."
#define FS_LIST_CMD	"list"
#define FS_CANCEL_CMD	"cancel"
#define FS_TEST_FILE	"test"

#define _wdeb1 _wdeb

static inline void _fs_assm_rpkt(jn_pkt* req_pkt, jn_pkt** rpkt, uchar* msg){
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

//returns number of packets
int _fs_assm_list_rpkt(jn_pkt* req_pkt, jn_pkt** rpkt, uchar* msg, size_t msgsize){
	int ret = JN_NPKT(msgsize);
	int cnt;
	*rpkt = (jn_pkt*)malloc(sizeof(jn_pkt)*ret);
	
	assert(req_pkt);
			
	for(cnt = 0; cnt < ret; cnt++){
	
		memcpy(&(*rpkt[cnt].hdr), &req_pkt->hdr, sizeof(jn_hdr));
		(*rpkt)->hdr.type = JNT_COMREQ;
		(*rpkt)->hdr.rs = JN_RS_BY_CNT(msgsize, cnt);
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
	return ret;
}

int _fs_dir(char* path, char** buf, size_t* bufsize){
	struct dirent **namelist;
	int n;
	int ret;
	wchar_t name[JN_MAXPATHNAME];
	
	n = scandir(path, &namelist, 0, alphasort);
	if(n < 0){
		_wdeb(L"scandir() fails");
		return -JE_SYSE;
	}

	*bufsize = 0UL;
	for(ret = 0; ret < n; ret++){
		*bufsize += strlen(namelist[n]->d_name) + 1;
	}

	*buf = (char*)malloc(*bufsize);
	
	ret = 0;
	while(n--){
		memcpy(*buf + ret, namelist[n]->d_name, ret+=strlen(namelist[n]->d_name));
		_wdeb1(L"pos = %i", ret);
	}
	
	return 0;
	
}

static inline jn_fifo_entry* _fs_assm_job(jn_comod_entry* com, jn_pkt* pkt){
	jn_fifo_entry* job;
	job = (jn_fifo_entry*)malloc(sizeof(jn_fifo_entry));
	job->flags = JN_FF_FREE_DATA;
	job->type = JN_FT_PKT;
	job->datalen = sizeof(jn_hdr) + pkt->hdr.data_len;
	job->cc_list = com->cc_list;
	job->ptr = (void*)pkt;
	
	return job;
}

static inline uchar* _fs_assm_filelist(jn_comod_entry* com){

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
	//uchar* join_msg = _schat_assm_msg(conn->username, SCHAT_JOIN_MSG, NULL);;
	jn_cc_entry* entry;
	int mpkt = 0;
	
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
	
	_jn_com_req_inc(com);
	
	if(pkt->hdr.datalen){
		pkt->data[pkt->hdr.datalen] = '\0';
		if(!strcmp(pkt->data, FS_LIST_CMD)){
			_fs_send_list(h, 
		} else if(!strcmp(pkt->data, FS_CANCEL_CMD)){
			_fs_cancel_trans(
		}
	}
	
	return 0;
}

int jnmod_cleanup(void* vh, void* vcom_h, void* vreply_fn){
	
	return 0;
}

