#ifndef _JDB_INTERN_H
#define _JDB_INTERN_H

#include "hardio.h"
#include "sha256.h"
#include "aes.h"
#include "jer.h"
#include "debug.h"
#include "jhash.h"
#include "jbits.h"
#include "debug.h"
#include "jtime.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#define JDB_HDR_SIZE	1024

/*				types					*/
typedef uint32_t jdb_bid_t;	//block id
typedef uint32_t jdb_tid_t;	//table id
typedef uint32_t jdb_bufpos_t;	//buffer position
typedef uint16_t jdb_bsize_t;	//block size
typedef uint16_t jdb_bent_t;	//number of block entries
typedef uchar jdb_blk_t;
typedef uchar jdb_data_t;

/*
struct JDBConstHdr{
	uint16_t magic;
	uchar type;
	uchar ver;
};
*/

struct jdb_handle;

#define PWHASHSIZE	32//sha256

struct jdb_hdr {
	//fixed
	uint16_t magic;
	uchar type;
	uint32_t ver;

	//configuration
	uint16_t flags;
	uchar crc_type;
	uchar crypt_type;

	uchar wr_retries;
	uchar rd_retries;

	jdb_bsize_t blocksize;	/*must be multiplier of 2^10 (1024) */

	//limits
	jdb_bid_t max_blocks;

	//block entries
	jdb_bent_t map_bent;
	jdb_bent_t typedef_bent;
	jdb_bent_t col_typedef_bent;
	jdb_bent_t celldef_bent;
	jdb_bent_t index1_bent;
	jdb_bent_t dptr_bent;
	jdb_bent_t fav_bent;
	jdb_bent_t index0_bent;
	
	jdb_bid_t map_list_buck;
	jdb_bid_t list_buck;
	jdb_bid_t fav_load;

	//status
	jdb_bid_t nblocks;	//number of blocks
	jdb_bid_t nmaps;	//number of map blocks
	jdb_tid_t ntables;
	uint64_t chid;		//change id
	
	uchar pwhash[PWHASHSIZE];	/*sha256 */
	
	//PAD
		
	uint32_t crc32;		//header crc
}__attribute__((packed));

//block types
#define JDB_BTYPE_EMPTY		0x00
#define JDB_BTYPE_MAP		0x01
#define JDB_BTYPE_TYPE_DEF	0x02
#define JDB_BTYPE_TABLE_DEF	0x03
#define JDB_BTYPE_COL_TYPEDEF	0x04
#define JDB_BTYPE_CELLDEF	0x05
#define JDB_BTYPE_CELL_DATA	0x06	/*not a real type, just used for
					  memmory allocations*/
#define JDB_BTYPE_CELL_DATA_VAR	0x07
#define JDB_BTYPE_CELL_DATA_FIX 0x08
#define JDB_BTYPE_CELL_DATA_PTR	0x09
#define JDB_BTYPE_INDEX		0x0a	/*index, see map_entry's dtype field*/
#define JDB_BTYPE_TABLE_FAV	0x0b	/*block-rating*/
#define JDB_BTYPE_VOID		0xff	/*void blocks type, mainly for
					   memmory allocations */

#define JDB_MAP_CMP_BTYPE	(0x01<<0)
#define JDB_MAP_CMP_DTYPE	(0x01<<1)
#define JDB_MAP_CMP_TID		(0x01<<2)
#define JDB_MAP_CMP_NFUL	(0x01<<3)	/* this flag is different from
						   other flags as long as the
						   map entry value must be less
						   than the requested value */

#define JDB_MIN_BLK_SIZE	1024
#define JDB_MAX_BLK_SIZE	65536
#define JDB_DEF_BLK_SIZE	4096

/*
	map blocks only address table_def, cell_main and index blocks
*/
struct jdb_map_blk_hdr {
	uchar type;
	uchar flags;

	jdb_bent_t nset;	//number of entries set
	uint32_t crc32;		//crc of the block including header
}__attribute__((packed));

struct jdb_map_blk_entry {
	jdb_blk_t blk_type;
	/*
	   dtype:
	   in CELL_DATA_*       block data type
	   in INDEX             index type
	 */
	jdb_data_t dtype;
	jdb_bent_t nful;
	jdb_tid_t tid;
	/*
	   jdb_bid_t bid;
	   don't need this, there is a formula to find the bid if there is
	   a map entry for each block, that is:
	   assuming if map_order = 0 then map_bid = 0;
	   map_bid = map_order*(map_entry_capacity + 1);
	   bid = map_bid + blk_entry_inside_the_map;
	 */
}__attribute__((packed));

