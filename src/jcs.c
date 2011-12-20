#include "jcs.h"
#include "jer.h"
#include "jhash.h"
#include "debug.h"
#include "hardio.h"
#include "jpack.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define _wdeb1 _wdeb		//isjcs debug

//size is len + 1 (1 for '\0')
size_t sstrlen(const char *str, size_t max_size)
{
	size_t ret;
	for (ret = 0; ret < max_size; ret++)
		if (str[ret] == '\0')
			break;

	return ret;
}

size_t swcslen(const wchar_t * str, size_t max_size)
{
	size_t ret;
	for (ret = 0; ret < max_size; ret++)
		if (str[ret] == L'\0')
			break;

	return ret;
}

#ifndef strlcpy
size_t strlcpy(char *dest, const char *src, size_t size)
{
	size_t ret = sstrlen(src, size);
	size_t len = (ret >= size) ? size - 1 : ret;

	assert(dest);

	if (size) {
		memcpy(dest, src, len);
		dest[len] = '\0';
	}
	return ret;
}
#endif
#ifndef strlcat
size_t strlcat(char *dest, const char *src, size_t count)
{
	size_t dsize = sstrlen(dest, count);
	size_t len = sstrlen(src, count);
	size_t res = dsize + len;

	assert(dest);

	dest += dsize;
	count -= dsize;
	if (len >= count)
		len = count - 1;
	memcpy(dest, src, len);
	dest[len] = 0;
	return res;
}
#endif
#ifndef wcslcpy
size_t wcslcpy(wchar_t * dest, const wchar_t * src, size_t size)
{
	size_t ret = swcslen(src, size);
	size_t len = (ret >= size) ? size - 1 : ret;

	assert(dest);

	if (size) {
		wmemcpy(dest, src, len);
		dest[len] = L'\0';
	}
	return ret;
}
#endif
#ifndef wcslcat
size_t wcslcat(wchar_t * dest, const wchar_t * src, size_t count)
{
	size_t dsize = swcslen(dest, count);
	size_t len = swcslen(src, count);
	size_t res = dsize + len;

	assert(dest);

	dest += dsize;
	count -= dsize;
	if (len >= count)
		len = count - 1;
	wmemcpy(dest, src, len);
	dest[len] = L'\0';
	return res;
}
#endif

size_t wtojcs_len(wchar_t * src, size_t max_len)
{
	size_t idst = 0;
	while (*src) {

		if (max_len) {
			if (idst >= max_len)
				return ((size_t) - 1);
		}

		if (*src < 128) {	//encode to one byte
			idst++;
		} else if ((*src > 0x007F) && (*src < 0x33FF)) {	//mapUNI
			idst += 2;
		} else if ((*src >= 0xFB50) && (*src <= 0xFDFF)) {	//mapA
			idst += 2;
		} else if ((*src >= 0xFE70) && (*src <= 0xFEFF)) {	//mapB
			idst += 2;
		} else {
			//_deb2("illegal sequence 0x%x, idst = %u", *src, idst);
			return ((size_t) - 1);
		}
		*src++;
	}
	return idst;
}

size_t wtojcs(wchar_t * src, unsigned char *dst, size_t max_len)
{
	size_t len = 0;
	uint16_t tmp;
	while (*src) {

		if (max_len) {
			if (len >= max_len)
				return ((size_t) - 1);
		}

		if (*src < 128) {
			*dst++ = *src++;
			len++;
		} else {
			len += 2;
			if (((*src) > 127) && ((*src) < 0x33ff)) {
				tmp = *src++;
			} else if (((*src) >= 0xfb50) && ((*src) <= 0xfdff)) {
				tmp = *src++;
				tmp -= 0xc750;
			} else if (((*src) >= 0xfe70) && ((*src) <= 0xfeff)) {
				tmp = *src++;
				tmp -= 0xc7c0;
			} else {
				//_deb2("src: %x, dst: %x, len = %u", *src-1, *dst, len);
				return ((size_t) - 1);
			}

			*dst++ = (tmp >> 8) | 0x80;
			*dst++ = tmp;
		}
	}
	*dst = 0x00;

	return len;
}

