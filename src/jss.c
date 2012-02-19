#include "jss.h"
#include "hardio.h"
#include "jer.h"
#include "jhash.h"
#include "jpack.h"
#include "aes.h"
#include "sha256.h"
#include "jcs.h"
#include "jdb.h"
#include "debug.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define _wdeb1 _wdeb

int _jss_seek_write(int fd, char *buf, size_t len, off_t offset)
{
	if (lseek(fd, offset, SEEK_SET) == ((off_t) - 1))
		return -JE_SEEK;
	if (hard_write(fd, buf, len) != len)
		return -JE_WRITE;
	return 0;
}

int _jss_seek_read(int fd, char *buf, size_t len, off_t offset)
{
	if (lseek(fd, offset, SEEK_SET) == ((off_t) - 1)) {
		return -JE_SEEK;
	}
	if (hard_read(fd, buf, len) != len) {
		return -JE_READ;
	}
	return 0;
}

static inline int _jss_write_hdr(struct jss_handle *h)
{
	uchar buf[JSS_HDR_SIZE];
	int i;

#if (defined JN_BIG_ENDIAN == 1)
	h->hdr.flags |= JSS_BE;
#else
	h->hdr.flags &= ~JSS_BE;
#endif

	h->hdr.crc32 = 0UL;
	h->hdr.crc32 =
	    joat_hash((uchar *) & h->hdr, sizeof(struct jss_file_hdr));
	_wdeb1(L"writing header, crc = 0x%x, HDR_SIZE=%u, sizeof(hdr) = %u",
	       h->hdr.crc32, JSS_HDR_SIZE, sizeof(struct jss_file_hdr));
	for (i = 0; i < JSS_HDR_SIZE; i += AES_BSIZE) {
		aes_encrypt(&h->aes, (uchar *) & h->hdr + i, buf + i);
	}
	return _jss_seek_write(h->fd, (char *)buf, JSS_HDR_SIZE, 0);
}

static inline int _jss_read_hdr(struct jss_handle *h)
{
	uchar buf[JSS_HDR_SIZE];
	int ret;
	uint32_t crc, i;

	//decrypt, unpack, crc

	_wdeb1(L"reading header...");

	ret = _jss_seek_read(h->fd, (char *)buf, JSS_HDR_SIZE, 0);
	if (ret < 0) {
		return ret;
	}

	for (i = 0; i < JSS_HDR_SIZE; i += AES_BSIZE) {
		aes_decrypt(&h->aes, buf + i, (uchar *) & h->hdr + i);
	}

	crc = h->hdr.crc32;
	h->hdr.crc32 = 0UL;
	h->hdr.crc32 =
	    joat_hash((uchar *) & h->hdr, sizeof(struct jss_file_hdr));
	_wdeb1(L"stored crc = 0x%x, calc crc = 0x%x", crc, h->hdr.crc32);
	if (crc == h->hdr.crc32)
		return 0;
	else
		return -JE_CRC;
}

static inline int _jss_write_rec(struct jss_handle *h,
				 struct jss_rec_entry *rec)
{
	uint32_t total, i;
	uchar *buf;

	if ((rec->hdr.flags & JSS_REC_DONTLOAD) && !(rec->data)) {
		_wdeb1(L"the call fails here, forbidden operation");
		return -JE_FORBID;
	}

	_wdeb1(L"namelen = %u, datalen = %u", rec->hdr.namelen,
	       rec->hdr.datalen);

	total = jss_size((rec->hdr.namelen + 1 + rec->hdr.datalen));

	_wdeb1(L"writing record %u @ offset %u, total encrypted size = %u",
	       rec->hdr.id, rec->hdr.offset, total);

	buf = (uchar *) malloc(total);
	if (!buf)
		return -JE_MALOC;

	_wdeb1(L"crc32 = 0x%x, namelen = %u, datalen = %u", rec->hdr.crc32,
	       rec->hdr.namelen, rec->hdr.datalen);

	for (i = 0; i < total; i += AES_BSIZE) {

		aes_encrypt(&h->aes, rec->data + i, buf + i);

	}

	if (_jss_seek_write(h->fd, (char *)buf, total, rec->hdr.offset)) {
		free(buf);
		return -JE_WRITE;
	}

