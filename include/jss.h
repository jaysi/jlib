/*

/jaysi\
~~~ ~~~

*/

#ifndef _JSS_H
#define _JSS_H

//j's stream(simple) store library

#include "jtypes.h"
#include "aes.h"
#include "wchar.h"

#if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
     __BYTE_ORDER == __LITTLE_ENDIAN) || \
    (defined(i386) || defined(__i386__) || defined(__i486__) || \
     defined(__i586__) || defined(__i686__) || defined(vax) || defined(MIPSEL))
#define JN_LITTLE_ENDIAN 1
#define JN_BIG_ENDIAN 0
#elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
       __BYTE_ORDER == __BIG_ENDIAN) || \
      (defined(sparc) || defined(POWERPC) || defined(mc68000) || defined(sel))
#define JN_LITTLE_ENDIAN 0
#define JN_BIG_ENDIAN 1
#else
#define JN_LITTLE_ENDIAN 0
#define JN_BIG_ENDIAN 0
#endif

#define JSS_MAGIC	0xd1e2
#define JSS_FILE_HEADER_SIZE
#define JSS_FILE_EXT	".jss"
#define JSS_FILE_EXT_W	L".jss"
#define JSS_MAXNAME	2048
#define JSS_MAXFILENAME	256
#define JSS_MAXKEY	256

#define JSS_HDR_SIZE	48

#define JSS_DB_VER	0x01

//the header is always packed, the rest is in native byte order
#define JSS_BE		(0x01<<0)	//big endian host
#define JSS_CRYPT	(0x01<<1)
#define JSS_CRC		(0x01<<2)
#define JSS_COMP	(0x01<<3)	//compressed

struct jss_file_hdr {

	uint16_t magic;		//first three entries are constant in all j dbs!
	uchar type;
	uchar ver;
	uchar flags;
	uint32_t crc32;		//header crc
	uint32_t nrec;
	uchar pwhash[32];	/*sha256 */
	uchar pad[3];
} __attribute__ ((packed));

#define JSS_VOID	0x0000
#define JSS_JCS		0x0001
#define JSS_BITMAP	0x0002
#define JSS_JPEG	0x0003
#define JSS_CHAR	0x0004
#define JSS_INT16	0x0005
#define JSS_INT32	0x0006
#define JSS_JTABLE	0x0007
#define JSS_RAW		0x0010
#define JSS_RESERVED	0x0010

#define JSS_REC_DONTLOAD	(0x01<<0)	//do not load at open
#define JSS_REC_FILE		(0x01<<1)	//the record was a file

#define JSS_REC_DONTTOUCH	(0x01<<7)	//temporary flag, not written on disk

struct jss_rec_hdr {
	uchar flags;
	uint32_t crc32;
	uint32_t offset;
	uint32_t datalen;
	uint32_t id;
	uint16_t type;
	uint16_t namelen;	//name is stored at the begining of the data in jcs format
} __attribute__ ((packed));

struct jss_rec_entry {
	struct jss_rec_hdr hdr;
	wchar_t *name;
	unsigned char *data;
	struct jss_rec_entry *next;
};

struct jss_rec_list {
	//uint32_t cnt; use h->hdr.nrec
	struct jss_rec_entry *first;
	struct jss_rec_entry *last;
};

//#define JSS_CRYPT     (0x01<<1)
//#define JSS_CRC       (0x01<<2)
#define JSS_OPEN	(0x01<<3)
#define JSS_INIT	(0x01<<4)
#define JSS_RDONLY	(0x01<<5)
#define JSS_MODIFIED	(0x01<<6)

struct jss_handle {
	uint16_t magic;
	uchar flags;
	int fd;
	wchar_t *filename;
	aes_context aes;
	uint32_t end_off;
	struct jss_file_hdr hdr;
	struct jss_rec_list *rec_list;
};

#define jss_size(size)	((size)%AES_BSIZE == 0)?(size):((((size) / AES_BSIZE)+1)*AES_BSIZE)
#define jss_malloc(size) malloc(jss_size(size))

#ifdef __cplusplus
extern "C" {
#endif

	extern __declspec(dllimport) int jss_init(struct jss_handle *h);
	extern __declspec(dllimport) void jss_destroy(struct jss_handle *h);
	extern __declspec(dllimport) int jss_open(struct jss_handle *h, wchar_t * filename, wchar_t * key, uchar flags);	//open/create
	extern __declspec(dllimport) void jss_close(struct jss_handle *h);
	extern __declspec(dllimport) int jss_add(struct jss_handle *h,
						 uint32_t id, uint16_t type,
						 uchar flags, wchar_t * name,
						 uint32_t datalen,
						 unsigned char *data);
	extern __declspec(dllimport) int jss_rm(struct jss_handle *h,
						uint32_t id);
	extern __declspec(dllimport) int jss_find(struct jss_handle *h,
						  uint32_t id,
						  struct jss_rec_entry **rec);
	extern __declspec(dllimport) int jss_find_name(struct jss_handle *h,
						       wchar_t * name,
						       struct jss_rec_list
						       *rec_list);
	extern __declspec(dllimport) int jss_add_file(struct jss_handle *h,
						      uint32_t id,
						      uint16_t type,
						      uchar flags,
						      wchar_t * filename);
	extern __declspec(dllimport) int jss_sync(struct jss_handle *h);
	extern __declspec(dllimport) void jss_close(struct jss_handle *h);
	extern __declspec(dllimport) int jss_get_rec(struct jss_handle *h,
						     uint32_t id,
						     uint16_t * type,
						     uchar * flags,
						     wchar_t ** name,
						     uint32_t * datalen,
						     unsigned char **data);
	extern __declspec(dllimport) int jss_list(struct jss_handle *h,
						  wchar_t * name, uint16_t type,
						  struct jss_rec_list *list);
	extern __declspec(dllimport) void jss_free_list(struct jss_rec_list
							*list);

#ifdef __cplusplus
}
#endif
#endif
