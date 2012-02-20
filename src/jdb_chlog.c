#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

#define JDB_JRNL_ALLOC_CHUNK	100

#include "jdb.h"

#define _wdeb_start	_wdeb
#define _wdeb_pos	_wdeb
#define _wdeb_proc	_wdeb
#define _wdeb_reg	_wdeb

static inline void _jdb_jrnl_hdr_to_buf(struct jdb_changelog_hdr* hdr, uchar* buf){
	memcpy(buf, (void*)&hdr->recsize, sizeof(uint32_t));	
	memcpy(buf + sizeof(uint32_t), (void*)&hdr->chid, sizeof(uint64_t));
	memcpy(buf + sizeof(uint32_t) + sizeof(uint64_t),
		(void*)&hdr->cmd, sizeof(uchar));
	memcpy(buf + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uchar),
		(void*)&hdr->ret, sizeof(int));
	memcpy(buf + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uchar) + sizeof(int),
		(void*)&hdr->nargs, sizeof(uchar));
}

static inline void _jdb_jrnl_buf_to_hdr(uchar* buf, struct jdb_changelog_hdr* hdr){
	memcpy((void*)&hdr->recsize, buf, sizeof(uint32_t));
	memcpy((void*)&hdr->chid, buf + sizeof(uint32_t), sizeof(uint64_t));
	memcpy((void*)&hdr->cmd, buf + sizeof(uint32_t) + sizeof(uint64_t),
		sizeof(uchar));
	memcpy((void*)&hdr->ret,
		buf + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uchar),
		sizeof(int));
	memcpy((void*)&hdr->nargs,
		buf + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uchar) + sizeof(int),
		sizeof(uchar));
}

