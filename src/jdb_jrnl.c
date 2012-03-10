#include "jdb.h"

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
						
		hard_write(h->jfd, (void*)entry->buf, entry->bufsize);
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
		ret = JE_EXISTS;
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
	
	if(ret == JE_EXISTS){//read str_key
		if(hard_read(h->jfd, h->conf.str_key, JDB_STR_KEY_SIZE) < 
			JDB_STR_KEY_SIZE){
				
			return -JE_READ;
		}
	} else if(!ret){//write str_key
		if(hard_write(h->jfd, h->conf.str_key, JDB_STR_KEY_SIZE) <
			JDB_STR_KEY_SIZE){
				
			return -JE_WRITE;
		}
	}
	
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
	_wdeb_io(L"closing %ls", jfilename);
	unlink(jfilename);
	free(jfilename);
	close(h->jfd);
	
	return 0;
}

static inline int _jdb_jrnl_load_all_recs(struct jdb_handle* h, struct jdb_jrnl_entry** first){
	
	struct jdb_jrnl_entry *entry, *last;
	
	_wdeb_io(L"loading all records...");

	lseek(h->jfd, JDB_STR_KEY_SIZE, SEEK_SET);
	*first = NULL;
	entry = (struct jdb_jrnl_entry*)malloc(sizeof(struct jdb_jrnl_entry));
	if(!entry){
		return -JE_MALOC;
	}
	entry->next = NULL;
	
	while(_jdb_jrnl_get_rec(h, h->jfd, &entry->rec, 1) != -JE_READ){
		
		//save the record
		if(!(*first)){
			*first = entry;
			last = *first;
		} else {
			last->next = entry;
			last = last->next;
		}
		
		//alloc for new record
		entry = (struct jdb_jrnl_entry*)malloc(sizeof(struct jdb_jrnl_entry));
		if(!entry){
			if(*first)
				_jdb_free_jrnl_list(first);
				return -JE_MALOC;
		}
		entry->next = NULL;
		
	}
	
	//release the last allocated pointer
	free(entry);
	
	return 0;
	
}

int _jdb_jrnl_check_chid_bounds(struct jdb_handle* h){
	struct jdb_jrnl_entry* first, *begin, *end;
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

static inline int _jdb_jrnl_rm_expired_chid(struct jdb_handle* h, struct jdb_jrnl_entry* first){
	
	struct jdb_jrnl_entry* entry;
	
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
	
	_wdeb_io(L"truncating journal...");
	
	close(h->jfd);
	
	ret = _jdb_jrnl_open(h, h->conf.filename, JDB_JRNL_TRUNC);
	if(ret == -JE_EXISTS) return 0;
	return ret;
}

int _jdb_jrnl_recover(struct jdb_handle* h){
	int ret;
	struct jdb_jrnl_entry* first;
	
	_wdeb_io(L"recovering journal...");
	
	if((ret = _jdb_jrnl_load_all_recs(h, &first)) < 0) return ret;
	
	if(!_jdb_jrnl_rm_expired_chid(h, first)) return 0;
	
	if((ret = _jdb_jrnl_run_rec_cmd_list(h, first)) < 0) return ret;
	
	//if recovered successfully, truncate old file!
	
	return _jdb_jrnl_trunc(h);
}

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

#define JDB_JRNL_ALLOC_CHUNK	100

#include "jdb.h"

uint32_t _jdb_jrnl_argbufsize(uchar nargs, uint32_t* argsize){
	uint32_t argbufsize;
	uchar n;
	
	argbufsize = nargs*sizeof(uint32_t);
	for(n = 0; n < nargs; n++)
		argbufsize += argsize[n];
	
	return argbufsize;
}

void* _jdb_jrnl_get_arg_buf(uint32_t* argsize, void* argbuf, uint32_t n){
	uint32_t pos, i;	
	pos = 0;
	for(i = 0; i < n; i++){
		pos += argsize[i];
	}
	
	return argbuf + pos;
}

void* _jdb_jrnl_get_arg(struct jdb_jrnl_rec* rec, uint32_t n){

	return _jdb_jrnl_get_arg_buf(rec->argsize, rec->arg, n);

}

int _jdb_jrnl_get_rec(struct jdb_handle* h, int fd, struct jdb_jrnl_rec* rec, int decrypt){
	size_t red;
	uint32_t argbufsize;
	
	_wdeb_io(L"reading record...");
	
	if((red = hard_read(	fd, (char*)&rec->hdr,
				sizeof(struct jdb_changlog_hdr))) != 
				sizeof(struct jdb_changlog_hdr))
					return -JE_READ;

	if(decrypt){	
		rc4(	&rec->hdr,
			sizeof(struct jdb_jrnl_hdr),
			&h->rc4);
	}	
						
	if(!rec->hdr.argbufsize){
		rec->buf = NULL;
		rec->arg = NULL;
		rec->argsize = NULL;
		return 0;
	}
		
	rec->argbuf = malloc(rec->hdr.argbufsize);	
	if(!rec->argbuf) return -JE_MALOC;
	
	if(hard_read(
				fd,
				(char*)(rec->argbuf),
				rec->hdr.argbufsize								
				) != rec->hdr.argbufsize)
				
		) return -JE_READ;

	if(decrypt){	
		rc4(	&rec->argbuf,
			rec->hdr.argbufsize,
			&h->rc4);
	}
		
	rec->argsize = (uint32_t*)(rec->argbuf);
	rec->arg = (void*)(rec->argsize + rec->hdr.nargs*sizeof(uint32_t));
	
	return 0;
	
}

