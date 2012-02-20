#include "jdb.h"
#include "jconst.h"

#include <unistd.h>

#define _wdeb_startup _wdeb

/*
#ifdef _WIN32
#define sleep(sec) Sleep(1000*sec)
#endif
*/

void _jdb_fill_date(uchar d[3], uchar t[3])
{
	time_t ti;
	struct tm *tp;
	ti = time(NULL);
	tp = localtime(&ti);
#ifndef WIN32
	d[0] = (uchar) tp->tm_year;
#else
	d[0] = (uchar) tp->tm_year + 70;
#endif
	d[1] = (uchar) tp->tm_mon;
	d[2] = (uchar) tp->tm_mday;
	t[0] = (uchar) tp->tm_hour;
	t[1] = (uchar) tp->tm_min;
	t[2] = (uchar) tp->tm_sec;
}

/*

*/

void _jdb_lock_handle(struct jdb_handle *h)
{

}

void _jdb_unlock_handle(struct jdb_handle *h)
{

}

void _jdb_unset_handle_flag(struct jdb_handle *h, uint16_t flag, int lock)
{
	if (lock)
		_jdb_lock_handle(h);
	h->flags &= ~flag;
	if (lock)
		_jdb_unlock_handle(h);
}

void _jdb_set_handle_flag(struct jdb_handle *h, uint16_t flag, int lock)
{
	if (lock)
		_jdb_lock_handle(h);
	h->flags |= flag;
	if (lock)
		_jdb_unlock_handle(h);
}

uint16_t _jdb_get_handle_flag(struct jdb_handle *h, int lock)
{
	uint16_t flags;
	if (lock)
		_jdb_lock_handle(h);
	flags = h->flags;
	if (lock)
		_jdb_unlock_handle(h);
	return flags;
}

uint64_t _jdb_get_chid(struct jdb_handle *h, int lock)
{
	uint64_t chid;
	if (lock)
		_jdb_lock_handle(h);
	chid = h->hdr.chid++;	
	if (lock)
		_jdb_unlock_handle(h);
		
	return chid;
}

//header IO functions has been placed to jdb_block.c

void _jdb_calc_bentries(struct jdb_handle *h)
{
	h->hdr.map_bent =
	    (h->hdr.blocksize -
	     sizeof(struct jdb_map_blk_hdr)) / sizeof(struct jdb_map_blk_entry);
	h->hdr.typedef_bent =
	    (h->hdr.blocksize -
	     sizeof(struct jdb_typedef_blk_hdr)) /
	    sizeof(struct jdb_typedef_blk_entry);
	h->hdr.col_typedef_bent =
	    (h->hdr.blocksize -
	     sizeof(struct jdb_col_typedef_blk_hdr)) /
	    sizeof(struct jdb_col_typedef_blk_entry);	    
	h->hdr.celldef_bent =
	    (h->hdr.blocksize -
	     sizeof(struct jdb_celldef_blk_hdr)) /
	    sizeof(struct jdb_celldef_blk_entry);
	h->hdr.index1_bent =
	    (h->hdr.blocksize -
	     sizeof(struct jdb_index_blk_hdr)) /
	    sizeof(struct jdb_index1_blk_entry);
	h->hdr.index0_bent =
	    (h->hdr.blocksize -
	     sizeof(struct jdb_index_blk_hdr)) /
	    sizeof(struct jdb_index0_blk_entry);
	h->hdr.dptr_bent =
	    (h->hdr.blocksize -
	     sizeof(struct jdb_cell_data_ptr_blk_hdr)) /
	    sizeof(struct jdb_cell_data_ptr_blk_entry);
	h->hdr.fav_bent = (h->hdr.blocksize - sizeof(struct jdb_fav_blk_hdr))/
				sizeof(struct jdb_fav_blk_entry);
}

static inline int _jdb_assert_conf(struct jdb_conf *conf)
{
	if (conf->blocksize % JDB_MIN_BLK_SIZE){
		_wdeb(L"blocksize = %u", conf->blocksize);
		return -JE_INV;
	}
	
	if(!conf->filename || !conf->key) return -JE_NULLPTR;
		
	return 0;
}