struct jdb_map {
	jdb_bid_t bid;
	uint32_t write;
	struct jdb_map_blk_hdr hdr;
	struct jdb_map_blk_entry *entry;
	struct jdb_map *next;
};

struct jdb_map_list {
	jdb_bid_t cnt;
	struct jdb_map *first;
	struct jdb_map *last;
};

struct jdb_table;

struct jdb_typedef_blk_hdr {
	uchar type;
	uchar flags;

	uint32_t crc32;
}__attribute__((packed));

struct jdb_typedef_blk_entry {
	uchar flags;
	uchar type_id;		//jdb_data_t
	uchar base;		//jdb_data_t too!
	uint32_t len;		//chunksize if var flag set
}__attribute__((packed));

struct jdb_typedef_blk {
	uint32_t write;
	jdb_bid_t bid;
	struct jdb_table *table;
	struct jdb_typedef_blk_hdr hdr;
	struct jdb_typedef_blk_entry *entry;
	struct jdb_typedef_blk *next;
};

struct jdb_typedef_list {
	jdb_bid_t cnt;
	struct jdb_typedef_blk *first;
	struct jdb_typedef_blk *last;
};

struct jdb_celldef_blk_hdr {
	uchar type;
	uchar flags;

	uint32_t crc32;
}__attribute__((packed));

struct jdb_celldef_blk_entry {
	struct jdb_celldef_blk* parent;		//pointer to the celldef_blk's write, so i can set it directly on change
	uint32_t row;
	uint32_t col;
	uint32_t data_crc32;
	uchar data_type;
	uint32_t datalen;	//real len, will be converted to the 16byte-chunk'ed len on pack/unpack
	/*
	   data chain entry pointer
	 */
	jdb_bid_t bid_entry;
	jdb_bent_t bent;
	uchar last_access_d[3];
	uchar last_access_t[3];
}__attribute__((packed));

struct jdb_celldef_blk {
	uint32_t write;
	jdb_bid_t bid;
	struct jdb_table *table;
	struct jdb_celldef_blk_hdr hdr;
	struct jdb_celldef_blk_entry *entry;
	struct jdb_celldef_blk *next;
};

struct jdb_celldef_list {
	jdb_bid_t cnt;
	struct jdb_celldef_blk *first;
	struct jdb_celldef_blk *last;
};

/*
	var. len. data chunks size is defined in table data type definition in len field
*/

#define JDB_VDATA_CHUNK_MAX	JDB_MAX_TYPE_SIZE
#define JDB_VDATA_CHUNK_MIN	32
#define JDB_VDATA_CHUNK_DEF	64

/*
	data can't be bigger than blocksize - data_blk_hdr size
*/

struct jdb_cell_data_blk_hdr {
	uchar type;
	uchar flags;
	uchar dtype;//already shown in map, put here for performance issues
	uint32_t crc32;
}__attribute__((packed));

struct jdb_cell_data_blk {
	uint32_t write;
	jdb_bid_t bid;
	uint16_t entsize;
	uint16_t base_len;
	jdb_bent_t bmapsize;
	jdb_bent_t nent;
	jdb_data_t data_type;
	jdb_data_t base_type;
	jdb_bent_t maxent;

	struct jdb_table *table;
	struct jdb_cell_data_blk_hdr hdr;
	uchar* bitmap;
	uchar* datapool;
	struct jdb_cell_data_blk *next;
};

struct jdb_cell_data_list{
	jdb_bid_t cnt;
	struct jdb_cell_data_blk* first;
	struct jdb_cell_data_blk* last;
};

struct jdb_cell_data_ptr_blk_hdr {
	uchar type;
	uchar flags;
	//uchar data_type; not needed, shown in map entry       
	uint32_t crc32;
}__attribute__((packed));

struct jdb_cell_data_ptr_blk_entry {
	struct jdb_cell_data_ptr_blk* parent;
	jdb_bid_t bid;
	jdb_bent_t bent;//begin
	jdb_bent_t nent;/*continues chunks inside the same block,
			INVAL if the entire block allocated*/
	jdb_bid_t nextdptrbid;
	jdb_bent_t nextdptrbent;
	struct jdb_cell_data_ptr_blk_entry* next;
}__attribute__((packed));

