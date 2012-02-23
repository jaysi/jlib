#include "jdb.h"

/*
void _jdb_lock_hdr(struct jdb_handle* h){

}

void _jdb_unlock_hdr(struct jdb_handle* h){

}

void _jdb_unset_hdr_flag(struct jdb_handle* h, uint16_t flag, int lock){
	if(lock) _jdb_lock_hdr(h);
	h->hdr.flags &= ~flag;
	if(lock) _jdb_unlock_hdr(h);
}

void _jdb_set_hdr_flag(struct jdb_handle* h, uint16_t flag, int lock){
	if(lock) _jdb_lock_hdr(h);
	h->hdr.flags |= flag;
	if(lock) _jdb_unlock_hdr(h);	
}

uint16_t _jdb_get_hdr_flag(struct jdb_handle* h, int lock){
	uint16_t flags;
	if(lock) _jdb_lock_hdr(h);
	flags = h->hdr.flags;
	if(lock) _jdb_unlock_hdr(h);
	return flags;
}
*/

int _jdb_hdr_to_buf(struct jdb_handle* h, uchar** buf, int alloc){
	
	if(alloc){
		*buf = (uchar*)malloc(JDB_HDR_SIZE);
		if(!(*buf)) return -JE_MALOC;
	}
	
	_jdb_lock_handle(h);

	if (!(_jdb_get_handle_flag(h, 0) & JDB_HMODIF)) {
		_jdb_unlock_handle(h);
		if(alloc) free(*buf);
		return -JE_EMPTY;
	}

	_jdb_pack_hdr(&h->hdr, *buf);

	_jdb_encrypt(h, *buf, *buf, JDB_HDR_SIZE);

	_jdb_unset_handle_flag(h, JDB_HMODIF, 0);

	_jdb_unlock_handle(h);
	
	return 0;
	
}

int _jdb_write_hdr(struct jdb_handle *h)
{
	uchar buf[JDB_HDR_SIZE];
	int ret;

	_jdb_lock_handle(h);

	if (!(_jdb_get_handle_flag(h, 0) & JDB_HMODIF)) {
		_jdb_unlock_handle(h);		
		return 0;
	}
	
	_wdeb_wr(L"writing header...");

	_jdb_pack_hdr(&h->hdr, buf);

	_jdb_encrypt(h, buf, buf, JDB_HDR_SIZE);

	_jdb_unset_handle_flag(h, JDB_HMODIF, 0);

	_jdb_unlock_handle(h);

	ret = _jdb_seek_write(h, buf, JDB_HDR_SIZE, 0);

	if (ret) {
		_jdb_set_handle_flag(h, JDB_HMODIF, 1);
	}

	return ret;
}

int _jdb_read_hdr(struct jdb_handle *h)
{
	uchar buf[JDB_HDR_SIZE];
	int ret;
	
	//Locking is not needed, the header is read ONCE at the startup

	/*
	if(h->hdr.flags & JDB_WR_THREAD){
		_lock_mx(&h->rdmx);
	}
	*/

	ret = _jdb_seek_read(h, buf, JDB_HDR_SIZE, 0);

	/*
	if(h->hdr.flags & JDB_WR_THREAD){
		_unlock_mx(&h->rdmx);
	}
	*/

	if (ret < 0){
		_wdeb(L"failed to read, %i", ret);
		return ret;
	}

	_jdb_decrypt(h, buf, buf, JDB_HDR_SIZE);

	_jdb_lock_handle(h);

	ret = _jdb_unpack_hdr(&h->hdr, buf);
	_jdb_unset_handle_flag(h, JDB_HMODIF, 0);

	_jdb_unlock_handle(h);

	if (ret < 0)
		_jdb_set_handle_flag(h, JDB_HMODIF, 1);

	return ret;
}