static inline int _jdb_assert_db_struct(struct jdb_handle *h)
{
	struct stat st;
	if (h->hdr.blocksize % JDB_MIN_BLK_SIZE)
		return -JE_INV;
	fstat(h->fd, &st);
	if (h->hdr.nblocks != st.st_size / h->hdr.blocksize)
		return -JE_INV;
	//if(h->hdr.nmaps < 
	return 0;
}

int jdb_init(struct jdb_handle *h)
{
	h->map_list.first = NULL;
	h->table_list.first = NULL;
	h->conf.filename = NULL;
	h->conf.key = NULL;

	h->jfd = -1;

	h->fd = -1;

	h->flags = 0;
	
	h->jopid = 0UL;

	memset(&h->hdr, 0, sizeof(struct jdb_hdr));

	h->magic = JDB_MAGIC;
	return 0;
}

int _jdb_cleanup_handle(struct jdb_handle *h)
{
	if (h->magic != JDB_MAGIC)
		return -JE_NOINIT;
	if (h->fd != -1) {
		close(h->fd);
		h->fd = -1;
	}
	if (h->jfd != -1) {
		close(h->jfd);
		h->jfd = -1;
	}
	if (h->conf.filename) {
		free(h->conf.filename);
		h->conf.filename = NULL;
	}
	if (h->conf.key) {
		free(h->conf.key);
		h->conf.key = NULL;
	}
	_jdb_free_map_list(h);
	h->flags = 0;
	
	memset(&h->hdr, 0, sizeof(struct jdb_hdr));

	return 0;
}

static inline int _jdb_fill_def_conf(struct jdb_conf *conf)
{
	_wdeb_startup(L"filling default config struct");
	conf->flags = JDB_DEF_FLAGS;
	conf->blocksize = JDB_DEF_BLK_SIZE;
	conf->max_blocks = JDB_ID_INVAL;

/*
	conf->filename = (wchar_t*)malloc(WBYTES(JDB_DEF_FILENAME));
	wcscpy(conf->filename, JDB_DEF_FILENAME);
	conf->key = (wchar_t*)malloc(WBYTES(JDB_DEF_KEY));
	wcscpy(conf->key, JDB_DEF_KEY);
*/

	conf->list_buck = JDB_DEF_LIST_BUCK;
	conf->map_list_buck = JDB_DEF_MAPLIST_BUCK;
	conf->fav_load = JDB_DEF_FAV_LOAD;
	conf->crypt_type = JDB_DEF_CRYPT;
	conf->crc_type = JDB_DEF_CRC;
	return 0;
}

int _jdb_init_crypto(struct jdb_handle *h)
{

	uchar key[32];
	//uchar des3_key[24]; //3x8byte keys
	
	rc4_init((uchar *) h->conf.key, WBYTES(h->conf.key), &h->rc4);

	switch (h->conf.crypt_type) {
	case JDB_CRYPT_NONE:
		return 0;
	case JDB_CRYPT_AES:
		sha256((uchar *) h->conf.key, WBYTES(h->conf.key), key);
		if(aes_set_key(&h->aes, (uchar *) key, 256)) return -JE_CRYPT;
		return 0;
		break;
	case JDB_CRYPT_DES3:
		sha256((uchar *) h->conf.key, WBYTES(h->conf.key), key);
		if(des3_set_3keys(&h->des3, (uchar *)key,
			(uchar *)key + DES3_KSIZE,
			(uchar *)key + (2*DES3_KSIZE))) return -JE_CRYPT;
		return 0;
		break;		
	default:
		_wdeb_startup(L"unknown encryption 0x%x", h->conf.crypt_type);

		return -JE_IMPLEMENT;
	}

}

static inline void _jdb_copy_conf_to_hdr(struct jdb_handle *h)
{

	h->hdr.flags = h->conf.flags;
	h->hdr.blocksize = h->conf.blocksize;
	h->hdr.map_list_buck = h->conf.map_list_buck;
	h->hdr.list_buck = h->conf.list_buck;
	h->hdr.max_blocks = h->conf.max_blocks;
	h->hdr.crc_type = h->conf.crc_type;
	h->hdr.crypt_type = h->conf.crypt_type;
}