void _jdb_free_jrnl_rec(struct jdb_jrnl_rec* rec){
	if(rec->argbuf) free(rec->argbuf);
}

static inline int _jdb_jrnl_parse_create_table(struct jdb_jrnl_rec* rec, struct jdb_jrnl_create_table* s){

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
	
	
	s->name = (wchar_t*)_jdb_get_jrnl_arg(rec, 0);
	s->nrows = (uint32_t*)_jdb_get_jrnl_arg(rec, 1);
	s->ncols = (uint32_t*)_jdb_get_jrnl_arg(rec, 2);
	s->flags = (uint32_t*)_jdb_get_jrnl_arg(rec, 3);
	s->indexes = (uint32_t*)_jdb_get_jrnl_arg(rec, 4);
	
	return 0;
}

static inline int _jdb_jrnl_parse_rm_table(struct jdb_jrnl_rec* rec, struct jdb_jrnl_rm_table* s){
	
	/*
		args {
			wchar_t* name;
		}
		
		nargs = 1
	*/
	
	if(rec->hdr.nargs != 1) return -JE_INV;
		
	s->name = (wchar_t*)_jdb_get_jrnl_arg(rec, 0);
	
	return 0;
}

static inline int _jdb_jrnl_parse_add_typedef(struct jdb_jrnl_rec* rec, struct jdb_jrnl_add_typedef* s){
	
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
	
	s->tname = (wchar_t*)_jdb_get_jrnl_arg(rec, 0);
	s->type_id = (jdb_data_t*)_jdb_get_jrnl_arg(rec, 1);
	s->base = (jdb_data_t*)_jdb_get_jrnl_arg(rec, 2);
	s->len = (uint32_t*)_jdb_get_jrnl_arg(rec, 3);
	s->flags = (uchar*)_jdb_get_jrnl_arg(rec, 4);
	
	return 0;
}

static inline int _jdb_jrnl_parse_rm_typedef(struct jdb_jrnl_rec* rec, struct jdb_jrnl_rm_typedef* s){
	
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
	
	s->tname = (wchar_t*)_jdb_get_jrnl_arg(rec, 0);
	s->type_id = (jdb_data_t*)_jdb_get_jrnl_arg(rec, 1);
	
	return 0;
}

static inline int _jdb_jrnl_parse_assign_coltype(struct jdb_jrnl_rec* rec, struct jdb_jrnl_assign_coltype* s){
	
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
	
	s->tname = (wchar_t*)_jdb_get_jrnl_arg(rec, 0);
	s->type_id = (jdb_data_t*)_jdb_get_jrnl_arg(rec, 1);
	s->col = (uint32_t*)_jdb_get_jrnl_arg(rec, 2);
	
	return 0;
}