struct jdb_cell_data_ptr_blk{
	uint32_t write;
	jdb_bid_t bid;
	jdb_bent_t nent;
	struct jdb_cell_data_ptr_blk_hdr hdr;
	struct jdb_cell_data_ptr_blk_entry* entry;
	struct jdb_cell_data_ptr_blk* next;
	
};

struct jdb_cell_data_ptr_list{

	jdb_bid_t cnt;
	struct jdb_cell_data_ptr_blk* first;
	struct jdb_cell_data_ptr_blk* last;

};

#define JDB_CELL_DATA_NOTLOADED	0
#define JDB_CELL_DATA_UPTODATE	1
#define JDB_CELL_DATA_MODIFIED	2

struct jdb_cell {
	uint32_t write;
	int data_stat;
	struct jdb_table *table;
	struct jdb_celldef_blk_entry *celldef;
	struct jdb_cell_data_ptr_blk_entry *dptr;/*data pointers, they point to the
						chunks inside the lists of data blocks
						the pointer is array and allocated as
						buckets
						*/
	struct jdb_cell* next;
	uchar *data;
};

struct jdb_cell_list {
	jdb_bid_t cnt;
	struct jdb_cell *first;
	struct jdb_cell *last;
};

struct jdb_table_def_blk_hdr {
	uchar type;
	uchar flags;

	uint16_t indexes;
	jdb_tid_t tid;
	uint16_t namelen;	//jcs len
	uint32_t nrows;
	uint32_t ncols;
	uint64_t ncells;
	jdb_bid_t ncol_typedef;	//number of col_typedef entries
	uint32_t crc32;
}__attribute__((packed));

struct jdb_table_def_blk {
	uint32_t write;
	struct jdb_table *table;
	struct jdb_table_def_blk_hdr hdr;
	wchar_t *name;
};

//#define JDB_COL_TYPEDEF_TYPE_COL_TYPE        0x01

struct jdb_col_typedef_blk_hdr {
	uchar type;
	uchar flags;

	uint32_t crc32;
}__attribute__((packed));

struct jdb_col_typedef_blk_entry {
	uchar type_id;
	uint32_t col;
}__attribute__((packed));

struct jdb_col_typedef {
	uint32_t write;
	jdb_bid_t bid;
	struct jdb_table *table;
	struct jdb_col_typedef_blk_hdr hdr;
	struct jdb_col_typedef_blk_entry *entry;
	struct jdb_col_typedef *next;
}__attribute__((packed));

struct jdb_col_typedef_list {
	jdb_bid_t cnt;
	struct jdb_col_typedef *first;
	struct jdb_col_typedef *last;
};

struct jdb_fav_blk_hdr{
	uchar type;
	uchar flags;
	uint32_t crc32;
}__attribute__((packed));

struct jdb_fav_blk_entry{
	jdb_bid_t bid;
	uint32_t nhits;
}__attribute__((packed));

struct jdb_fav_blk{
	uint32_t write;
	//struct jdb_table* table;
	jdb_bid_t bid;
	struct jdb_fav_blk_hdr hdr;
	struct jdb_fav_blk_entry* entry;
	struct jdb_fav_blk* next;
};

struct jdb_fav_blk_list{
	jdb_bid_t cnt;
	struct jdb_fav_blk* first;
	struct jdb_fav_blk* last;
};

/*				indexes					*/

//index types, set as map_entry's dtype field, length same as jdb_data_t
#define JDB_ITYPE0	0x00
#define JDB_ITYPE1	0x01
#define JDB_ITYPE2	0x02

/*
	index type 0 (GLOBAL-UNIQUE), hash -> id
	Usage:
		TableNameHash -> TableID	TID_INX
		Allows table renaming.
*/

struct jdb_index_blk_hdr{

	uchar type;
	uchar flags;

	uint32_t crc32;		//crc of the block including header
}__attribute__((packed));

struct jdb_index0_blk_entry{
	uint32_t hash;
	uint32_t tid;//JDB_ID_INVAL for empty entries
}__attribute__((packed));