int jdb_fstat(wchar_t * wfilename, wchar_t * key, uint16_t mode)
{

	char filename[J_MAX_PATHNAME8];

	wcstombs(filename, wfilename, J_MAX_PATHNAME8);

	if (mode & JDB_FSTAT_EXIST) {
		if (access(filename, F_OK))
			return -JE_FAIL;
	}
	
	_wdeb(L"WARNING: NOT COMPLETED YET!");

	return 0;
}

int _jdb_create(struct jdb_handle *h)
{
	int ret;
	char filename[J_MAX_PATHNAME8];

	_wdeb_startup(L"called, filename: %ls, flags: 0x%04x", h->conf.filename,
		      h->conf.flags);

	wcstombs(filename, h->conf.filename, J_MAX_PATHNAME8);
#ifdef _WIN32	
	h->fd = open(filename, O_RDWR | O_CREAT | O_EXCL | O_BINARY, S_IREAD | S_IWRITE);
#else
	h->fd = open(filename, O_RDWR | O_CREAT | O_EXCL , S_IREAD | S_IWRITE);
#endif

	_wdeb_startup(L"opened %i ", h->fd);

	if (h->fd < 0)
		return -JE_OPEN;

	h->hdr.magic = JDB_MAGIC;
	h->hdr.type = JDB_T_JDB;
	h->hdr.ver = JDB_VER;

	if ((ret = _jdb_init_crypto(h)) < 0) {
		_jdb_cleanup_handle(h);
		return ret;
	}

	sha256((uchar *) h->conf.key, WBYTES(h->conf.key),
	       (uchar *) h->hdr.pwhash);

	/*
	   used memset instead
	   h->hdr.nblocks = 0;
	   h->hdr.nmaps = 0;
	   h->hdr.ntables = 0;
	   h->hdr.nchid = 0;
	 */

	_jdb_calc_bentries(h);

	ret = _jdb_create_map(h);
	if (ret < 0) {
		_jdb_cleanup_handle(h);
		return ret;
	}

	if ((ret = _jdb_write_hdr(h)) < 0) {
		_wdeb_startup(L"write_hdr() returns %i", ret);
		_jdb_cleanup_handle(h);
		return ret;
	}

	return 0;
}

int _jdb_open(struct jdb_handle *h)
{
	char pwhash[32];
	int ret;
	char filename[J_MAX_PATHNAME8];

	_wdeb_startup(L"called, filename: %ls, flags: 0x%04x", h->conf.filename,
		      h->conf.flags);

	wcstombs(filename, h->conf.filename, J_MAX_PATHNAME8);
#ifdef _WIN32	
	h->fd = open(filename, O_RDWR | O_BINARY);
#else
	h->fd = open(filename, O_RDWR);
#endif

	if (h->fd < 0)
		return -JE_OPEN;

	if ((ret = _jdb_init_crypto(h)) < 0) {
		_jdb_cleanup_handle(h);
		return ret;
	}

	_wdeb_startup(L"loading header");

	ret = _jdb_read_hdr(h);
	if (ret < 0) {
		//_wdeb1(L"load_hdr returns: %i", ret);
		_jdb_cleanup_handle(h);
		_wdeb_startup(L"failed %i", ret);
		return ret;
	}

	_wdeb_startup(L"checking key");

	sha256((uchar *) h->conf.key, WBYTES(h->conf.key), (uchar *) pwhash);
	if (memcmp(pwhash, h->hdr.pwhash, 32)) {
		_jdb_cleanup_handle(h);
		return -JE_PASS;
	}

	_wdeb_startup(L"loading map");

	ret = _jdb_load_map(h);
	if (ret < 0) {
		_wdeb_startup(L"load map returns: %i", ret);
		_jdb_cleanup_handle(h);
		return ret;
	}

	_wdeb_startup(L"done, flags are 0x%04x", h->hdr.flags);
/*	
	h->hdr.flags = conf->flags;
	h->hdr.crypt_type = conf->crypt_type;
	h->hdr.crc_type = conf->crc_type;
	h->hdr.blocksize = conf->blocksize;
	
	_wdeb_startup(L"returning");
*/
	return 0;
}

