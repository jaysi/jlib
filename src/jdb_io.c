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
	default:
		return -JE_IMPLEMENT;
		break;
	}

	return 0;

}

int _jdb_decrypt(struct jdb_handle *h, uchar * src, uchar * dst, size_t bufsize)
{
	register size_t pos;

	switch (h->conf.crypt_type) {
	case JDB_CRYPT_NONE:
		memcpy((char *)dst, (char *)src, bufsize);
		break;
	case JDB_CRYPT_AES:
		assert(!(bufsize%AES_BSIZE));
		for (pos = 0; pos < bufsize; pos += AES_BSIZE) {
			aes_decrypt(&h->aes, src + pos, dst + pos);
		}
		break;
	default:
		return -JE_IMPLEMENT;
		break;
	}

	return 0;
}
