#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

#define JDB_JRNL_ALLOC_CHUNK	100

#include "jdb.h"

void* _jdb_jrnl_thread(void* arg){
	struct jdb_handle* h = (struct jdb_handle*)arg;
	int oldcancelstate;
	struct jdb_jrnl_fifo_entry* entry;
	jdb_bid_t i, pos;
	
	//EXIT: if !bufsize
	
	while(1){
		_wait_sem(&h->jsem);
		_lock_mx(&h->jmx);
		
		entry = h->jrnl_fifo.first;
		h->jrnl_fifo.first = h->jrnl_fifo.first->next;
		
		_unlock_mx(&h->jmx);
		
		if(!entry->bufsize) break;
		
		//_lock_mx(&h->rdmx);
		
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldcancelstate);
		
		//write here		
		
		fwrite((void*)entry->buf, entry->bufsize, 1, h->jf);
		fflush(h->jf);
		
		pthread_setcancelstate(oldcancelstate, NULL);	
		//_unlock_mx(&h->rdmx);
		
		if(entry->bufsize){			
			free(entry->buf);
		}
		free(entry);		

		
	}
	
	return NULL;	
}

int _jdb_init_jrnl_thread(struct jdb_handle* h){
	
	int ret;

	pthread_attr_t tattr;

#ifndef NDEBUG
	jmxattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif

#ifndef NDEBUG
	_init_mx(&h->jmx, &attr);
//	_init_mx(&h->rdmx, &attr);
#else
	_init_mx(&h->jmx, NULL);
//	_init_mx(&h->rdmx, NULL);
#endif

	h->jopid = 0UL;

	_init_sem(&h->jsem);
	
	pthread_attr_init(&tattr);
	pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);

	h->jrnl_fifo.first = NULL;

	if (_create_thread(&(h->jrnlthid), &tattr, _jdb_jrnl_thread, (void *)h)
	    < 0) {
		ret = -JE_THREAD;		
	}

	pthread_attr_destroy(&tattr);

	return ret;
		
}

int _jdb_request_jrnl_thread_exit(struct jdb_handle* h){
	struct jdb_jrnl_fifo_entry* entry;

	entry = (struct jdb_jrnl_fifo_entry*)malloc(sizeof(struct jdb_jrnl_fifo_entry));
	if(!entry) return -JE_MALOC;	
	
	entry->bufsize = 0UL;
	entry->next = NULL;

	_lock_mx(&h->jmx);
	
	if(!h->jrnl_fifo.first){
		h->jrnl_fifo.first = entry;
		h->jrnl_fifo.last = entry;
		h->jrnl_fifo.cnt = 1UL;
	} else {
		h->jrnl_fifo.last->next = entry;
		h->jrnl_fifo.last = entry;
		h->jrnl_fifo.cnt++;
	}
	
	_post_sem(&h->jsem);
	_unlock_mx(&h->jmx);
	
	return 0;

		
}

int _jdb_request_jrnl_write(struct jdb_handle* h, size_t bufsize, uchar* buf){
	struct jdb_jrnl_fifo_entry* entry;
	
	int ret = 0, ret2;	

	entry = (struct jdb_jrnl_fifo_entry*)malloc(sizeof(struct jdb_jrnl_fifo_entry));
	if(!entry) return -JE_MALOC;

	entry->bufsize = bufsize;
	entry->buf = buf;
	
	entry->next = NULL;	
	
	_lock_mx(&h->jmx);
	
	if(!h->jrnl_fifo.first){
		h->jrnl_fifo.first = entry;
		h->jrnl_fifo.last = entry;
		h->jrnl_fifo.cnt = 1UL;
	} else {
		h->jrnl_fifo.last->next = entry;
		h->jrnl_fifo.last = entry;
		h->jrnl_fifo.cnt++;
	}
	
	_post_sem(&h->jsem);
	_unlock_mx(&h->jmx);
	
	return 0;
}




static inline char *_jdb_jrnl_filename(struct jdb_handle *h,
					   wchar_t * filename)
{
	wchar_t* jfile;
	char* jcfile;
 
	jfile = (wchar_t *) malloc(WBYTES(filename) + WBYTES(JDB_DEF_JRNL_EXT));	
	
	if (!jfile)
		return NULL;
	
 
	wcscpy(jfile, filename);	
	wcscat(jfile, JDB_DEF_JRNL_EXT);
	
	jcfile = (char*)malloc(WBYTES(jfile));
	wcstombs(jcfile, jfile, WBYTES(jfile));
	
	free(jfile);	
	
	return jcfile;
}


 
int _jdb_jrnl_open(struct jdb_handle *h, wchar_t * filename, uint16_t flags)
{
 
	char* jfilename;
 
	jfilename = _jdb_jrnl_filename(h, filename);
	
	if (!jfilename)
		return -JE_MALOC;
	
	h->jf = fopen(jfilename, "w+");
	
	free(jfilename);
	
	if(!h->jf) return -JE_NOOPEN;
	
	return 0;

}

static inline uint32_t _jdb_get_jopid(struct jdb_handle* h){
	uint32_t opid;
	
	_lock_mx(&h->jmx);
	opid = h->jopid;
	h->jopid++;
	_unlock_mx(&h->jmx);
	
	return opid;
}		

int _jdb_jrnl_close(struct jdb_handle *h)
{
	fflush(h->jf);
	fclose(h->jf);
	
	return 0;
}

int _jdb_jrnl_end_op(struct jdb_handle* h, uint32_t* opid){
	size_t bufsize = sizeof(uint32_t)+sizeof(uchar);
	uchar* buf = (uchar*)malloc(bufsize);
	if(!buf) return -JE_MALOC;
	
	memcpy(buf, opid, sizeof(uint32_t));
	buf[sizeof(uint32_t)] = JDB_CMD_END;
	
	return 0;
}

 
int _jdb_jrnl_reg_open(struct jdb_handle *h, wchar_t * filename,
			  uint16_t flags, uint32_t* opid)
{
	return -JE_IMPLEMENT;
}


 
int _jdb_jrnl_reg_close(struct jdb_handle *h, uint32_t* opid)
{
	return -JE_IMPLEMENT;

}


 
int _jdb_jrnl_reg_create_table(struct jdb_handle *h, wchar_t * name,
				  uint32_t nrows, uint32_t ncols, uchar flags
				  , uint32_t* opid)
{
	return -JE_IMPLEMENT;

}


 
int _jdb_jrnl_reg_rm_table(struct jdb_handle *h, wchar_t * name, uint32_t* opid)
{
	return -JE_IMPLEMENT;

}


 
int _jdb_jrnl_reg_add_typedef(struct jdb_handle *h, struct jdb_table *table,
				 uchar type_id, uchar base, uint32_t len,
				 uchar flags, uint32_t* opid)
{
	return -JE_IMPLEMENT;

}


 
int _jdb_jrnl_reg_rm_typedef(struct jdb_handle *h, struct jdb_table *table,
				uchar type_id, uint32_t* opid)
{
	return -JE_IMPLEMENT;
}


 
int _jdb_jrnl_reg_assign_col_type(struct jdb_handle *h,
				     struct jdb_table *table, uchar type_id,
				     uint32_t col, uint32_t* opid)
{
	return -JE_IMPLEMENT;
}


 
int _jdb_jrnl_reg_create_cell(struct jdb_handle *h, wchar_t * table,			 
				uint32_t col, uint32_t row, 
				uchar * data,
				uint32_t datalen, 
				uchar data_type, int unsign, uint32_t* opid)
{ 
	return -JE_IMPLEMENT;
}


 
 
