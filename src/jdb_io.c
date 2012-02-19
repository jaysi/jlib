#include "jdb.h"

#define _wdeb_io(f, a...)
#define _wdeb_crc(f, a...)

int _jdb_seek_write(struct jdb_handle *h, uchar * buf, size_t len, off_t offset)
{
	_wdeb_io(L"seeking to offset < %u > to WRITE < %u > bytes on fd < %i >",
		 offset, len, h->fd);
	if (lseek(h->fd, offset, SEEK_SET) == ((off_t) - 1))
		return -JE_SEEK;
	if (hard_write(h->fd, (char *)buf, len) != len)
		return -JE_WRITE;
	
	_wdeb_crc(L"crc of written buffer 0x%08x", _jdb_crc32(buf, len));

	return 0;
}

int _jdb_seek_read(struct jdb_handle *h, uchar * buf, size_t len, off_t offset)
{
#ifndef NDEBUG
	size_t red;
#endif
	_wdeb_io(L"seeking to offset < %u > to READ < %u > bytes on fd < %i >",
		 offset, len, h->fd);
	if (lseek(h->fd, offset, SEEK_SET) == ((off_t) - 1))
		return -JE_SEEK;
#ifndef NDEBUG
	if ((red = hard_read(h->fd, (char *)buf, len)) != len){
		_wdeb_io(L"red: %u != %u", red, len);
		return -JE_READ;
	}
	
#else
	if (hard_read(h->fd, (char *)buf, len) != len){		
		return -JE_READ;
	}
#endif		
	_wdeb_crc(L"crc of read buffer 0x%08x", _jdb_crc32(buf, len));
		
	return 0;
}

int _jdb_encrypt(struct jdb_handle *h, uchar * src, uchar * dst, size_t bufsize)
{
	register size_t pos;

	switch (h->conf.crypt_type) {
	case JDB_CRYPT_NONE:
		memcpy((char *)dst, (char *)src, bufsize);
		break;
	case JDB_CRYPT_AES:
		assert(!(bufsize%AES_BSIZE));
		for (pos = 0; pos < bufsize; pos += AES_BSIZE) {
			aes_encrypt(&h->aes, src + pos, dst + pos);
		}
		break;
	case JDB_CRYPT_DES3:
		assert(!(bufsize%DES3_BSIZE));
		for (pos = 0; pos < bufsize; pos += DES3_BSIZE) {
			des3_encrypt(&h->des3, src + pos, dst + pos);
		}
		break;			
	default:
		return -JE_IMPLEMENT;
		break;
	}

	return 0;

}

int _jdb_decrypt(struct jdb_handle *h, uchar * src, uchar * dst, size_t bufsize)
{
	register size_t pos;

	switch (h->conf.crypt_type){
	case JDB_CRYPT_NONE:
		memcpy((char *)dst, (char *)src, bufsize);
		break;
	case JDB_CRYPT_AES:
		assert(!(bufsize%AES_BSIZE));
		for (pos = 0; pos < bufsize; pos += AES_BSIZE) {
			aes_decrypt(&h->aes, src + pos, dst + pos);
		}
		break;
	case JDB_CRYPT_DES3:
		assert(!(bufsize%DES3_BSIZE));
		for (pos = 0; pos < bufsize; pos += DES3_BSIZE) {
			des3_decrypt(&h->des3, src + pos, dst + pos);
		}
		break;	
	default:
		return -JE_IMPLEMENT;
		break;
	}

	return 0;
}

int _jdb_write_crypt(struct jdb_handle* h, int fd, uchar** buf, size_t bufsize, off_t offset){
	size_t bsize;
	size_t crsize;
	int ret;
	
	switch (h->conf.crypt_type) {
	case JDB_CRYPT_NONE:
		bsize = 1;
		break;
	case JDB_CRYPT_AES:
		bsize = AES_BSIZE;
		break;
	case JDB_CRYPT_DES3:
		bsize = DES3_BSIZE;
		break;	
	default:
		return -JE_IMPLEMENT;
		break;
	}
	
	crsize = crypt_size(bufsize, bsize);
	
	if(crsize != bufsize) *buf = (uchar*)realloc(*buf, crsize);
	if(!(*buf)) return -JE_MALOC;
	
	if((ret = _jdb_encrypt(h, *buf, *buf, crsize)) < 0) return ret;
	
	if (lseek(fd, offset, SEEK_SET) == ((off_t) - 1))
		return -JE_SEEK;
	if (hard_write(fd, (char *)(*buf), crsize) != crsize)
		return -JE_WRITE;

	return 0;	
}

int _jdb_read_crypt(struct jdb_handle* h, int fd, uchar** buf, size_t bufsize, off_t offset){
	size_t bsize;
	size_t crsize;
	int ret;
	
	switch (h->conf.crypt_type) {
	case JDB_CRYPT_NONE:
		bsize = 1;
		break;
	case JDB_CRYPT_AES:
		bsize = AES_BSIZE;
		break;
	case JDB_CRYPT_DES3:
		bsize = DES3_BSIZE;
		break;	
	default:
		return -JE_IMPLEMENT;
		break;
	}
	
	crsize = crypt_size(bufsize, bsize);
	
	if(crsize != bufsize) *buf = (uchar*)malloc(crsize);
	if(!(*buf)) return -JE_MALOC;
	
	if((ret = _jdb_decrypt(h, *buf, *buf, crsize)) < 0) return ret;
	
	if (lseek(fd, offset, SEEK_SET) == ((off_t) - 1))
		return -JE_SEEK;
	if (hard_read(fd, (char *)(*buf), crsize) != crsize)
		return -JE_READ;

	return 0;	
}