static inline int _jdb_jrnl_parse_rm_coltype(struct jdb_jrnl_rec* rec, struct jdb_jrnl_rm_coltype* s){
	
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
	
	s->tname = (wchar_t*)_jdb_get_jrnl_arg(rec, 0);	
	s->col = (uint32_t*)_jdb_get_jrnl_arg(rec, 1);
	
	return 0;
}


static inline int _jdb_jrnl_parse_create_cell(struct jdb_jrnl_rec* rec, struct jdb_jrnl_create_cell* s){
	
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

	s->tname = (wchar_t*)_jdb_get_jrnl_arg(rec, 0);
	s->col = (uint32_t*)_jdb_get_jrnl_arg(rec, 1);
	s->row = (uint32_t*)_jdb_get_jrnl_arg(rec, 2);
	s->data = (uchar*)_jdb_get_jrnl_arg(rec, 3);
	s->datalen = (uint32_t*)_jdb_get_jrnl_arg(rec, 4);
	s->data_type = (uchar*)_jdb_get_jrnl_arg(rec, 5);

	return 0;
}

static inline int _jdb_jrnl_parse_rm_cell(struct jdb_jrnl_rec* rec, struct jdb_jrnl_rm_cell* s){
	
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

	s->tname = (wchar_t*)_jdb_get_jrnl_arg(rec, 0);
	s->col = (uint32_t*)_jdb_get_jrnl_arg(rec, 1);
	s->row = (uint32_t*)_jdb_get_jrnl_arg(rec, 2);

	return 0;
}

static inline int _jdb_jrnl_parse_update_cell(struct jdb_jrnl_rec* rec, struct jdb_jrnl_update_cell* s){
	
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

	
	s->tname = (wchar_t*)_jdb_get_jrnl_arg(rec, 0);	
	s->col = (uint32_t*)_jdb_get_jrnl_arg(rec, 1);
	s->row = (uint32_t*)_jdb_get_jrnl_arg(rec, 2);
	s->data = (uchar*)_jdb_get_jrnl_arg(rec, 3);
	s->datalen = (uint32_t*)_jdb_get_jrnl_arg(rec, 4);	
	
	return 0;
}

static inline void _jdb_free_jrnl_list(struct jdb_jrnl_entry** first){
	struct jdb_jrnl_entry *del;
	del = *first;
	while(*first){
		*first = (*first)->next;
		_jdb_free_jrnl_rec(&del->rec);
		free(del);
		del = *first;
	}
	
}

static inline int _jdb_jrnl_run_create_table(struct jdb_handle* h,
			struct jdb_jrnl_create_table* create_table){
	return jdb_create_table(h,
				create_table->name,
				*(create_table->nrows),
				*(create_table->ncols),
				*(create_table->flags),
				*(create_table->indexes)
				);
}

static inline int _jdb_jrnl_run_rm_table(struct jdb_handle* h,
			struct jdb_jrnl_rm_table* rm_table){
	return jdb_rm_table(h,
				rm_table->name
				);
}

static inline int _jdb_jrnl_run_add_typedef(struct jdb_handle* h,
			struct jdb_jrnl_add_typedef* add_typedef){
	return jdb_add_typedef(h,
				add_typedef->tname,
				*(add_typedef->type_id),
				*(add_typedef->base),
				*(add_typedef->len),
				*(add_typedef->flags)
				);
}

static inline int _jdb_jrnl_run_rm_typedef(struct jdb_handle* h,
			struct jdb_jrnl_rm_typedef* rm_typedef){
	return jdb_rm_typedef(h,
				rm_typedef->tname,
				*(rm_typedef->type_id)
				);
}

static inline int _jdb_jrnl_run_assign_coltype(struct jdb_handle* h,
			struct jdb_jrnl_assign_coltype* assign_coltype){
	return jdb_assign_col_type(h,
				assign_coltype->tname,
				*(assign_coltype->type_id),
				*(assign_coltype->col)
				);
}

static inline int _jdb_jrnl_run_rm_coltype(struct jdb_handle* h,
			struct jdb_jrnl_rm_coltype* rm_coltype){
	return jdb_rm_col_type(h,
				rm_coltype->tname,
				*(rm_coltype->col)
				);
}

