/*

/jaysi\
~~~ ~~~

*/


#ifndef _JCS_H
#define _JCS_H

#include "jtypes.h"
#include <wchar.h>

size_t sstrlen(const char *str, size_t max_len);
size_t swcslen(const wchar_t * str, size_t max_len);

#ifndef strlcpy
size_t strlcpy(char *dest, const char *src, size_t size);
#endif
#ifndef strlcat
size_t strlcat(char *dest, const char *src, size_t count);
#endif
#ifndef wcslcpy
size_t wcslcpy(wchar_t * dest, const wchar_t * src, size_t size);
#endif
#ifndef wcslcat
size_t wcslcat(wchar_t * dest, const wchar_t * src, size_t count);
#endif

#define WBYTES(str) ((wcslen(str)+1)*sizeof(wchar_t))
#define CBYTES(str) (strlen((char*)str)+1)
#define CWBYTES(str) ((strlen((char*)str)+1)*sizeof(wchar_t))

#define JS_RAW		0x00
#define JS_UT8		0x01
#define JS_UT16		0x02
#define JS_CHAR		0x03
#define JS_UT16L	0x04	//little endian
#define JS_STD		0x05	//my standard! every transaction MUST use this!
#define JS_EMPTY	0x0f

#define JS_STD_UNI_SUPP	0x3400	//unicode table support
#define JS_STD_MAX	0x3750	/*max supported value, unicode table range plus
				   arabic presentation forms-A    FB50~FDFF (2AF)
				   mapped to                      3400~36AF -C750
				   arabic presentation forms-B    FE70~FEFF (8F)
				   mapped to                      36B0~373F -C7C0
				   needs 14 bits
				 */
#define JS_STD_ARA_A_MINUS	0xC750
#define JS_STD_ARA_B_MINUS	0xC7C0

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct _jcs_list {
		//size_t jcslen;
		//uchar* jcs;
		wchar_t *wcs;
		struct _jcs_list *next;
	} jcslist_entry;

	int isjcs(unsigned char *str, size_t max_len);
	size_t wtojcs_len(wchar_t * src, size_t max_len);
	size_t wtojcs(wchar_t * src, unsigned char *dst, size_t max_len);
	size_t jcstow(unsigned char *src, wchar_t * dst, size_t max_len);
	int jcslist_parse_buf(jcslist_entry ** list, unsigned char *buf,
			      uint32_t bufsize);
	int jcslist_buf(jcslist_entry * list, unsigned char **buf,
			size_t * bufsize, size_t max);
	void jcslist_free(jcslist_entry * list);

#ifdef __cplusplus
}
#endif
#endif