	return 0;
}

static inline int _jss_read_rec(struct jss_handle *h, struct jss_rec_entry *rec)
{

	uint32_t total, i;
	uchar *buf;

	_wdeb1(L"namelen = %u, datalen = %u", rec->hdr.namelen,
	       rec->hdr.datalen);

	total = jss_size((rec->hdr.namelen + 1 + rec->hdr.datalen));

	_wdeb1(L"reading record: %u, total = %u", rec->hdr.id, total);

	buf = (uchar *) malloc(total);
	if (!buf)
		return -JE_MALOC;

	if (_jss_seek_read(h->fd, (char *)buf, total, rec->hdr.offset)) {
		free(buf);
		return -JE_READ;
	}

	for (i = 0; i < total; i += AES_BSIZE) {
		aes_decrypt(&h->aes, buf + i, buf + i);
		if (!i) {
			_wdeb(L"first chunk: %s", buf);
		}
	}

	if (rec->hdr.namelen) {
		rec->name =
		    (wchar_t *) malloc((rec->hdr.namelen + 1) *
				       sizeof(wchar_t));
		jcstow(buf, rec->name, total);
	}

	if (rec->hdr.datalen) {
		rec->data = (uchar *) malloc(total);
		if (!rec->data) {
			free(rec->name);
			free(buf);
			return -JE_MALOC;
		}
		memcpy(rec->data, buf, total);

		if (rec->hdr.crc32 !=
		    joat_hash(buf, rec->hdr.datalen + rec->hdr.namelen + 1)) {
			free(buf);
			_wdeb1(L"stored crc = 0x%x, calc'ed crc = 0x%x",
			       rec->hdr.crc32, joat_hash(rec->data,
							 rec->hdr.namelen +
							 rec->hdr.datalen));
			return -JE_CRC;
		}

	}

	free(buf);

	return 0;

}

static inline void _jss_calc_offset(struct jss_handle *h)
{
	uint32_t total = 0UL;
	struct jss_rec_entry *rec;
	uint32_t next_off;

	total = jss_size(h->hdr.nrec * sizeof(struct jss_rec_hdr));
	total += JSS_HDR_SIZE;
	next_off = 0UL;
	for (rec = h->rec_list->first; rec; rec = rec->next) {

		rec->hdr.offset = total + next_off;

		if (rec->hdr.datalen) {
			rec->hdr.crc32 =
			    joat_hash(rec->data,
				      rec->hdr.datalen + rec->hdr.namelen + 1);
		} else {
			rec->hdr.crc32 = 0UL;
		}

		next_off =
		    jss_size((rec->hdr.offset + rec->hdr.datalen +
			      rec->hdr.namelen + 1));
	}
}

static inline int _jss_write_headers(struct jss_handle *h)
{
	uchar *buf;
	uint32_t total, i;
	struct jss_rec_entry *rec;

	total = jss_size(h->hdr.nrec * sizeof(struct jss_rec_hdr));

	_wdeb1(L"writing %u headers, total encrypted size %u bytes",
	       h->hdr.nrec, total);

	buf = (uchar *) malloc(total);
	if (!buf)
		return -JE_MALOC;

	i = 0;
	for (rec = h->rec_list->first; rec; rec = rec->next) {

		/*
		   if(rec == h->rec_list->first){
		   rec->hdr.offset = JSS_HDR_SIZE + total;
		   } else {
		   rec->hdr.offset = next_off;
		   }

		   rec->hdr.crc32 = joat_hash(rec->data, rec->hdr.namelen + rec->hdr.datalen);

		   next_off = jss_size((rec->hdr.offset + rec->hdr.datalen + rec->hdr.namelen + 1));

		   _wdeb1(L"i = %u, data offset = %u, next offset = %u", i, rec->hdr.offset, next_off);
		 */

		memcpy(buf + i, (uchar *) & rec->hdr,
		       sizeof(struct jss_rec_hdr));
		i += sizeof(struct jss_rec_hdr);
	}

	for (i = 0; i < total; i += AES_BSIZE) {
		aes_encrypt(&h->aes, buf + i, buf + i);
	}

	if (_jss_seek_write(h->fd, (char *)buf, total, JSS_HDR_SIZE)) {
		_wdeb1(L"fails here");
		free(buf);
		return -JE_WRITE;
	}

	free(buf);

	return 0;

}