static inline int _jdb_jrnl_run_create_cell(struct jdb_handle* h,
			struct jdb_jrnl_create_cell* create_cell){
	return jdb_create_cell(h,
				create_cell->tname,
				*(create_cell->col),
				*(create_cell->row),
				create_cell->data,
				*(create_cell->datalen),
				*(create_cell->data_type)
				);
}

static inline int _jdb_jrnl_run_rm_cell(struct jdb_handle* h,
			struct jdb_jrnl_rm_cell* rm_cell){
	return jdb_rm_cell(h,
				rm_cell->tname,
				*(rm_cell->col),
				*(rm_cell->row)
				);
}

static inline int _jdb_jrnl_run_update_cell(struct jdb_handle* h,
			struct jdb_jrnl_update_cell* update_cell){
	return jdb_update_cell(h,
				update_cell->tname,
				*(update_cell->col),
				*(update_cell->row),
				update_cell->data,
				*(update_cell->datalen)				
				);
}

static inline int _jdb_jrnl_run_rec_cmd(struct jdb_handle* h, struct jdb_jrnl_entry* entry){
		
	struct jdb_jrnl_create_table	create_table;
	struct jdb_jrnl_rm_table		rm_table;
	struct jdb_jrnl_add_typedef	add_typedef;
	struct jdb_jrnl_rm_typedef		rm_typedef;
	struct jdb_jrnl_assign_coltype	assign_coltype;
	struct jdb_jrnl_rm_coltype		rm_coltype;
	struct jdb_jrnl_create_cell	create_cell;
	struct jdb_jrnl_rm_cell		rm_cell;
	struct jdb_jrnl_update_cell	update_cell;
	
	switch(entry->rec.hdr.cmd & JDB_CMD_MASK){
		
		case JDB_CMD_CREATE_TABLE:
			if((ret = _jdb_jrnl_parse_create_table(
				entry, &create_table)) < 0) return ret;
			return _jdb_jrnl_run_create_table(h,
				&create_table);
			break;
		case JDB_CMD_RM_TABLE:
			if((ret = _jdb_jrnl_parse_rm_table(
				entry, &rm_table)) < 0) return ret;
			return _jdb_jrnl_run_rm_table(h,
				&rm_table);
			break;			
		case JDB_CMD_ADD_TYPEDEF:
			if((ret = _jdb_jrnl_parse_add_typedef(
				entry, &add_typedef)) < 0) return ret;
			return _jdb_jrnl_run_add_typedef(h,
				&add_typedef);
			break;
		case JDB_CMD_RM_TYPEDEF:
			if((ret = _jdb_jrnl_parse_rm_typedef(
				entry, &rm_typedef)) < 0) return ret;
			return _jdb_jrnl_run_rm_typedef(h,
				&rm_typedef);
			break;
		case JDB_CMD_ASSIGN_COLTYPE:
			if((ret = _jdb_jrnl_parse_assign_coltype(
				entry, &assign_coltype)) < 0) return ret;
			return _jdb_jrnl_run_assign_coltype(h,
				&assign_coltype);
			break;
		case JDB_CMD_RM_COLTYPE:
			if((ret = _jdb_jrnl_parse_rm_coltype(
				entry, &rm_coltype)) < 0) return ret;
			return _jdb_jrnl_run_rm_coltype(h,
				&rm_coltype);
			break;				
		case JDB_CMD_CREATE_CELL:
			if((ret = _jdb_jrnl_parse_create_cell(
				entry, &create_cell)) < 0) return ret;
			return _jdb_jrnl_run_create_cell(h,
				&create_cell);
			break;
		case JDB_CMD_RM_CELL:
			if((ret = _jdb_jrnl_parse_rm_cell(
				entry, &rm_cell)) < 0) return ret;
			return _jdb_jrnl_run_rm_cell(h,
				&rm_cell);
			break;
		case JDB_CMD_UPDATE_CELL:
			if((ret = _jdb_jrnl_parse_update_cell(
				entry, &update_cell)) < 0) return ret;
			return _jdb_jrnl_run_update_cell(h,
				&update_cell);
			break;
		
		default:
			return -JE_TYPE;
			break;
	}
		
	return 0;
}

