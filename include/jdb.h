/*

/jaysi\
~~~ ~~~

*/

#ifndef _JDB_H
#define _JDB_H

#include "jdb_intern.h"
#include "jdb_chlog.h"
#include "jdb_list.h"
#include "jtypes.h"
#include "jtable.h"
#include "jcs.h"
#include "jdb_debug.h"
#include "jcrypt.h"
#ifndef JDB_SINGLE_THREAD
#define _J_USE_PTHREAD
#include "jcompat.h"
#define JDB_USE_WR_THREAD
#endif

/*				stat modes				*/
#define JDB_FSTAT_EXIST	0x0001	//check for existance
#define JDB_FSTAT_READ		//check for readability, NA
#define JDB_FSTAT_WRITE		//check for writablity, NA
#define JDB_FSTAT_STRUCT	//check db structure, a key is required, NA

/*				types					*/
/*			DEFINITION MOVED TO jdb_intern.h		*/

#define JDB_MAGIC	0x1bcd

/*				db-types				*/
#define JDB_T_ERR	0x00	//returns on errors
#define JDB_T_JAC	0x01
#define JDB_T_JDB	0x02
#define JDB_T_JSS	0x03
#define JDB_T_NOT_J	0xff
#define JDB_T_LAST	JDB_T_JSS

/*				base types				*/
#define JDB_TYPE_EMPTY	0x00	//empty
#define JDB_TYPE_BYTE	0x01	//8-bits
#define JDB_TYPE_FIXED_BASE_START	JDB_TYPE_BYTE
#define JDB_TYPE_SHORT	0x02	//16-bits
#define JDB_TYPE_LONG	0x03	//32-bits
#define JDB_TYPE_LONG_LONG	0x04	//64-bits
#define JDB_TYPE_DOUBLE	0x05	//64-bits
#define JDB_TYPE_FIXED_BASE_END		JDB_TYPE_DOUBLE
#define JDB_TYPE_CHAR	0x06	//utf-8/ascii string
#define JDB_TYPE_JCS	0x07	//jaysi's charset
#define JDB_TYPE_WIDE	0x08	//wide charset, fixed 32-bit string, host byte order
//#define JDB_TYPE_FORMULA	0x09	//formula, not necessary in this level
#define JDB_TYPE_RAW	0x09	//raw data
#define JDB_TYPE_NULL	0x0f	//null type, also shows the reserved area for base-types.

#define JDB_TYPE_UNSIGNED	(0x01<<0)	//unsigned value
#define JDB_TYPE_VAR		(0x01<<1)	//variable length

#define JDB_TYPELEN_EMPTY	0
#define JDB_TYPELEN_BYTE	1
#define JDB_TYPELEN_SHORT	2
#define JDB_TYPELEN_LONG	4
#define JDB_TYPELEN_LONG_LONG	8
#define JDB_TYPELEN_DOUBLE	8
#define JDB_TYPELEN_CHAR	1
#define JDB_TYPELEN_JCS		2
#define JDB_TYPELEN_WIDE	4
#define JDB_TYPELEN_RAW		1
#define JDB_TYPELEN_NULL	0

/*
	versioning:
	byte1: engine version major
	byte2: engine version minor
	byte3: not in use
	byte4: database format version
	
	engine major 00
	engine minor 01
	database format 01 --> fixed 4k blocks
*/
#define JDB_ENG_MAJOR_VER	0x00000000
#define JDB_ENG_MINOR_VER	0x00010000
#define JDB_RESERVED_VER	0x00000000
#define JDB_FORMAT_VER		0x00000001

#define JDB_ENG_MAJOR_VER_MASK	0xff000000
#define JDB_ENG_MINOR_VER_MASK	0x00ff0000
#define JDB_RESERVED_VER_MASK	0x0000ff00
#define JDB_FORMAT_VER_MASK	0x000000ff

#define JDB_VER			(JDB_ENG_MAJOR_VER | JDB_ENG_MINOR_VER | JDB_RESERVED_VER | JDB_FORMAT_VER)

/*				limits					*/
#define JDB_MAX_BSIZE		0xffff
#define JDB_MIN_BSIZE		1024
#define JDB_HASH_INVAL		0xffffffff
#define JDB_HASH_EMPTY		0x00000000
#define JDB_ID_INVAL		0xffffffff
#define JDB_SIZE_INVAL		0xffffffff
#define JDB_ID_EMPTY		0x00000000
#define JDB_BENT_INVAL		0xffff
#define JDB_MAX_TYPE_SIZE	1024
//#define JDB_MAX_TABLE_NAME	depends on blocksize...

/*
	Configuration
*/