static inline int _jss_read_headers(struct jss_handle *h)
{
	uchar *buf;
	uint32_t total, i;
	struct jss_rec_entry *rec;

	total = jss_size(h->hdr.nrec * sizeof(struct jss_rec_hdr));

	_wdeb1(L"reading %u headers, total encrypted size %u bytes",
	       h->hdr.nrec, total);

	buf = (uchar *) malloc(total);
	if (!buf)
		return -JE_MALOC;

	if (_jss_seek_read(h->fd, (char *)buf, total, JSS_HDR_SIZE)) {
		free(buf);
		return -JE_READ;
	}

	_wdeb1(L"read headers, decrypting...");

	for (i = 0; i < total; i += AES_BSIZE) {
		aes_decrypt(&h->aes, buf + i, buf + i);
	}

	for (i = 0; i < h->hdr.nrec; i++) {
		rec = (struct jss_rec_entry *)
		    malloc(sizeof(struct jss_rec_entry));
		if (!rec)
			return -JE_MALOC;
		memcpy((uchar *) & rec->hdr,
		       buf + i * sizeof(struct jss_rec_hdr),
		       sizeof(struct jss_rec_hdr));
		rec->next = NULL;
		_wdeb(L"added #id: %u", rec->hdr.id);
		if (!h->rec_list->first) {
			h->rec_list->first = rec;
			h->rec_list->last = rec;
		} else {
			h->rec_list->last->next = rec;
			h->rec_list->last = h->rec_list->last->next;
		}
	}

	free(buf);

	return 0;

}

int jss_find(struct jss_handle *h, uint32_t id, struct jss_rec_entry **rec)
{

	if (h->magic != JSS_MAGIC)
		return -JE_MAGIC;
	if (!(h->flags & JSS_INIT))
		return -JE_NOINIT;
	if (!(h->flags & JSS_OPEN))
		return -JE_NOOPEN;

	for (*rec = h->rec_list->first; *rec; *rec = (*rec)->next)
		if ((*rec)->hdr.id == id)
			break;

	if (!(*rec))
		return -JE_NOTFOUND;

	return 0;
}

int jss_add(struct jss_handle *h, uint32_t id, uint16_t type,
	    uchar flags, wchar_t * name, uint32_t datalen, unsigned char *data)
{

	struct jss_rec_entry *rec;

	_wdeb1(L"adding #%u", id);

	if (h->magic != JSS_MAGIC)
		return -JE_MAGIC;
	if (!(h->flags & JSS_INIT))
		return -JE_NOINIT;
	if (!(h->flags & JSS_OPEN))
		return -JE_NOOPEN;

	if (h->flags & JSS_RDONLY) {
		_wdeb1(L"read-only");
		return -JE_NOPERM;
	}
	//if((!datalen && data) || (datalen && !data)) return -JE_FORBID;

	if (!jss_find(h, id, &rec))
		return -JE_EXISTS;

	rec = (struct jss_rec_entry *)malloc(sizeof(struct jss_rec_entry));
	if (!rec)
		return -JE_MALOC;

	rec->hdr.id = id;
	rec->hdr.type = type;
	rec->hdr.flags = flags;
	rec->hdr.datalen = datalen;

	//total = rec->hdr.datalen;

	if (name) {
		rec->hdr.namelen = wtojcs_len(name, JSS_MAXNAME);
		rec->name = malloc(WBYTES(name));
		if (!rec->name) {
			free(rec);
			return -JE_MALOC;
		}
		wcscpy(rec->name, name);
		//      total += rec->hdr.namelen;
	} else {
		rec->hdr.namelen = 0;
		rec->name = NULL;
	}

	rec->data = (uchar *) jss_malloc((datalen + rec->hdr.namelen + 1));
	if (!rec->data) {
		free(rec->name);
		free(rec);
		return -JE_MALOC;
	}

	if (name) {
		wtojcs(name, rec->data, rec->hdr.namelen);
	}

	memcpy(rec->data + rec->hdr.namelen + 1, data, datalen);

//      memcpy(rec->data, data, datalen);

	rec->next = NULL;

	if (!h->rec_list->first) {
		//rec->offset = JSS_HDR_SIZE;
		h->rec_list->first = rec;
		h->rec_list->last = rec;
		h->hdr.nrec = 1UL;
	} else {
		//rec->offset = h->rec_list->last->offset + jss_size((h->rec_list->last->hdr.namelen + h->rec_list->last->hdr.datalen));
		h->rec_list->last->next = rec;
		h->rec_list->last = h->rec_list->last->next;
		h->hdr.nrec++;
	}

	h->flags |= JSS_MODIFIED;

	return 0;
}