struct jdb_index0_blk{
	uint32_t write;
	jdb_bid_t bid;
	struct jdb_index_blk_hdr hdr;
	struct jdb_index0_blk_entry* entry;
	struct jdb_index0_blk* next;
};

struct jdb_index0_blk_list{
	uint32_t cnt;
	struct jdb_index0_blk* first;
	struct jdb_index0_blk* last;

};

/*
	index type 1 (LOCAL-UNIQUE), hash -> row, col
	Usage:
		FullDataHash -> Row, Column
		
		Allows fast data search
		limitations are support for exact data match only
*/

struct jdb_index1_blk_entry {
	uint64_t hash64;	
	uint32_t row;
	uint32_t col;
}__attribute__((packed));

struct jdb_index1 {
	uint32_t write;

	struct jdb_table *table;
	struct jdb_index_blk_hdr hdr;
	struct jdb_index1_blk_entry *entry;
	struct jdb_index1 *next;
};

struct jdb_index1_list {
	jdb_bid_t cnt;
	struct jdb_index1 *first;
	struct jdb_index1 *last;
};

/*
	index type 2 (LOCAL-NON_UNIQUE), hash -> row, col
	Usage:
		DataHash -> Row, Column
		
		Allows fast data search even for parts of data in a cell
		May return multiple results, Same record may not exist in
		a table, thus insertion is SLOW due to needed searches
*/

struct jdb_index2_blk_entry {
	uint32_t hash32;	
	uint32_t row;
	uint32_t col;
}__attribute__((packed));

struct jdb_index2 {
	uint32_t write;

	struct jdb_table *table;
	struct jdb_index_blk_hdr hdr;
	struct jdb_index2_blk_entry *entry;
	struct jdb_index2 *next;
};

struct jdb_index2_list {
	jdb_bid_t cnt;
	struct jdb_index2 *first;
	struct jdb_index2 *last;
};

/*			Table-Definition			*/

struct jdb_table {
	jdb_bid_t table_def_bid;

	jdb_bid_t nwrblk;
	jdb_bid_t map_chg_list_size, map_chg_ptr;
	jdb_bid_t* map_chg_list;
	
	struct jdb_table_def_blk main;
	struct jdb_col_typedef_list col_typedef_list;
	struct jdb_typedef_list typedef_list;
	struct jdb_celldef_list celldef_list;
	struct jdb_cell_list cell_list;
	struct jdb_index1_list index1_list;
	struct jdb_index2_list index2_list;
	struct jdb_cell_data_ptr_list data_ptr_list;
	struct jdb_cell_data_list data_list;
	struct jdb_fav_blk_list fav_list;
	struct jdb_table *next;
};

struct jdb_table_list {
	jdb_tid_t cnt;
	struct jdb_table *first;
	struct jdb_table *last;
};

struct jdb_wr_fifo_entry{
	jdb_bid_t nblocks;
	jdb_bid_t* bid_list;
	uint32_t bufsize;
	uchar* hdrbuf;
	uchar* buf;
	struct jdb_wr_fifo_entry* next;
};

struct jdb_wr_fifo{
	uint32_t cnt;
	struct jdb_wr_fifo_entry* first, *last;
};


/*
	init / cleanup
*/
int _jdb_cleanup_handle(struct jdb_handle *h);

/*
	packing, they set/check crc too!
*/
int _jdb_pack_hdr(struct jdb_hdr *hdr, uchar * buf);
int _jdb_unpack_hdr(struct jdb_hdr *hdr, uchar * buf);
int _jdb_pack_map(struct jdb_handle* h, struct jdb_map *blk, uchar * buf);
int _jdb_unpack_map(struct jdb_handle* h, struct jdb_map *blk, uchar * buf);
int _jdb_pack_typedef(struct jdb_handle* h, struct jdb_typedef_blk *blk, uchar * buf);
int _jdb_unpack_typedef(struct jdb_handle* h, struct jdb_typedef_blk *blk, uchar * buf);
int _jdb_pack_celldef(struct jdb_handle* h, struct jdb_celldef_blk *blk, uchar * buf);
int _jdb_unpack_celldef(struct jdb_handle* h, struct jdb_celldef_blk *blk, uchar * buf);
int _jdb_pack_table_def(struct jdb_handle* h, struct jdb_table_def_blk *blk, uchar * buf);
int _jdb_unpack_table_def(struct jdb_handle* h, struct jdb_table_def_blk *blk, uchar * buf);
int _jdb_pack_col_typedef(struct jdb_handle* h, struct jdb_col_typedef *blk, uchar * buf);
int _jdb_unpack_col_typedef(struct jdb_handle* h, struct jdb_col_typedef *blk, uchar * buf);
int _jdb_pack_data(struct jdb_handle* h, struct jdb_cell_data_blk *blk, uchar * buf);
int _jdb_unpack_data(	struct jdb_handle* h, struct jdb_table* table,
			struct jdb_cell_data_blk *blk, jdb_data_t type_id,
			uchar * buf);