//flags
#define JDB_O_JRNL	(0x0001<<0)	//enable journalling
#define JDB_O_LOG	(0x0001<<1)	//enable logging
#define JDB_O_SNAP	(0x0001<<2)	//enable snapshots
#define JDB_O_RDONLY	(0x0001<<3)	//read-only
#define JDB_O_PACK	(0x0001<<4)	//pack everything, in any case the header is packed
#define JDB_O_AIO	(0x0001<<5)	//enable aio
#define JDB_O_WR_THREAD	(0x0001<<6)	//enable threaded write
#define JDB_O_UNDO	(0x0001<<6)	//enable undo function


//encryption-crc
#define JDB_CRC_NONE	0x00
#define JDB_CRC_WIKI	0x01
#define JDB_CRYPT_NONE	JCRYPT_NONE
#define JDB_CRYPT_AES	JCRYPT_AES
#define JDB_CRYPT_DES3	JCRYPT_DES3

//default
#define JDB_DEF_FLAGS		JDB_O_PACK | JDB_O_WR_THREAD | JDB_O_JRNL | JDB_O_SNAP | JDB_O_UNDO
#define JDB_DEF_BLOCK_SIZE	4096
#define JDB_DEF_FILENAME	L"1.jdb"
#define JDB_DEF_KEY		L"jaysi13@gmail.com"
#define JDB_DEF_MAPLIST_BUCK	25
#define JDB_DEF_FAV_LOAD	10
#define JDB_DEF_CRC		JDB_CRC_WIKI
#define JDB_DEF_CRYPT		JDB_CRYPT_AES
#define JDB_DEF_JRNL_EXT	L".jdbjrnl"
#define JDB_DEF_PREV_EXT	L".jdbprev"
#define JDB_DEF_LIST_BUCK	10

struct jdb_conf {
	uint16_t flags;
	uint16_t blocksize;
	uchar crc_type;
	uchar crypt_type;
	uint32_t map_list_buck;	//jdb_bid_t
	uint32_t list_buck;
	uint32_t max_blocks;
	uint32_t fav_load;//number of favourite blocks to load
	wchar_t *filename;
	wchar_t *key;
	uchar	str_key[JDB_STR_KEY_SIZE];
}__attribute__((packed));

/*
	JDB Handle
*/

#define JDB_CALL_ERR		(0x0001<<0)	//Error happened on LAST call (usefull?)
#define JDB_HMODIF		(0x0001<<1)	//header modified (and needs to be rewritten at shutdown)
#define JDB_WR			(0x0001<<4)	//write     *
#define JDB_RD			(0x0001<<5)	//read      *
//#define JDB_OPEN		(0x0001<<7)	//db is open
#define JDB_USE_DEF_CONF	(0x0001<<8)	//use default configuration

struct jdb_handle {

	uint16_t magic;
	int fd;
	aes_context aes;
	des3_context des3;
	rc4_ctx	rc4;
	uint16_t flags;

	//journalling
	int jfd;
	jthread_t jrnlthid;
	jmx_t jmx;
	jsem_t jsem;
	//uint32_t jopid;
	
	struct jdb_jrnl_fifo jrnl_fifo;

	jthread_t wrthid;
	jmx_t wrmx;
	jsem_t wrsem;
	jmx_t rdmx;

	struct jdb_wr_fifo wr_fifo;
	
/*
*/
#ifndef NDEBUG
	char* wrbuf1;
	char* rdbuf1;
#endif

	struct jdb_conf conf;

	struct jdb_hdr hdr;
	struct jdb_map_list map_list;
	struct jdb_table_list table_list;	//only loaded tables will be put on this list
	
	//global indexing
	struct jdb_index0_blk_list index0_list;
	
	//undo
	struct jdb_changelog_list undo_list;
	struct jdb_changelog_list redo_list;
	
	//snapshots
	struct jdb_snapshot_list snapshot_list;
};

/*				table-flags					*/
#define JDB_TABLE_BIND_COLS		(0x01<<0)	//the cell's col field cannot exceed the table's col
#define JDB_TABLE_BIND_ROWS		(0x01<<1)	//same as above for rows
#define JDB_TABLE_ENFORCE_COL_TYPE	(0x01<<2)	//enforce column types, no cell can break the rule!
#define JDB_TABLE_LOAD_DATA		(0x01<<3)	//load data when loading table

/*				type-flags*/
//#define JDB_TYPEDEF_VDATA	(0x01<<0)

/*				index types					*/
#define JDB_INDEX_CELL_VDATA	JDB_ITYPE_INDEX1	//exact vdata match index, mandatory when flag is set

/*				cell search flags				*/
#define JDB_FIND_EXACT		(0x0001<<0)