int jss_add_file(struct jss_handle *h, uint32_t id, uint16_t type, uchar flags,
		 wchar_t * filename)
{
	int fd;
	char *fname;
	uchar *buf;
	struct stat s;
	int ret;

	_wdeb1(L"adding #%u", id);

	if (h->magic != JSS_MAGIC)
		return -JE_MAGIC;
	if (!(h->flags & JSS_INIT))
		return -JE_NOINIT;
	if (!(h->flags & JSS_OPEN))
		return -JE_NOOPEN;

	if (h->flags & JSS_RDONLY) {
		_wdeb1(L"read-only");
		return -JE_NOPERM;
	}

	fname = (char *)malloc(wtojcs_len(filename, JSS_MAXFILENAME));
	if (!fname)
		return -JE_MALOC;

	wtojcs(filename, (uchar *) fname, JSS_MAXFILENAME);

	fd = open(fname, O_RDONLY);

	free(fname);

	if (fstat(fd, &s) < 0) {
		close(fd);
		return -JE_STAT;
	}

	buf = (uchar *) malloc(s.st_size);
	if (!buf) {
		close(fd);
		return -JE_MALOC;
	}

	if (hard_read(fd, (char *)buf, s.st_size) < s.st_size) {
		free(buf);
		close(fd);
		return -JE_READ;
	}
	close(fd);

	ret = jss_add(h, id, type, flags | JSS_REC_FILE,
		      filename, s.st_size, buf);

	free(buf);

	return ret;

}

int jss_rm(struct jss_handle *h, uint32_t id)
{
	struct jss_rec_entry *rec, *prev;

	if (h->magic != JSS_MAGIC)
		return -JE_MAGIC;
	if (!(h->flags & JSS_INIT))
		return -JE_NOINIT;
	if (!(h->flags & JSS_OPEN))
		return -JE_NOOPEN;

	if (h->flags & JSS_RDONLY) {
		_wdeb1(L"read-only");
		return -JE_NOPERM;
	}

	rec = h->rec_list->first;
	prev = rec;

	while (rec->hdr.id != id) {
		prev = rec;
		rec = rec->next;
	}

	if (!rec)
		return -JE_NOTFOUND;

	if (rec->hdr.id == h->rec_list->first->hdr.id) {
		h->rec_list->first = h->rec_list->first->next;
	} else if (rec->hdr.id == h->rec_list->last->hdr.id) {
		prev->next = NULL;
		h->rec_list->last = prev;
	} else {
		prev->next = rec->next;
	}

	h->hdr.nrec--;

	if (rec->data)
		free(rec->data);
	if (rec->name)
		free(rec->name);
	free(rec);

	h->flags |= JSS_MODIFIED;

	return 0;

}