/*
	hashing
*/
uint32_t _jdb_hash(unsigned char *key, size_t key_len);
uchar _jdb_pearson(unsigned char *key, size_t len);
#define _jdb_crc32(key, size) _jdb_hash(key, size)
#define _jdb_key32(key, size) _jdb_hash(key, size)

/*
	read/write
*/
#define JDB_WR_SHUTDOWN	(0x01<<0)	//the db is shutting down, do not use AIO and you may change the buffers' contents
#define JDB_WR_NONEW	(0x01<<1)	//do not allocate new memmory (i.e. you may change the buffer contents and it will be valid till the write is complete.

int _jdb_read_hdr(struct jdb_handle *h);
int _jdb_write_hdr(struct jdb_handle *h);
int _jdb_write_block(struct jdb_handle *h, uchar * block, jdb_bid_t bid,
		     uchar flags);
int _jdb_read_block(struct jdb_handle *h, uchar * block, jdb_bid_t bid);
int _jdb_write_map_blk(struct jdb_handle *h, struct jdb_map *map);
int _jdb_read_map_blk(struct jdb_handle *h, struct jdb_map *map);
int _jdb_seek_write(struct jdb_handle *h, uchar * buf, size_t len,
		    off_t offset);
int _jdb_seek_read(struct jdb_handle *h, uchar * buf, size_t len, off_t offset);
int _jdb_write_typedef_blk(struct jdb_handle *h, struct jdb_typedef_blk *blk);
int _jdb_read_typedef_blk(struct jdb_handle *h, struct jdb_typedef_blk *blk);
int _jdb_write_col_typedef_blk(struct jdb_handle *h, struct jdb_col_typedef *blk);
int _jdb_read_col_typedef_blk(struct jdb_handle *h, struct jdb_col_typedef *blk);
int _jdb_write_celldef_blk(struct jdb_handle *h, struct jdb_celldef_blk *blk);
int _jdb_read_celldef_blk(struct jdb_handle *h, struct jdb_celldef_blk *blk);
int _jdb_read_table_def_blk(struct jdb_handle *h, struct jdb_table *table);
int _jdb_write_table_def_blk(struct jdb_handle *h, struct jdb_table *table);
int _jdb_write_data_blk(struct jdb_handle *h, struct jdb_cell_data_blk *blk);
int _jdb_read_data_blk(	struct jdb_handle *h, struct jdb_table* table,
			struct jdb_cell_data_blk *blk, jdb_data_t type_id);
int _jdb_write_fav_blk(struct jdb_handle *h, struct jdb_fav_blk *blk);
int _jdb_read_fav_blk(struct jdb_handle *h, struct jdb_fav_blk *blk);			


/*
	access control
*/
void _jdb_fill_date(uchar d[3], uchar t[3]);

/*
	locking
*/
void _jdb_lock_handle(struct jdb_handle *h);
void _jdb_unlock_handle(struct jdb_handle *h);
//void _jdb_lock_hdr(struct jdb_handle* h);
//void _jdb_unlock_hdr(struct jdb_handle* h);

/*
	handle
*/
void _jdb_unset_handle_flag(struct jdb_handle *h, uint16_t flag, int lock);
void _jdb_set_handle_flag(struct jdb_handle *h, uint16_t flag, int lock);
uint16_t _jdb_get_handle_flag(struct jdb_handle *h, int lock);

/*
	header
*/
//void _jdb_unset_hdr_flag(struct jdb_handle* h, uint16_t flag, int lock);
//void _jdb_set_hdr_flag(struct jdb_handle* h, uint16_t flag, int lock);