static inline int _jdb_jrnl_run_rec_cmd_list(struct jdb_handle* h, struct jdb_jrnl_entry* first){
	struct jdb_jrnl_entry* entry;
	int ret;
	
	for(entry = first; entry; entry = entry->next)
		if((ret = _jdb_jrnl_run_rec_cmd(h, entry)) < 0) return ret;	
	
	return 0;
}

int _jdb_jrnl_assm_argbuf(uchar** argbuf, size_t* argbufsize, uchar nargs, size_t* argsize, ...){

	va_list ap;
	uchar n;
	size_t pos;
	
	*argbufsize = 0;
	pos = 0UL;
	for(n = 0; n < nargs; n++){
		*argbufsize += argsize[n];
	}
	
	*argbuf = (uchar*)malloc(*argbufsize);
	if(!(*argbuf)){
		return -JE_MALOC;
	}	
	
	va_start(ap, argsize);
	
	for(n = 0; n < nargs; n++){

		//copy data
		memcpy((*argbuf) + pos, va_arg(ap, uchar*), argsize[n]);
		pos += argsize[n];
		_wdeb_proc(L"pos = %u (after adding argsize[%u])", pos, n);

	}
	
	va_end(ap);

	return 0;
}

int _jdb_jrnl_assm_rec(	struct jdb_jrnl_rec* rec, uint64_t chid,
				uchar cmd, int ret, uchar nargs,
				size_t* argsize, ...){

	va_list ap;
	uchar n;
	size_t pos;
	
	rec->hdr.argbufsize = nargs*sizeof(uint32_t);
	pos = rec->hdr.argbufsize;
	for(n = 0; n < nargs; n++){
		rec->hdr.argbufsize += argsize[n];
	}
	
	rec->argbuf = malloc(rec->hdr.argbufsize);
	if(!rec->argbuf){
		return -JE_MALOC;
	}
	
	//copy argsize
	memcpy(rec->argbuf, (void*)argsize, pos);
	
	rec->argsize = (uint32_t*)rec->argbuf;
	rec->arg = (void*)(rec->argsize + pos);
	
	va_start(ap, argsize);	
	
	for(n = 0; n < nargs; n++){

		//copy data
		memcpy(rec->argbuf + pos, va_arg(ap, uchar*), argsize[n]);
		pos += argsize[n];
		_wdeb_proc(L"pos = %u (after adding argsize[%u])", pos, n);

	}
	
	va_end(ap);		

	return 0;
}

int _jdb_jrnl_reg(struct jdb_handle* h, struct jdb_jrnl_rec* rec){	
	uchar* buf;	
	
	_wdeb_reg(L"registerning command 0x%x", rec->hdr.cmd);
	
	if(h->hdr.flags & JDB_O_JRNL){//JOURNALLING
	
		buf = (uchar*)malloc(	rec->hdr.argbufsize + 
					sizeof(struct jdb_jrnl_hdr));
		if(!buf) return -JE_MALOC;			
		
		//copy/encrypt header
				
		memcpy(buf, (void*)&rec->hdr, sizeof(struct jdb_jrnl_hdr));
		rc4(buf, sizeof(struct jdb_changlog_hdr), &h->rc4);
				
		memcpy(	buf + sizeof(struct jdb_jrnl_hdr),
			rec->argbuf, rec->hdr.argbufsize);
			
		rc4(	buf + sizeof(struct jdb_changlog_hdr),
			rec->hdr.argbufsize,
			&h->rc4);			
			
		_jdb_jrnl_request_write(h,
			rec->hdr.argbufsize + sizeof(struct jdb_jrnl_hdr),
			buf);
	}//end of if JOURNALLING
	
	return 0;
}

int _jdb_jrnl_reg_end(struct jdb_handle* h, struct jdb_jrnl_rec* rec, int ret){
	rec->hdr.ret = ret;
	rec->hdr.cmd |= JDB_CMD_END;
	return _jdb_jrnl_reg(h, rec);
}
