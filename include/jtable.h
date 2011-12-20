#ifndef _JTABLE_H
#define _JTABLE_H

#include "jtypes.h"
#include "aes.h"
#include <wchar.h>

#define JTC_MERGE_NO	0x0
#define JTC_MERGE_U	0x1
#define JTC_MERGE_D	0x2
#define JTC_MERGE_L	0x3
#define JTC_MERGE_R	0x4

#define JTC_TYPE_EMPTY		0x00
#define JTC_TYPE_CHAR		0x01
#define JTC_TYPE_INT		0x02
#define JTC_TYPE_FLOAT		0x03
#define JTC_TYPE_JCS		0x04
#define JTC_TYPE_W		0x05	//unicode 16 bits
#define JTC_TYPE_RAW		0x06
#define JTC_TYPE_FORMULA	0x07
#define JTC_TYPE_UNK		0xff

#define JTC_SEARCH_DATA		(0x01<<0)
#define JTC_SEARCH_COMMENTS		(0x01<<1)
#define JTC_SEARCH_FORMAT		(0x01<<2)
#define JTC_SEARCH_MERGE		(0x01<<3)
#define JTC_SEARCH_LOCATE		(0x01<<4)

#define JTC_DOT_SIGN		'.'
#define JTC_FORMULA_SIGN	'='

#define JT_EOL_CR	1
#define JT_EOL_CRLF	2

struct jt_cell_hdr {
	uchar flags;
	uchar unused:3, unsign:1, merge_dir:4;
	uint32_t id;
	uint32_t row;
	uint32_t col;

	uchar type;		//data type
	uint32_t off;		//data offset in buffer
	uint32_t datalen;	//data len
	uint32_t clen;		//comments' len plus 1 for null termination
	uint32_t fdatalen;	//length of formatting data

} __attribute__ ((packed));

struct jt_cell_chunk_list {

	uint64_t bid;		//block id
	uint16_t b_off;		//offset inside the block
	uint16_t b_len;		//chunk len inside the block

	struct jt_cell_chunk_list *next;
};

struct jt_cell {

	struct jt_cell_hdr hdr;

	uchar *data;
	uchar *comments;	//jcs
	uchar *fdata;		//formatting data

	struct jt_cell *next;
	struct jt_cell *next_array;
};

struct jt_table_hdr {
	uint32_t id;
	uint32_t nrows;
	uint32_t ncols;
	uint32_t last_cell_id;
	uint32_t ncells;
};

struct jt_table {
	struct jt_table_hdr hdr;
	wchar_t *name;
	struct jt_cell *first_cell, *last_cell;
};

#ifdef __cplusplus
extern "C" {
#endif

	uchar jt_detect_type(uchar * buf, uint32_t bufsize, uchar dotsign,
			     uchar formulasign);
	int jt_add_cell(struct jt_table *table, struct jt_cell *cell);
	int jt_rm_cell(struct jt_table *table, uint32_t row, uint32_t col);
	int jt_rm_row(struct jt_table *table, uint32_t pos, int remap);
	int jt_rm_col(struct jt_table *table, uint32_t pos, int remap);
	void jt_insert_row(struct jt_table *table, uint32_t row);
	void jt_insert_col(struct jt_table *table, uint32_t col);
	int jt_create_table(struct jt_table *table, uint32_t nrows,
			    uint32_t ncols, uint32_t id);
	int jt_table_set_size(struct jt_table *table, uint32_t nrows,
			      uint32_t ncols);

	struct jt_cell *jt_create_cell(uchar * data, uint32_t datasize,
				       uchar * comments, uint32_t clen,
				       uchar * format, uint32_t flen,
				       uchar type, uchar merge_dir,
				       uint32_t row, uint32_t col);

	int jt_update_cell(struct jt_table *table, uint32_t row, uint32_t col,
			   uchar * data, uint32_t datasize,
			   uchar * comments, uint32_t clen, uchar * format,
			   uint32_t flen, uchar merge_dir, uint32_t new_row,
			   uint32_t new_col, uchar jt_upd_flags);

	int jt_set_cell_comments(struct jt_cell *cell, wchar_t * comments);
	int jt_get_cell(struct jt_table *table, uint32_t row, uint32_t col,
			struct jt_cell **cell);
	int jt_table_set_size(struct jt_table *table, uint32_t nrows,
			      uint32_t ncols);
	wchar_t *jt_get_cell_comments(struct jt_cell *cell);
	void jt_free_table(struct jt_table *table);

//use next_array pointer instead of next to use the output of these calls
	struct jt_cell *jt_get_row(struct jt_table *table, uint32_t row,
				   uint32_t * n);
	struct jt_cell *jt_get_col(struct jt_table *table, uint32_t col,
				   uint32_t * n);
	struct jt_cell *jt_enum_cells(struct jt_table *table, uchar * data,
				      uint32_t datalen, uchar type,
				      uint32_t * n);
	uint32_t *jt_list_rows(struct jt_table *table, uchar * data,
			       uint32_t datalen, uchar * comments,
			       uint32_t clen, uchar * fdata, uint32_t fdatalen,
			       uchar merge_dir, uchar jt_search_flags,
			       uint32_t * nrows);

//sort by row
	void jt_sort_table(struct jt_table *table);
	void jt_sort_table_by_col(struct jt_table *table);

//these are two calls made for jss
	int jt_table_to_buf(uchar ** buf, uint32_t * bufsize,
			    struct jt_table *table);
	int jt_buf_to_table(struct jt_table *table, uchar * buf,
			    uint32_t bufsize);

//table - text conversion
	int jt_text_to_table(uchar * buf, size_t bufsize,
			     struct jt_table *table, uint32_t table_id,
			     uchar delim);
	int jt_table_to_text(struct jt_table *table, uchar ** buf,
			     size_t * bufsize, uchar delim, int end_line_type);

	int jt_file_to_table(struct jt_table *table, uint32_t id,
			     wchar_t * filename, uchar delim);

	int jt_table_to_file(wchar_t * filename, struct jt_table *table,
			     uchar delim, int end_line_type);

#ifdef __cplusplus
}
#endif
#endif