/*
	crypto
*/
int _jdb_decrypt(struct jdb_handle *h, uchar * src, uchar * dst,
		 size_t bufsize);
int _jdb_encrypt(struct jdb_handle *h, uchar * src, uchar * dst,
		 size_t bufsize);

/*
	allocation
*/
//#define _jdb_blk_off(n, hsize, bsize) ((n)==0?hsize:((hsize) + (((n)-1)*(bsize))))

  /*
     jdb_bid_t bid;
     don't need this, there is a formula to find the bid if there is
     a map entry for each block, that is:
     assuming if map_order = 0 then map_bid = 0;
     map_bid = map_order*(map_entry_capacity + 1);
     bid = map_bid + blk_entry_inside_the_map;
   */

//#define _jdb_calc_bid(map_no, map_bent_capacity, map_bent) ((map_no*(map_bent_capacity+1))+map_bent)
jdb_bent_t _jdb_calc_bent(uint16_t blk_size, uint16_t ent_size,
			  uint16_t blk_hdr_size, uint16_t * pad);
int _jdb_calc_data(uint16_t blocksize, uint16_t entsize, jdb_bent_t * bent,
		    jdb_bent_t * bmapsize);
jdb_bid_t _jdb_get_empty_bid(struct jdb_handle *h, int zero_map_entry);
jdb_bid_t _jdb_find_table_def_bid(struct jdb_handle *h, uint32_t key);

/*
	calculators
*/
int _jdb_calc_data(jdb_bsize_t blocksize, uint16_t entsize,
		    jdb_bent_t * bent, jdb_bent_t * bmapsize);
size_t _jdb_base_dtype_size(uchar dtype);
size_t _jdb_data_len(struct jdb_handle *h, struct jdb_table *table,
		      uchar dtype, uchar * base_type, size_t * base_len);
size_t _jdb_typedef_len(struct jdb_handle *h, struct jdb_table *table,
			jdb_data_t type_id, jdb_data_t * base_type,
			size_t * base_len);

/*
	mapping
*/

int _jdb_free_map_list(struct jdb_handle *h);
jdb_bent_t _jdb_max_nful(struct jdb_handle *h, jdb_blk_t btype);
int _jdb_create_map(struct jdb_handle *h);
int _jdb_set_map_entry(struct jdb_handle *h,
		       struct jdb_map *map,
		       jdb_bent_t map_bent,
		       jdb_blk_t btype,
		       jdb_data_t dtype, jdb_tid_t tid, jdb_bent_t nful);
int _jdb_set_map_nful_by_bid(struct jdb_handle *h, jdb_bid_t bid,
			     jdb_bent_t nful);
int _jdb_inc_map_nful(struct jdb_handle *h, struct jdb_map *map,
		      jdb_bent_t map_bent);
int _jdb_dec_map_nful(struct jdb_handle *h, struct jdb_map *map,
		      jdb_bent_t map_bent);
int _jdb_get_map_nful_by_bid(struct jdb_handle *h, jdb_bid_t bid, jdb_bent_t* nful);		      
jdb_bid_t _jdb_get_empty_map_entry(struct jdb_handle *h, jdb_blk_t btype,
				   jdb_data_t dtype, jdb_tid_t tid,
				   jdb_bent_t nful);
int _jdb_get_empty_map_entries(struct jdb_handle *h, jdb_blk_t * btype,
			       jdb_data_t * dtype, jdb_tid_t * tid,
			       jdb_bent_t * nful, jdb_bid_t * bid, jdb_bid_t n);
int _jdb_rm_map_bid_entries(struct jdb_handle *h, jdb_bid_t * bid, jdb_bid_t n);
int _jdb_write_map(struct jdb_handle *h);
int _jdb_load_map(struct jdb_handle *h);
int _jdb_inc_map_nful_by_bid(struct jdb_handle *h, jdb_bid_t bid, jdb_bent_t n);
int _jdb_dec_map_nful_by_bid(struct jdb_handle *h, jdb_bid_t bid, jdb_bent_t n);
int _jdb_list_map_match(struct jdb_handle *h, jdb_blk_t btype,
			jdb_data_t dtype, jdb_tid_t tid, jdb_bent_t nful,
			uchar cmp_flags, jdb_bid_t ** bid, jdb_bid_t * n);