int jss_sync(struct jss_handle *h)
{
	int ret;
	struct jss_rec_entry *rec;

	_wdeb1(L"synchronising...");

	if (h->magic != JSS_MAGIC)
		return -JE_MAGIC;
	if (!(h->flags & JSS_INIT))
		return -JE_NOINIT;
	if (!(h->flags & JSS_OPEN))
		return -JE_NOOPEN;

	if (h->flags & JSS_RDONLY) {
		_wdeb1(L"read-only");
		return -JE_NOPERM;
	}

	if (!(h->flags & JSS_MODIFIED))
		return 0;

	_wdeb1(L"writing hdr");

	if ((ret = _jss_write_hdr(h)) < 0) {
		_wdeb(L"returns %i", ret);
		return ret;
	}

	_jss_calc_offset(h);

	if ((ret = _jss_write_headers(h)) < 0) {
		_wdeb(L"returns %i", ret);
		return ret;
	}

	_wdeb1(L"writing data...");

	for (rec = h->rec_list->first; rec; rec = rec->next) {

		_wdeb1(L"record #%u", rec->hdr.id);

		if (!(rec->hdr.flags & JSS_REC_DONTLOAD)) {
			ret = _jss_write_rec(h, rec);
			if (ret < 0) {
				_wdeb(L"fails here, Id: %u", rec->hdr.id);
				return ret;
			}
		} else {
			if (!rec->data) {
				ret = _jss_read_rec(h, rec);
				if (ret < 0) {
					_wdeb(L"fails here, Id: %u",
					      rec->hdr.id);
					return ret;
				}
			}

			ret = _jss_write_rec(h, rec);
			if (ret < 0) {
				_wdeb(L"fails here, Id: %u", rec->hdr.id);
				return ret;
			}

		}
	}

	h->flags &= ~JSS_MODIFIED;

	return 0;

}

int jss_init(struct jss_handle *h)
{

	assert(sizeof(struct jss_file_hdr) == JSS_HDR_SIZE);

	if ((h->magic == JSS_MAGIC) && (h->flags & JSS_INIT))
		return -JE_ALREADYINIT;

	h->rec_list =
	    (struct jss_rec_list *)malloc(sizeof(struct jss_rec_list));
	if (!h->rec_list)
		return -JE_MALOC;

	h->filename = NULL;

	h->rec_list->first = NULL;

	h->end_off = 0UL;

	h->magic = JSS_MAGIC;
	h->flags = JSS_INIT;

	return 0;
}

int _jss_create(struct jss_handle *h, wchar_t * wfilename, wchar_t * key)
{

	char filename[JSS_MAXFILENAME];

	_wdeb1(L"creating jss...");

	wcstombs(filename, wfilename, JSS_MAXFILENAME);
	h->fd = open(filename, O_RDWR | O_CREAT | O_EXCL, S_IREAD | S_IWRITE);

	if (h->fd < 0)
		return -JE_OPEN;

	h->hdr.magic = JDB_MAGIC;
	h->hdr.type = JDB_T_JSS;
	h->hdr.ver = JSS_DB_VER;
	h->hdr.nrec = 0UL;

	sha256((uchar *) key, WBYTES(key), h->hdr.pwhash);
	aes_set_key(&h->aes, h->hdr.pwhash, 256);

	return 0;
}

int _jss_just_open(struct jss_handle *h, wchar_t * wfilename, wchar_t * key)
{
	char pwhash[32];
	uchar aes_key[32];
	int ret;
	char filename[JSS_MAXFILENAME];
	struct jss_rec_entry *rec;

	_wdeb1(L"openning jss...");

	wcstombs(filename, wfilename, JSS_MAXFILENAME);
	h->fd = open(filename, O_RDWR);

	if (h->fd < 0)
		return -JE_OPEN;

	sha256((uchar *) key, WBYTES(key), aes_key);
	aes_set_key(&h->aes, aes_key, 256);

	ret = _jss_read_hdr(h);
	if (ret < 0)
		return ret;

	sha256((uchar *) key, WBYTES(key), (uchar *) pwhash);
	if (memcmp(pwhash, h->hdr.pwhash, 32)) {
		return -JE_PASS;
	}

	ret = _jss_read_headers(h);
	if (ret < 0) {
		_wdeb(L"fails here (read headers)");
		return ret;
	}

	for (rec = h->rec_list->first; rec; rec = rec->next) {
		if (!(rec->hdr.flags & JSS_REC_DONTLOAD)) {
			ret = _jss_read_rec(h, rec);
			if (ret < 0) {
				_wdeb(L"fails here, id = %u", rec->hdr.id);
				return ret;
			}
		} else {
			rec->data = NULL;
		}
	}

	return 0;

}