size_t jcstow(unsigned char *src, wchar_t * dst, size_t max_len)
{
	size_t len = 0;
	while (*src) {

		if (max_len) {
			if (len >= max_len)
				return ((size_t) - 1);
		}

		len++;
		if (*src < 128) {
			*dst++ = *src++;
		} else {
			if (*src + 1 == '\0')
				return ((size_t) - 1);
			*src &= ~0x80;
			*dst = (*src << 8);
			*src++;
			*dst |= *src++;

			if (((*dst) >= 127) && ((*dst) <= 0x33ff)) {
			} else if (((*dst) >= 0x3400) && ((*dst) <= 0x36af)) {
				*dst += 0xc750;
			} else if (((*dst) >= 0x36b0) && ((*dst) <= 0x373f)) {
				*dst += 0xc7c0;
			} else {
				//_deb2("src: %x, dst: %x, len = %u", *src-1, *dst, len);
				return ((size_t) - 1);
			}
			*dst++;
		}
	}
	*dst = 0L;
	return len;
}

int isjcs(unsigned char *src, size_t max_len)
{
	size_t len = 0;
	wchar_t dst;
	while (*src) {

		if (max_len) {
			if (len >= max_len)
				return ((size_t) - 1);
		}

		len++;
		if (*src < 128) {
			*src++;
		} else {
			*src &= ~0x80;
			dst = (*src << 8);
			*src++;
			dst |= *src++;

			if (((dst) >= 127) && ((dst) <= 0x33ff)) {
			} else if (((dst) >= 0x3400) && ((dst) <= 0x36af)) {
				dst += 0xc750;
			} else if (((dst) >= 0x36b0) && ((dst) <= 0x373f)) {
				dst += 0xc7c0;
			} else {
				_wdeb1(L"pos = %u", len);
				return 0;
			}
			dst++;
		}
	}

	_wdeb1(L"pos = %u", len);

	return len;
}

int jcslist_buf(jcslist_entry * list, uchar ** buf, size_t * len, size_t max)
{
	jcslist_entry *entry;
	uint32_t jslen;
	*len = 0L;
	for (entry = list; entry; entry = entry->next) {
		jslen = wtojcs_len(entry->wcs, max);
		if (jslen == (size_t) - 1) {
			return -JE_INV;
		}
		jslen++;	// for null termination
		*len += jslen;
	}

	*buf = (uchar *) malloc(*len);
	jslen = 0L;
	for (entry = list; entry; entry = entry->next) {
		jslen = wtojcs(list->wcs, (*buf) + jslen, max);
		jslen++;	//for null              
	}

	return 0;
}

int jcslist_parse_buf(jcslist_entry ** list, uchar * buf, uint32_t bufsize)
{
	jcslist_entry *entry, *last;
	uint32_t pos;
	size_t len;

	if (buf[bufsize - 1] != '\0')
		return -JE_INV;

	pos = 0L;
	*list = NULL;
	last = NULL;
	while (pos < bufsize) {

		len = strlen((char *)buf + pos);
		len++;		//for null

		entry = (jcslist_entry *) malloc(sizeof(jcslist_entry));
		entry->next = NULL;

		entry->wcs = (wchar_t *) malloc(len * sizeof(wchar_t));
		if (jcstow(buf + pos, entry->wcs, 0) == ((size_t) - 1))
			return -JE_INV;

		pos += len;

		if (!(*list)) {
			*list = entry;
			last = entry;
		} else {
			last->next = entry;
			last = entry;
		}
	}

	return 0;

}

void jcslist_free(jcslist_entry * list)
{
	jcslist_entry *entry;
	while (list) {
		entry = list;
		list = list->next;
		if (entry->wcs)
			free(entry->wcs);
		free(entry);
	}
}

/*
static inline void _jcs_filter_detect_bounds(wchar_t* str, size_t* start,
						size_t* end){

	size_t str_len = wcslen(str);
	
	*start = 0;

	for(*start = 0; *start < str_len, str[*start] != L'*'; (*start)++);

	for(*end = str_len; *end, str[*end] != L'*'; (*end)++);
}						

static inline int _jcs_filter_cmp_strt(wchar_t* fstr, wchar_t* sstr, 

static inline int _jcs_filter_cmp_end(

int jcs_filter_cmp(wchar_t* filter, wchar_t* str){
	size_t fstart, fend;
	size_t sstart, send;

	_jcs_filter_detect_bounds(filter, &fstart, &
	
	
}

		

	return 0;
}

*/