void* _jdb_jrnl_thread(void* arg){
	struct jdb_handle* h = (struct jdb_handle*)arg;
	int oldcancelstate;
	struct jdb_jrnl_fifo_entry* entry;
	
	//EXIT: if !bufsize
	
	_wdeb_start(L"journal thread started...");
	
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
		
		rc4(entry->buf + sizeof(uint32_t), entry->bufsize - sizeof(uint32_t), &h->rc4);
		
		write(h->jfd, (void*)entry->buf, entry->bufsize);
#ifndef _WIN32
		fsync(h->jfd);
#else
		_commit(h->jfd);
#endif
		
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

int _jdb_jrnl_request_write(struct jdb_handle* h, size_t bufsize, uchar* buf){
	struct jdb_jrnl_fifo_entry* entry;
	
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

static inline char *_jdb_jrnl_filename(	struct jdb_handle *h,
					wchar_t * filename, wchar_t* ext)
{
	wchar_t* jfile;
	char* jcfile;
 
	jfile = (wchar_t *) malloc(WBYTES(filename) + WBYTES(ext));	
	
	if (!jfile)
		return NULL;
	
	wcscpy(jfile, filename);	
	wcscat(jfile, ext);
	
	jcfile = (char*)malloc(WBYTES(jfile));
	if(!jcfile) { free(jfile); return NULL; }
	wcstombs(jcfile, jfile, WBYTES(jfile));
	
	free(jfile);	
	
	return jcfile;
}

int _jdb_jrnl_open(struct jdb_handle *h, wchar_t * filename, uint16_t flags)
{
 
	char* jfilename;
	int oflags;
	mode_t mode;
	int ret;
	
	if(!(h->hdr.flags & JDB_O_JRNL)) return 0;

	jfilename = _jdb_jrnl_filename(h, filename, JDB_DEF_JRNL_EXT);
	
	if (!jfilename)
		return -JE_MALOC;
		
	if (!access(jfilename, F_OK)){
		/*
		  the file exists, possibly from a previous CRASH!
		  try opening it, and check the journal, recovery is optional.
		*/
#ifdef _WIN32		
		oflags = O_RDONLY | O_BINARY;
#else
		oflags = O_RDONLY;
#endif

		if(flags & JDB_JRNL_TRUNC){
			oflags |= O_TRUNC;
		}

		mode = 0;
		ret = -JE_EXISTS;
	} else {
		/*
		  create a new file
		*/
#ifdef _WIN32		
		oflags = O_WRONLY | O_BINARY | O_CREAT;
#else
		oflags = O_WRONLY | O_CREAT;
#endif
		mode = S_IWRITE | S_IREAD;
		
		ret = 0;	
	}

	h->jfd = open(jfilename, oflags, mode);
	
	free(jfilename);
	
	if(h->jfd < 0) return -JE_NOOPEN;
	
	return ret;
}

int _jdb_jrnl_close(struct jdb_handle *h)
{
	char* jfilename;
#ifndef _WIN32
	fsync(h->jfd);
#else
	_commit(h->jfd);
#endif
	jfilename = _jdb_jrnl_filename(h, h->conf.filename, JDB_DEF_JRNL_EXT);
	unlink(jfilename);
	free(jfilename);
	close(h->jfd);
	
	return 0;
}

/*
static inline int _jdb_jrnl_get_rec_hdr(struct jdb_handle* h, struct jdb_changelog_hdr* hdr){	
	size_t red;
	
	if((red = hard_read(h->jfd, hdr, sizeof(struct jdb_changelog_hdr)) != 
		sizeof(struct jdb_changelog_hdr)) return -JE_READ;
	
	return 0;
}
*/

int _jdb_jrnl_get_rec(struct jdb_handle* h, struct jdb_changelog_rec* rec){
	size_t red;
	uint32_t recsize;	
	
	//if((ret = _jdb_jrnl_get_rec_hdr(h, &rec->hdr)) < 0) return ret;
	
	if((red = hard_read(h->jfd, (char*)&recsize, sizeof(uint32_t))) != 
		sizeof(uint32_t)) return -JE_READ;
		
	if(recsize < sizeof(struct jdb_changelog_hdr)) return -JE_SIZE;	
	
	if(recsize == sizeof(struct jdb_changelog_hdr)){
		rec->buf = NULL;
		rec->arg = NULL;
		rec->argsize = NULL;
		return 0;
	}
		
	rec->buf = (uchar*)malloc(recsize);	
	if(!rec->buf) return -JE_MALOC;
	
	memcpy(rec->buf, &recsize, sizeof(uint32_t));
	
	if((red = hard_read(h->jfd, (char*)(rec->buf + sizeof(uint32_t)),
			recsize - sizeof(uint32_t))) != 
			(recsize - sizeof(uint32_t)))
			return -JE_READ;
	
	rc4(rec->buf + sizeof(uint32_t), recsize, &h->rc4);
	
	_jdb_jrnl_buf_to_hdr(rec->buf, &rec->hdr);
	
	//rec->nargs = rec->buf[sizeof(struct jdb_changelog_hdr)];
	rec->argsize = (uint32_t*)(rec->buf + sizeof(struct jdb_changelog_hdr) + sizeof(uchar));
	//rec->arg = (uchar*)(rec->argsize + rec->nargs*sizeof(uint32_t));
	rec->arg = (uchar*)(rec->argsize + rec->hdr.nargs);
	
	return 0;
	
}

static inline uchar* _jdb_get_changelog_arg_raw(uint32_t* argsize, uchar* arg, uint32_t n){
	uint32_t pos, i;	
	pos = 0;
	for(i = 0; i < n; i++){
		pos += argsize[i];
	}
	_wdeb_pos(L"pos is %u", pos);
	return arg + pos;
}

static inline uchar* _jdb_get_changelog_arg(struct jdb_changelog_rec* rec, uint32_t n){

	return _jdb_get_changelog_arg_raw(rec->argsize, rec->arg, n);

}

static inline void _jdb_free_changelog_rec(struct jdb_changelog_rec* rec){
	if(rec->buf) free(rec->buf);
}

static inline int _jdb_jrnl_chk_create_table(struct jdb_changelog_rec* rec, struct jdb_jrnl_create_table* s){

	/*
		args {
			wchar_t* name;
			uint32_t nrows;
			uint32_t ncols;
			uchar flags;
			uint16_t indexes;
		}

		nargs = 5
	*/
	
	if(rec->hdr.nargs != 5) return -JE_INV;
	
	if(
	   rec->argsize[1] != sizeof(uint32_t) ||
	   rec->argsize[2] != sizeof(uint32_t) ||
	   rec->argsize[3] != sizeof(uchar) ||
	   rec->argsize[4] != sizeof(uint16_t)
	   )
	   	return -JE_INV;
	
	
	s->name = (wchar_t*)_jdb_get_changelog_arg(rec, 0);
	s->nrows = (uint32_t*)_jdb_get_changelog_arg(rec, 1);
	s->ncols = (uint32_t*)_jdb_get_changelog_arg(rec, 2);
	s->flags = (uint32_t*)_jdb_get_changelog_arg(rec, 3);
	s->indexes = (uint32_t*)_jdb_get_changelog_arg(rec, 4);
	
	return 0;
}

static inline int _jdb_jrnl_chk_rm_table(struct jdb_changelog_rec* rec, struct jdb_jrnl_rm_table* s){
	
	/*
		args {
			wchar_t* name;
		}
		
		nargs = 1
	*/
	
	if(rec->hdr.nargs != 1) return -JE_INV;
		
	s->name = (wchar_t*)_jdb_get_changelog_arg(rec, 0);
	
	return 0;
}

static inline int _jdb_jrnl_chk_add_typedef(struct jdb_changelog_rec* rec, struct jdb_jrnl_add_typedef* s){
	
	/*
		args {
			wchar_t* tname;	
			jdb_data_t type_id;
			jdb_data_t base;
			uint32_t len;
			uchar flags;
		}
		
		nargs = 5				
	*/
	
	if(rec->hdr.nargs != 5) return -JE_INV;
	
	if(
	   rec->argsize[1] != sizeof(jdb_data_t) ||
	   rec->argsize[2] != sizeof(jdb_data_t) ||
	   rec->argsize[3] != sizeof(uint32_t) ||
	   rec->argsize[4] != sizeof(uchar)
	   )
	   	return -JE_INV;	
	
	s->tname = (wchar_t*)_jdb_get_changelog_arg(rec, 0);
	s->type_id = (jdb_data_t*)_jdb_get_changelog_arg(rec, 1);
	s->base = (jdb_data_t*)_jdb_get_changelog_arg(rec, 2);
	s->len = (uint32_t*)_jdb_get_changelog_arg(rec, 3);
	s->flags = (uchar*)_jdb_get_changelog_arg(rec, 4);
	
	return 0;
}

static inline int _jdb_jrnl_chk_rm_typedef(struct jdb_changelog_rec* rec, struct jdb_jrnl_rm_typedef* s){
	
	/*
		args {
			wchar_t* tname;	
			jdb_data_t type_id;
		}
		
		nargs = 2
	*/
	
	if(rec->hdr.nargs != 2) return -JE_INV;

	if(
	   rec->argsize[1] != sizeof(jdb_data_t)
	   )
		return -JE_INV;
	
	s->tname = (wchar_t*)_jdb_get_changelog_arg(rec, 0);
	s->type_id = (jdb_data_t*)_jdb_get_changelog_arg(rec, 1);
	
	return 0;
}

static inline int _jdb_jrnl_chk_assign_coltype(struct jdb_changelog_rec* rec, struct jdb_jrnl_assign_coltype* s){
	
	/*
		args {
			wchar_t* tname;	
			jdb_data_t type_id;
			uint32_t col;
		}
		
		nargs = 3
	*/
	
	if(rec->hdr.nargs != 3) return -JE_INV;
	
	if(
	   rec->argsize[1] != sizeof(jdb_data_t) ||
	   rec->argsize[2] != sizeof(uint32_t)
	   )
	   	return -JE_INV;		
	
	s->tname = (wchar_t*)_jdb_get_changelog_arg(rec, 0);
	s->type_id = (jdb_data_t*)_jdb_get_changelog_arg(rec, 1);
	s->col = (uint32_t*)_jdb_get_changelog_arg(rec, 2);
	
	return 0;
}

static inline int _jdb_jrnl_chk_rm_coltype(struct jdb_changelog_rec* rec, struct jdb_jrnl_rm_coltype* s){
	
	/*
		args {
			wchar_t* tname;				
			uint32_t col;
		}
		
		nargs = 2
	*/
	
	if(rec->hdr.nargs != 2) return -JE_INV;
	
	if(
	   rec->argsize[1] != sizeof(uint32_t)
	   )
	   	return -JE_INV;		
	
	s->tname = (wchar_t*)_jdb_get_changelog_arg(rec, 0);	
	s->col = (uint32_t*)_jdb_get_changelog_arg(rec, 1);
	
	return 0;
}


static inline int _jdb_jrnl_chk_create_cell(struct jdb_changelog_rec* rec, struct jdb_jrnl_create_cell* s){
	
	/*
		args {
			wchar_t* tname;
			uint32_t* col;
			uint32_t* row;
			uchar* data;
			uint32_t* datalen;
			jdb_data_t* data_type;
		}

		nargs = 6
	*/
	
	if(rec->hdr.nargs != 6) return -JE_INV;

	if(
	   rec->argsize[1] != sizeof(uint32_t) ||
	   rec->argsize[2] != sizeof(uint32_t) ||
	   rec->argsize[4] != sizeof(uint32_t) ||
	   rec->argsize[5] != sizeof(uchar)
	   )
	   	return -JE_INV;	

	s->tname = (wchar_t*)_jdb_get_changelog_arg(rec, 0);
	s->col = (uint32_t*)_jdb_get_changelog_arg(rec, 1);
	s->row = (uint32_t*)_jdb_get_changelog_arg(rec, 2);
	s->data = (uchar*)_jdb_get_changelog_arg(rec, 3);
	s->datalen = (uint32_t*)_jdb_get_changelog_arg(rec, 4);
	s->data_type = (uchar*)_jdb_get_changelog_arg(rec, 5);

	return 0;
}

static inline int _jdb_jrnl_chk_rm_cell(struct jdb_changelog_rec* rec, struct jdb_jrnl_rm_cell* s){
	
	/*
		args {
			wchar_t* tname;
			uint32_t* col;
			uint32_t* row;
		}

		nargs = 3
	*/
	
	if(rec->hdr.nargs != 3) return -JE_INV;

	if(
	   rec->argsize[1] != sizeof(uint32_t) ||
	   rec->argsize[2] != sizeof(uint32_t)
	   )
	   	return -JE_INV;	

	s->tname = (wchar_t*)_jdb_get_changelog_arg(rec, 0);
	s->col = (uint32_t*)_jdb_get_changelog_arg(rec, 1);
	s->row = (uint32_t*)_jdb_get_changelog_arg(rec, 2);

	return 0;
}

static inline int _jdb_jrnl_chk_update_cell(struct jdb_changelog_rec* rec, struct jdb_jrnl_update_cell* s){
	
	/*
		args {
			wchar_t* tname;
			uint32_t* col;
			uint32_t* row;
			uchar* data;
			uint32_t* datalen;
		}

		nargs = 5
	*/
	
	if(rec->hdr.nargs != 5) return -JE_INV;

	if(
	   rec->argsize[1] != sizeof(uint32_t) ||
	   rec->argsize[2] != sizeof(uint32_t) ||
	   rec->argsize[4] != sizeof(uint32_t)
	   )
	   	return -JE_INV;	

	
	s->tname = (wchar_t*)_jdb_get_changelog_arg(rec, 0);	
	s->col = (uint32_t*)_jdb_get_changelog_arg(rec, 1);
	s->row = (uint32_t*)_jdb_get_changelog_arg(rec, 2);
	s->data = (uchar*)_jdb_get_changelog_arg(rec, 3);
	s->datalen = (uint32_t*)_jdb_get_changelog_arg(rec, 4);	
	
	return 0;
}

static inline void _jdb_free_changelog_list(struct jdb_changelog_entry** first){
	struct jdb_changelog_entry *del;
	del = *first;
	while(*first){
		*first = (*first)->next;
		_jdb_free_changelog_rec(&del->rec);
		free(del);
		del = *first;
	}
	
}

static inline int _jdb_jrnl_load_all_recs(struct jdb_handle* h, struct jdb_changelog_entry** first){
	
	struct jdb_changelog_entry *entry, *last;

	lseek(h->jfd, 0, SEEK_SET);
	*first = NULL;
	entry = (struct jdb_changelog_entry*)malloc(sizeof(struct jdb_changelog_entry));
	if(!entry){
		return -JE_MALOC;
	}
	entry->next = NULL;
	
	while(_jdb_jrnl_get_rec(h, &entry->rec) != -JE_READ){
		
		//save the record
		if(!(*first)){
			*first = entry;
			last = *first;
		} else {
			last->next = entry;
			last = last->next;
		}
		
		//alloc for new record
		entry = (struct jdb_changelog_entry*)malloc(sizeof(struct jdb_changelog_entry));
		if(!entry){
			if(*first)
				_jdb_free_changelog_list(first);
				return -JE_MALOC;
		}
		entry->next = NULL;
		
	}
	
	//release the last allocated pointer
	free(entry);
	
	return 0;
	
}


int _jdb_jrnl_check_chid_bounds(struct jdb_handle* h){
	struct jdb_changelog_entry* first, *begin, *end;
	int ret;
	
	if((ret = _jdb_jrnl_load_all_recs(h, &first)) < 0) return ret;
	
	for(begin = first; begin; begin = begin->next){
		for(end = first; end; end = end->next){
			if(
			   begin->rec.hdr.cmd != JDB_CMD_EMPTY &&
			   end->rec.hdr.cmd != JDB_CMD_EMPTY
			  ){
				
				if(end->rec.hdr.chid == begin->rec.hdr.chid){
					begin->rec.hdr.cmd = JDB_CMD_EMPTY;
					end->rec.hdr.cmd = JDB_CMD_EMPTY;
				}
				
			}
		}
	}
	
	for(begin = first; begin; begin = begin->next){
		if(begin->rec.hdr.cmd != JDB_CMD_EMPTY) return -JE_EXISTS;
	}
	
	return 0;
	
}

static inline int _jdb_jrnl_run_rec_cmd(struct jdb_handle* h, struct jdb_changelog_entry* first){
	struct jdb_changelog_entry* entry;
	
	for(entry = first; entry; entry = entry->next){
		switch(entry->rec.hdr.cmd){
			
			case JDB_CMD_CREATE_TABLE:
				break;
			case JDB_CMD_RM_TABLE:
				break;			
			case JDB_CMD_ADD_TYPEDEF:
				break;
			case JDB_CMD_RM_TYPEDEF:
				break;
			case JDB_CMD_ASSIGN_COLTYPE:
				break;
			case JDB_CMD_RM_COLTYPE:
				break;				
			case JDB_CMD_CREATE_CELL:
				break;
			case JDB_CMD_RM_CELL:
				break;
			case JDB_CMD_UPDATE_CELL:
				break;			
			
			default:
				break;
		}
	}
	
	return -JE_IMPLEMENT;
}

static inline int _jdb_jrnl_rm_expired_chid(struct jdb_handle* h, struct jdb_changelog_entry* first){
	
	struct jdb_changelog_entry* entry;
	
	for(entry = first; entry; entry = entry->next){
		if(entry->rec.hdr.chid <= h->hdr.chid){
			entry->rec.hdr.cmd = JDB_CMD_EMPTY;
		}
	}

	for(entry = first; entry; entry = entry->next){
		if(entry->rec.hdr.cmd != JDB_CMD_EMPTY) return -JE_EXISTS;
	}
	
	return 0;
	
}

static inline int _jdb_jrnl_trunc(struct jdb_handle* h){
	int ret;
	close(h->jfd);
	
	ret = _jdb_jrnl_open(h, h->conf.filename, JDB_JRNL_TRUNC);
	if(ret == -JE_EXISTS) return 0;
	return ret;
}

int _jdb_jrnl_recover(struct jdb_handle* h){
	int ret;
	struct jdb_changelog_entry* first;
	
	if((ret = _jdb_jrnl_load_all_recs(h, &first)) < 0) return ret;
	
	if(!_jdb_jrnl_rm_expired_chid(h, first)) return 0;
	
	if((ret = _jdb_jrnl_run_rec_cmd(h, first)) < 0) return ret;
	
	//if recovered successfully, truncate old file!
	
	return _jdb_jrnl_trunc(h);
}

//int _jdb_changelog_backup_

int _jdb_changelog_reg(struct jdb_handle* h, uint64_t chid , uchar cmd, int ret, uchar nargs, size_t* argsize, ...){
	va_list ap;
	uchar n;
	size_t bufsize;
	size_t pos;	
	uchar* buf;
	struct jdb_changelog_hdr hdr;

	if(!(h->flags & JDB_O_JRNL)) return -JE_NOJOURNAL;	

	bufsize = sizeof(struct jdb_changelog_hdr);
	//if(nargs) bufsize += sizeof(uchar); //for nargs
	pos = bufsize;
	for(n = 0; n < nargs; n++){
		bufsize += argsize[n];
	}
	
	buf = (uchar*)malloc(bufsize);
	if(!buf){
		return -JE_MALOC;
	}	
	
	va_start(ap, argsize);
	
	for(n = 0; n < nargs; n++){
			
		//copy data
		memcpy(buf + pos, va_arg(ap, uchar*), argsize[n]);
		pos += argsize[n];
		_wdeb_proc(L"pos = %u (after adding argsize[%u])", pos, n);
		
	}
	
	va_end(ap);
	
	hdr.recsize = bufsize;	
	hdr.chid = chid;
	hdr.cmd = cmd;
	hdr.ret = ret;
	hdr.nargs = nargs;

	_wdeb_reg(L"recsize = %u, ret = %i, cmd = %u, chid = %llu, nargs = %u",
		hdr.recsize, hdr.ret, hdr.cmd, hdr.chid, hdr.nargs);
	
	//memcpy((void*)buf, (void*)&hdr, sizeof(struct jdb_changelog_hdr));
	
	_jdb_jrnl_hdr_to_buf(&hdr, buf);
		
	//if(nargs) buf[sizeof(struct jdb_changelog_hdr)] = nargs;
	
	_jdb_jrnl_request_write(h, hdr.recsize, buf);
	
	return 0;
}

int _jdb_changelog_reg_end(struct jdb_handle* h, uint64_t chid, int ret){
	return _jdb_changelog_reg(h, chid, JDB_CMD_END, ret, 0, 0, 0, NULL);
}