int jss_open(struct jss_handle *h, wchar_t * filename, wchar_t * key,
	     uchar flags)
{
	int ret;

	if (h->magic != JSS_MAGIC) {
		_wdeb(L"handle was not init, initializing...");
		if ((ret = jss_init(h)) < 0)
			return ret;
	}

	if (h->flags & JSS_OPEN) {
		_wdeb(L"file is already open");
		return -JE_ISOPEN;
	}

	h->flags |= flags;

	if (!(flags & JSS_RDONLY)) {
		ret = _jss_create(h, filename, key);
	} else {
		ret = 1;
	}

	if (ret) {
		_wdeb(L"file exists or read_only flag set, openning...");
		ret = _jss_just_open(h, filename, key);
	}

	if (!ret) {
		h->flags |= JSS_OPEN;
		return 0;
	}

	h->flags &= ~JSS_OPEN;
	return ret;

}

void jss_free(struct jss_handle *h)
{
	struct jss_rec_entry *rec;

	if (h->magic != JSS_MAGIC)
		return;
	if (!(h->flags & JSS_INIT))
		return;
	if (!(h->flags & JSS_OPEN))
		return;

	while (h->rec_list->first) {
		rec = h->rec_list->first;
		h->rec_list->first = h->rec_list->first->next;
		if (rec->hdr.namelen)
			free(rec->name);
		if (rec->hdr.datalen && !(rec->hdr.flags & JSS_REC_DONTLOAD))
			free(rec->data);
		free(rec);
	}

}

void jss_close(struct jss_handle *h)
{

	if (h->magic != JSS_MAGIC)
		return;
	if (!(h->flags & JSS_INIT))
		return;
	if (!(h->flags & JSS_OPEN))
		return;

	jss_sync(h);
	close(h->fd);
	jss_free(h);
	free(h->rec_list);
	h->magic = 0;
	h->flags = 0;

	_wdeb(L"closed");
}

/*
	use this call to access records, this call loads the
	DONT_LOAD flagged records, too!
	you may free *name but do not free the *data pointer
*/
int jss_get_rec(struct jss_handle *h, uint32_t id, uint16_t * type,
		uchar * flags, wchar_t ** name, uint32_t * datalen,
		unsigned char **data)
{
	int ret;
	struct jss_rec_entry *rec;

	for (rec = h->rec_list->first; rec; rec = rec->next) {
		if (rec->hdr.id == id) {
			if ((rec->hdr.flags & JSS_REC_DONTLOAD) && !(rec->data)) {
				if ((ret = _jss_read_rec(h, rec)) < 0)
					return ret;
			}

			if (rec->hdr.namelen) {

				*name =
				    (wchar_t *) malloc((rec->hdr.namelen + 1) *
						       sizeof(wchar_t));
				if (!(*name))
					return -JE_MALOC;

				if (jcstow(rec->data, *name, rec->hdr.namelen)
				    == ((size_t) - 1)) {
					free(*name);
					return -JE_FORBID;
				}
			} else {
				*name = NULL;
			}

			/*      ***     */
			*data = rec->data + rec->hdr.namelen + 1;
			/*      ***     */

			*type = rec->hdr.type;

			*datalen = rec->hdr.datalen;

			return 0;
		}
	}

	return -JE_NOTFOUND;

}

void jss_free_list(struct jss_rec_list *list)
{
	struct jss_rec_entry *entry;

	while (list->first) {
		entry = list->first;
		list->first = list->first->next;
		free(entry);
	}

}

int jss_list(struct jss_handle *h, wchar_t * name, uint16_t type,
	     struct jss_rec_list *list)
{
	int do_add = 0;
	struct jss_rec_entry *entry, *add;

	list->first = NULL;

	for (entry = h->rec_list->first; entry; entry = entry->next) {
		if (name && entry->name) {
			if (!wcscmp(name, entry->name)) {
				do_add = 1;
			}
		} else {
			if (entry->hdr.type == type) {
				do_add = 1;
			}
		}
		if (do_add) {
			do_add = 0;
			add = (struct jss_rec_entry *)
			    malloc(sizeof(struct jss_rec_entry));
			if (!add) {
				jss_free_list(list);
				return -JE_MALOC;
			}
			memcpy(&add->hdr, &entry->hdr,
			       sizeof(struct jss_rec_hdr));
			add->name = entry->name;
			add->data = entry->data;
			add->next = NULL;

			if (!list->first) {
				list->first = add;
				list->last = add;
			} else {
				list->last->next = add;
				list->last = add;
			}
		}
	}

	return 0;
}