#ifdef __cplusplus
extern "C" {
#endif

/*
	basic
*/
	uint32_t jdb_ver();
	int jdb_fstat(wchar_t * filename, wchar_t * key, uint16_t mode);
	int jdb_init(struct jdb_handle *h);
	int jdb_open(struct jdb_handle *h, wchar_t * filename, wchar_t * key,
		     uint16_t flags);
	int jdb_open2(struct jdb_handle *h, int use_default_configuration);
	int jdb_sync(struct jdb_handle *h);
	int jdb_close(struct jdb_handle *h);

/*
	table create/remove/open/close
*/
	int jdb_create_table(struct jdb_handle *h, wchar_t * name,
			     uint32_t nrows, uint32_t ncols, uchar flags,
			     uint16_t indexes);
	int jdb_rm_table(struct jdb_handle *h, wchar_t * name);
	int jdb_open_table(struct jdb_handle *h, wchar_t * name, uchar flags);
	int jdb_close_table(struct jdb_handle *h, wchar_t * name);
	int jdb_list_tables(struct jdb_handle *h, jcslist_entry ** list,
				jdb_bid_t * n);
	int jdb_sync_table(struct jdb_handle *h, wchar_t * name);
	int jdb_sync(struct jdb_handle* h);
	int jdb_find_table(struct jdb_handle *h, wchar_t * name);
	int jdb_sync_tables(struct jdb_handle* h);

/*
	table type
*/
	int jdb_add_typedef(struct jdb_handle *h,
			    wchar_t* table_name, jdb_data_t type_id,
			    jdb_data_t base, uint32_t len, uchar flags);
	int jdb_rm_typedef(struct jdb_handle *h,
			   wchar_t* table_name, uchar type_id);
	int jdb_typedef_flags(	struct jdb_handle* h, wchar_t* table_name,
				jdb_data_t type_id, uchar* flags);
	jdb_data_t jdb_find_col_type(struct jdb_handle *h,
				wchar_t* table_name, uint32_t col);
	int jdb_assign_col_type(struct jdb_handle *h,
				wchar_t* table_name, uchar type_id,
				uint32_t col);
	int jdb_rm_col_type(struct jdb_handle *h,
			    wchar_t* table_name, uint32_t col);
	int jdb_find_type_base(	struct jdb_handle* h, wchar_t* table_name,
				jdb_data_t type_id, jdb_data_t* base);

/*
	table cell
*/
	int jdb_create_cell(struct jdb_handle *h, wchar_t * table,
			    uint32_t col, uint32_t row,
			    uchar * data, uint32_t datalen,
			    jdb_data_t data_type);
	int jdb_load_cell(struct jdb_handle *h, wchar_t * table,
			uint32_t col, uint32_t row, uchar** data,
			uint32_t* datalen, uchar* data_type);
	int jdb_rm_cell(struct jdb_handle* h, wchar_t * table,
			uint32_t col, uint32_t row);
	int jdb_update_cell(struct jdb_handle *h, wchar_t * table,
			    uint32_t col, uint32_t row,
			    uchar * data, uint32_t datalen);//you cannot change
			    					//data_type
			    					//by this call!
	int jdb_find_cell(struct jdb_handle* h, wchar_t * table,
			uchar* data, uint32_t datalen, uchar data_type,
			uint16_t flags, uint32_t* col, uint32_t* row,
			uint32_t* n);
	int jdb_list_cells(struct jdb_handle* h, wchar_t* table_name,
			uint32_t col, uint32_t row,
			uint32_t** li_col, uint32_t** li_row, uint32_t* n);

/*
	table col-row acts
*/
	
	int jdb_rm_col(struct jdb_handle* h, wchar_t* table, uint32_t col,
			int shift);
	int jdb_insert_col(struct jdb_handle* h, wchar_t* table, uint32_t col,
			int shift);
	int jdb_rm_row(struct jdb_handle* h, wchar_t* table, uint32_t row,
			int shift);
	int jdb_insert_row(struct jdb_handle* h, wchar_t* table, uint32_t row,
			int shift);
	int jdb_swap_col(struct jdb_handle* h, wchar_t* table, uint32_t col1,
			uint32_t col2);
	int jdb_swap_row(struct jdb_handle* h, wchar_t* table, uint32_t row1,
			uint32_t row2);
/*
	filters, lists
*/
	int jdb_init_filter(	struct jdb_filter* filter, size_t n,
				uint32_t col_start,
				uint32_t col_end, uint32_t row_start,
				uint32_t row_end,
				uint32_t sort_col, void* sort_fn);
	int jdb_add_filter(	struct jdb_filter* filter, uchar type,
				uchar not, uint32_t col,
				uchar logic, uchar match, jdb_data_t data_type,
				uint32_t str_len, uchar* str);
	int jdb_list(	struct jdb_handle* h, struct jdb_table* table,
 	    		struct jdb_filter* filter, struct jdb_rowset* rowset);
 	    		
/*
	info
*/
	size_t jdb_max_table_name_size(struct jdb_handle* h);
	
/*
	recovery
*/	
	int jdb_recover_journal(wchar_t* filename, wchar_t* key);
	int jdb_recover_db(wchar_t* filename, wchar_t* key);
	
#ifdef __cplusplus
}
#endif

#endif