jdb_bid_t _jdb_find_first_map_match(struct jdb_handle *h, jdb_blk_t btype,
				    jdb_data_t dtype, jdb_tid_t tid,
				    jdb_bent_t nful, uchar cmp_flags);
struct jdb_map_blk_entry* _jdb_get_map_entry_ptr(struct jdb_handle* h, jdb_bid_t bid);
int _jdb_map_chg_proc(struct jdb_handle* h, jdb_bid_t bid, struct jdb_table* table, jdb_bid_t map_bid);			    

/*
	memmory
*/
uchar *_jdb_get_blockbuf(struct jdb_handle *h, uchar btype);
void _jdb_release_blockbuf(struct jdb_handle *h, uchar * buf);

/*
	types
*/
int _jdb_load_col_typedef(struct jdb_handle *h, struct jdb_table *table);
int _jdb_find_typedef(struct jdb_handle *h,
			    struct jdb_table *table, uchar type_id,
			    struct jdb_typedef_blk **blk,
			    struct jdb_typedef_blk_entry **entry);
int _jdb_load_typedef(struct jdb_handle *h, struct jdb_table *table);
void _jdb_free_typedef_list(struct jdb_table *table);
void _jdb_free_col_typedef_list(struct jdb_table *table);			    

/*
	data
*/
int _jdb_load_all_cell_data(struct jdb_handle* h, struct jdb_table* table);
void _jdb_free_vdata_list(struct jdb_table *table);
void _jdb_free_data_list(struct jdb_table *table);
void _jdb_free_cell_list(struct jdb_table* table);
int _jdb_alloc_cell_data(struct jdb_handle *h, struct jdb_table *table,
		     struct jdb_cell *cell, jdb_data_t dtype,
		     uchar* data, size_t datalen, int first);
		     
void _jdb_free_celldef_list(struct jdb_table *table);
int _jdb_create_dptr_chain(struct jdb_handle* h, struct jdb_table* table,
				struct jdb_cell_data_ptr_blk_entry** list,
				jdb_bid_t needed,
				jdb_bid_t* first_bid, jdb_bent_t* first_bent);
int _jdb_load_dptr_chain(	struct jdb_handle* h, struct jdb_table* table,
				struct jdb_cell* cell,
				struct jdb_cell_data_ptr_blk_entry** list);
int _jdb_free_data_ptr_list(struct jdb_table *table);						     

/*
	search
*/
int _jdb_table_handle(struct jdb_handle* h, wchar_t* name, struct jdb_table** table);

int _jdb_init_wr_thread(struct jdb_handle* h);
int _jdb_request_wr_thread_exit(struct jdb_handle* h);
int jdb_sync_table(struct jdb_handle *h, wchar_t * name);
int _jdb_unpack_data_ptr(struct jdb_handle* h, struct jdb_cell_data_ptr_blk *blk,
			uchar* buf);
int _jdb_pack_data_ptr(struct jdb_handle* h, struct jdb_cell_data_ptr_blk *blk,
			uchar* buf);
int _jdb_unpack_fav(struct jdb_handle* h, struct jdb_fav_blk *blk,
			uchar * buf);
int _jdb_pack_fav(struct jdb_handle* h, struct jdb_fav_blk *blk,
			uchar * buf);
int _jdb_load_cell_data(struct jdb_handle* h, struct jdb_table* table, struct jdb_cell* cell);
int _jdb_rm_map_entries_by_tid(struct jdb_handle *h, jdb_tid_t tid);
int _jdb_rm_map_bid_entry(struct jdb_handle *h, jdb_bid_t bid);
int _jdb_add_fdata(struct jdb_handle *h, struct jdb_table *table, uchar dtype,
			uchar * data, jdb_bid_t* databid, jdb_bent_t* databent);
int _jdb_rm_data_seg(struct jdb_handle *h, struct jdb_table *table,
	      jdb_bid_t bid, jdb_bent_t bent, jdb_bent_t nent);															
int _jdb_rm_dptr_chain(		struct jdb_handle* h, struct jdb_table* table,
				struct jdb_cell* cell,
				jdb_bid_t bid, jdb_bent_t bent);
int _jdb_rm_fav(struct jdb_handle *h, struct jdb_table* table, jdb_bid_t bid);
int _jdb_typedef_flags_by_ptr(struct jdb_handle* h, struct jdb_table *table, jdb_data_t type_id, uchar* flags);
int _jdb_inc_fav(struct jdb_handle *h, struct jdb_table* table,
		    jdb_bid_t bid);