int jdb_open2(struct jdb_handle *h, int default_conf)
{
	int ret;
	wchar_t *filename, *key;

	//save the pointers
	filename = h->conf.filename;
	key = h->conf.key;

	if (h->magic != JDB_MAGIC) {
		_wdeb_startup(L"handle was not initialized, initing...");
		jdb_init(h);
	}
	if (h->fd != -1)
		return -JE_ISOPEN;

//      wcstombs(filename, conf->filename, J_MAX_PATHNAME8);       
//      ret = access(filename, F_OK);

	if (default_conf) {
		_wdeb_startup(L"using default config...");
		_jdb_fill_def_conf(&h->conf);
		if(!h->conf.filename){	
			h->conf.filename = (wchar_t *) malloc(WBYTES(JDB_DEF_FILENAME));

			if (!h->conf.filename)
				return -JE_MALOC;
			
			wcscpy(h->conf.filename, JDB_DEF_FILENAME);			
		}
	
		if(!h->conf.key){
			h->conf.key = (wchar_t *) malloc(WBYTES(JDB_DEF_KEY));
	
			if (!h->conf.key) {
				free(h->conf.filename);
				return -JE_MALOC;
			}
		
			wcscpy(h->conf.key, JDB_DEF_KEY);
		}
		
				
	} else {
		h->conf.filename = filename;
		h->conf.key = key;
	}	

	if (_jdb_assert_conf(&h->conf) < 0) {
		_wdeb_startup(L"failed to assert config");
		_jdb_fill_def_conf(&h->conf);
		return -JE_BADCONF;
	}

	_jdb_copy_conf_to_hdr(h);
		
	if(h->hdr.flags & JDB_O_WR_THREAD){
		ret = _jdb_init_wr_thread(h);
	}
	if(ret < 0) return ret;

	_wdeb_startup(L"creating jdb < %ls >", h->conf.filename);
	ret = _jdb_create(h);

	_wdeb_startup(L"returned %i", ret);

	if (ret == -JE_OPEN) {
		_wdeb_startup(L"openning");
		_wdeb_startup(L"openning jdb < %ls >", h->conf.filename);
		ret = _jdb_open(h);
	}
	
	if(!ret){		
		//journalling
		ret = _jdb_jrnl_open(h, h->conf.filename, 0);
		if(ret == -JE_EXISTS){
			ret = _jdb_jrnl_recover(h);
		}	
	}
	
	_wdeb_startup(L"returned %i, flags are 0x%04x", ret, h->hdr.flags);
	
	if(ret < 0)
		_jdb_request_wr_thread_exit(h);
	
	return ret;
}

int
jdb_open(struct jdb_handle *h, wchar_t * filename, wchar_t * key,
	 uint16_t flags)
{
	int ret;

	//_wdeb1(L"called for %s", filename);

	_jdb_fill_def_conf(&h->conf);

	h->conf.flags = flags;

	h->conf.filename = (wchar_t *) malloc(WBYTES(filename));
	wcscpy(h->conf.filename, filename);
	h->conf.key = (wchar_t *) malloc(WBYTES(key));
	wcscpy(h->conf.key, key);

	ret = jdb_open2(h, 0);

	return ret;
}

int jdb_sync(struct jdb_handle *h)
{

	int ret;

	ret = jdb_sync_tables(h);
	ret = _jdb_write_map(h);
	ret = _jdb_write_hdr(h);

	return ret;
}

int jdb_close(struct jdb_handle *h)
{
	int ret;
	int semval = 1;

	if (h->magic != JDB_MAGIC)
		return -JE_NOINIT;
	if (h->fd == -1)
		return -JE_NOOPEN;
		
	if((ret = jdb_sync(h)) < 0)
		return ret;		
	
	if(h->hdr.flags & JDB_O_WR_THREAD){
	
		_jdb_request_wr_thread_exit(h);	
	
	//	pthread_wait(h->wrthid);
	
		while(semval){
			if(sem_getvalue(&h->wrsem, &semval) < 0) break;
			#ifndef _WIN32
			if(semval) sleep(1);
			#endif
		}
	
	}
	
	h->fd = -1;
	h->magic = 0x0000;

	return 0;
}