int _jdb_write_data_ptr_blk(struct jdb_handle *h, struct jdb_cell_data_ptr_blk *blk);
int _jdb_read_data_ptr_blk(struct jdb_handle *h, struct jdb_cell_data_ptr_blk *blk);
int _jdb_load_celldef(struct jdb_handle *h, struct jdb_table *table);
int _jdb_load_data_ptr(struct jdb_handle *h, struct jdb_table *table);
int _jdb_load_fav(struct jdb_handle *h, struct jdb_table *table);
int _jdb_load_fav_blocks(struct jdb_handle* h, struct jdb_table* table);
void _jdb_free_fav_list(struct jdb_table *table);
int _jdb_load_cells(struct jdb_handle *h, struct jdb_table *table, int loaddata);
int _jdb_request_table_write(struct jdb_handle* h, struct jdb_table* table);
int _jdb_hdr_to_buf(struct jdb_handle* h, uchar** buf, int alloc);
		    
int _jdb_changelog_reg(struct jdb_handle* h, uint64_t chid , uchar cmd, int ret, uchar nargs, size_t* argsize, ...);
int _jdb_jrnl_recover(struct jdb_handle* h);
int _jdb_changelog_reg_end(struct jdb_handle* h, uint64_t chid, int ret);			
int _jdb_jrnl_open(struct jdb_handle *h, wchar_t * filename, uint16_t flags);
uint64_t _jdb_get_chid(struct jdb_handle *h, int lock);

/*
	MACROs
*/

#define _jdb_blk_off(n, hsize, bsize) ((hsize) + ((n)*(bsize)))

#define _jdb_calc_bid(M_map_bid, M_map_bent) (M_map_bid + M_map_bent + 1)

#define _JDB_MAP_ORDER(bid, map_bent) ((bid)/((map_bent)+1))
#define _JDB_MAP_BID(bid, map_bent) (_JDB_MAP_ORDER(bid, map_bent)*((map_bent)+1))
#define _JDB_MAP_BENT(bid, map_bent) (((bid)%((map_bent)+1))-1)

#define JDB_MAP_CHG_LIST_BUCK	10
/* re-written as function call
#define _JDB_MAP_CHG_PROC(h, bid, table, map_bid)\
	if(h->hdr.flags & JDB_O_WR_THREAD){\
		int cntr;\
		for(cntr = 0; cntr < table->map_chg_list_size; cntr++){\
			if(table->map_chg_list[cntr] == map_bid){\
				_wdeb_tm(L"found m_c_l[ %u ] = %u", cntr, map_bid);\
				break;\
			}\
		}\
		if(cntr == table->map_chg_list_size){\
			_wdeb_tm(L"cntr[ %u ] = m_c_l_size %u", cntr, table->map_chg_list_size);\
			table->map_chg_list_size += JDB_MAP_CHG_LIST_BUCK;\
			table->map_chg_list = realloc(table->map_chg_list, table->map_chg_list_size);\
			table->map_chg_list[table->map_chg_list_size - JDB_MAP_CHG_LIST_BUCK] = map_bid;\
			_wdeb_tm(L"m_c_l_size now %u, set pos %u to %u", table->map_chg_list_size, table->map_chg_list_size - JDB_MAP_CHG_LIST_BUCK, map_bid);\
		} else {\
			_wdeb_tm(L"cntr[ %u ] set to %u", cntr, map_bid);\
		}\
	}
*/
#define _JDB_SET_WR(h, blk, bid, table, map_chg_flag)\
	_wdeb(L"_JDB_SET_WR->bid= %u (IS different with map bid), chg_flag = %i, blk->write = %u", bid, map_chg_flag, (blk)->write);\
	if((blk)->write){\
		(blk)->write++;\
		if(map_chg_flag) _jdb_map_chg_proc(h, bid, table, _JDB_MAP_BID(bid, h->hdr.map_bent));\
	} else {\
		(blk)->write = 1;\
		table->nwrblk++;\
		if(map_chg_flag) _jdb_map_chg_proc(h, bid, table, _JDB_MAP_BID(bid, h->hdr.map_bent));\
	}


#endif
