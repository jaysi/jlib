#include "jss.h"
#include "jer.h"
#include "jhash.h"
#include "jpack.h"
#include "aes.h"
#include "sha256.h"
#include "jcs.h"
#include "debug.h"
#include "jtable.h"
#include "hardio.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#define _wdeb1 _wdeb		//(f, a...)
#define _wdeb2 _wdeb
#define _wdeb_jtx	_wdeb
#define _wdeb_look	_wdeb

#define _JT_CR	'\n'
#define _JT_LF	'\r'

struct jt_cell *jt_find_merge_entry(struct jt_table *table,
				    struct jt_cell *cell)
{
	struct jt_cell *entry, *cur;

	switch (cell->hdr.merge_dir) {
	case JTC_MERGE_U:
	case JTC_MERGE_D:
	case JTC_MERGE_L:
	case JTC_MERGE_R:
		break;
	default:
		return cell;
	}

	cur = cell;

	while (cur->hdr.merge_dir) {
		for (entry = table->first_cell; entry; entry = entry->next) {

			if ((cur->hdr.merge_dir == JTC_MERGE_U) &&
			    (entry->hdr.row == cur->hdr.row - 1) &&
			    (entry->hdr.col == cur->hdr.col)) {
				cur = entry;
				break;
			}

			if ((cur->hdr.merge_dir == JTC_MERGE_D) &&
			    (entry->hdr.row == cur->hdr.row + 1) &&
			    (entry->hdr.col == cur->hdr.col)) {
				cur = entry;
				break;
			}

			if ((cur->hdr.merge_dir == JTC_MERGE_L) &&
			    (entry->hdr.row == cur->hdr.row) &&
			    (entry->hdr.col == cur->hdr.col - 1)) {
				cur = entry;
				break;
			}

			if ((cur->hdr.merge_dir == JTC_MERGE_R) &&
			    (entry->hdr.row == cur->hdr.row) &&
			    (entry->hdr.col == cur->hdr.col + 1)) {
				cur = entry;
				break;
			}
		}
	}

	return cur;
}

static inline int _jt_is_digit(uchar c)
{
	uchar i;

	for (i = '0'; i <= '9'; i++)
		if (c == i)
			return 1;

	return 0;
}

static inline uchar _jt_detect_str_type(uchar * buf, uint32_t bufsize)
{

	if (isjcs(buf, bufsize))
		return JTC_TYPE_JCS;
	if (wtojcs_len((wchar_t *) buf, bufsize) != ((size_t) - 1))
		return JTC_TYPE_W;
	if (strlen((char *)buf) == bufsize - 1)
		return JTC_TYPE_CHAR;
	return JTC_TYPE_RAW;

}

uchar jt_detect_type(uchar * buf, uint32_t bufsize, uchar dotsign,
		     uchar formulasign)
{
	int has_alpha = 0;
	int has_dot = 0;
	uint32_t i;

	_wdeb_jtx(L"bufsize = %u", bufsize);

	if (buf[0] == formulasign)
		return JTC_TYPE_FORMULA;

	for (i = 0; i < bufsize; i++) {
		if (!isdigit(buf[i])) {
			if (buf[i] == dotsign) {
				_wdeb_jtx(L"has alpha");
				has_dot = 1;
			} else {
				_wdeb_jtx(L"has alpha");
				has_alpha = 1;
			}
		}

		if (has_alpha)
			return _jt_detect_str_type(buf, bufsize);
	}

	if (!has_dot)
		return JTC_TYPE_INT;
	else
		return JTC_TYPE_FLOAT;

}

struct jt_cell *jt_create_cell(uchar * data, uint32_t datasize,
			       uchar * comments, uint32_t clen, uchar * format,
			       uint32_t flen, uchar type, uchar merge_dir,
			       uint32_t row, uint32_t col)
{

	struct jt_cell *cell;

	cell = (struct jt_cell *)malloc(sizeof(struct jt_cell));
	if (!cell)
		return NULL;

	cell->comments = NULL;
	cell->data = NULL;
	cell->fdata = NULL;

	cell->hdr.clen = clen;
	cell->hdr.datalen = datasize;
	cell->hdr.fdatalen = flen;

	cell->hdr.row = row;
	cell->hdr.col = col;

	if (clen) {
		cell->comments = (uchar *) malloc(clen + 1);
		memcpy(cell->comments, comments, clen);
		cell->comments[clen] = '\0';
	}

	if (datasize) {
		cell->data = (uchar *) malloc(datasize + 1);
		memcpy(cell->data, data, datasize);
		cell->data[datasize] = '\0';
	}

	if (flen) {
		cell->fdata = (uchar *) malloc(flen + 1);
		memcpy(cell->fdata, format, flen);
		cell->fdata[flen] = '\0';
	}

	if (type == JTC_TYPE_EMPTY && datasize) {
		_wdeb_jtx(L"auto-detect enabled");
		cell->hdr.type = jt_detect_type(data, datasize, JTC_DOT_SIGN,
						JTC_FORMULA_SIGN);
		_wdeb_jtx(L"detected type: 0x%x", cell->hdr.type);
	} else {
		cell->hdr.type = type;
	}
	cell->hdr.merge_dir = merge_dir;

	return cell;
}

int jt_create_table(struct jt_table *table, uint32_t nrows, uint32_t ncols,
		    uint32_t id)
{

	if (!nrows || !ncols)
		return -JE_FORBID;

	table->hdr.id = id;
	table->hdr.nrows = nrows;
	table->hdr.ncols = ncols;
	table->hdr.last_cell_id = 0UL;
	table->hdr.ncells = 0UL;

	table->first_cell = NULL;

	return 0;
}

wchar_t *jt_get_cell_comments(struct jt_cell * cell)
{
	wchar_t *comments;
	if (!cell->hdr.clen || !cell->comments)
		return NULL;

	comments = (wchar_t *) malloc(CWBYTES(cell->comments));
	jcstow(cell->comments, comments, cell->hdr.clen);

	return comments;
}

int jt_set_cell_comments(struct jt_cell *cell, wchar_t * comments)
{

	size_t jcslen;

	if (cell->comments || cell->hdr.clen) {
		free(cell->comments);
	}

	jcslen = wtojcs_len(comments, 0);

	cell->comments = (uchar *) malloc(jcslen + 1);

	if ((wtojcs(comments, cell->comments, jcslen + 1)) == ((size_t) - 1)) {
		free(cell->comments);
		return -JE_FAIL;
	}

	cell->hdr.clen = jcslen + 1;

	return 0;

}

int jt_get_cell(struct jt_table *table, uint32_t row, uint32_t col,
		struct jt_cell **cell)
{

	if (row > table->hdr.nrows - 1 || col > table->hdr.ncols - 1)
		return -JE_LIMIT;

	for (*cell = table->first_cell; *cell; *cell = (*cell)->next) {
		if ((row == (*cell)->hdr.row) && (col == (*cell)->hdr.col))
			return 0;
	}

	return -JE_NOTFOUND;

}

int jt_add_cell(struct jt_table *table, struct jt_cell *cell)
{

	struct jt_cell *cell_entry;

	if (cell->hdr.row > table->hdr.nrows - 1
	    || cell->hdr.col > table->hdr.ncols - 1)
		return -JE_LIMIT;

	for (cell_entry = table->first_cell; cell_entry;
	     cell_entry = cell_entry->next) {
		if ((cell->hdr.row == cell_entry->hdr.row)
		    && (cell->hdr.col == cell_entry->hdr.col))
			return -JE_EXISTS;
	}

	cell->next = NULL;
	cell->hdr.id = table->hdr.last_cell_id++;

	if (!table->first_cell) {
		table->first_cell = cell;
		table->last_cell = cell;
	} else {
		table->last_cell->next = cell;
		table->last_cell = table->last_cell->next;
	}

	_wdeb2(L"data (rec): %s", cell->data);

	table->hdr.ncells++;

	return 0;
}

int jt_update_cell(struct jt_table *table, uint32_t row, uint32_t col,
		   uchar * data, uint32_t datasize,
		   uchar * comments, uint32_t clen, uchar * fdata,
		   uint32_t fdatalen, uchar merge_dir, uint32_t new_row,
		   uint32_t new_col, uchar jt_upd_flags)
{

	int ret;
	struct jt_cell *cell;
	uchar *buf;
	ret = jt_get_cell(table, row, col, &cell);
	if (ret < 0)
		return ret;

	if (jt_upd_flags & JTC_SEARCH_DATA) {
		if (datasize) {
			if (cell->hdr.datalen) {
				if (cell->hdr.datalen - 1 == datasize) {
					memcpy(cell->data, data, datasize);
					cell->data[datasize] = '\0';
				} else {
					buf = (uchar *) malloc(datasize + 1);
					if (!buf)
						return -JE_MALOC;
					memcpy(buf, data, datasize);
					buf[datasize] = '\0';
					cell->hdr.datalen = datasize;
					free(cell->data);
					cell->data = buf;
				}
			} else {
				cell->data = (uchar *) malloc(datasize + 1);
				if (!cell->data)
					return -JE_MALOC;
				cell->hdr.datalen = datasize;
				memcpy(cell->data, data, datasize);
				cell->data[datasize] = '\0';
			}
		} else {
			if (cell->hdr.datalen) {
				free(cell->data);
				cell->hdr.datalen = 0;
			}
		}
	}

	if (jt_upd_flags & JTC_SEARCH_COMMENTS) {
		if (clen) {
			if (cell->hdr.clen) {
				if (cell->hdr.clen - 1 == clen) {
					memcpy(cell->comments, comments, clen);
					cell->comments[clen] = '\0';
				} else {
					buf = (uchar *) malloc(clen + 1);
					if (!buf)
						return -JE_MALOC;
					memcpy(buf, comments, clen);
					buf[clen] = '\0';
					cell->hdr.clen = clen;
					free(cell->comments);
					cell->comments = buf;
				}
			} else {
				cell->comments = (uchar *) malloc(clen + 1);
				if (!cell->comments)
					return -JE_MALOC;
				cell->hdr.clen = clen;
				memcpy(cell->comments, comments, clen);
				cell->comments[clen] = '\0';
			}
		} else {
			if (cell->hdr.clen) {
				free(cell->comments);
				cell->hdr.clen = 0;
			}
		}
	}

	if (jt_upd_flags & JTC_SEARCH_FORMAT) {
		if (fdatalen) {
			if (cell->hdr.fdatalen) {
				if (cell->hdr.fdatalen - 1 == fdatalen) {
					memcpy(cell->fdata, fdata, fdatalen);
					cell->fdata[fdatalen] = '\0';
				} else {
					buf = (uchar *) malloc(fdatalen + 1);
					if (!buf)
						return -JE_MALOC;
					memcpy(buf, fdata, fdatalen);
					buf[fdatalen] = '\0';
					cell->hdr.fdatalen = fdatalen;
					free(cell->fdata);
					cell->fdata = buf;
				}
			} else {
				cell->fdata = (uchar *) malloc(fdatalen + 1);
				if (!cell->fdata)
					return -JE_MALOC;
				cell->hdr.fdatalen = fdatalen;
				memcpy(cell->fdata, fdata, fdatalen);
				cell->fdata[fdatalen] = '\0';
			}
		} else {
			if (cell->hdr.fdatalen) {
				free(cell->fdata);
				cell->hdr.fdatalen = 0;
			}
		}
	}

	if (jt_upd_flags & JTC_SEARCH_MERGE) {
		cell->hdr.merge_dir = merge_dir;
	}

	if (jt_upd_flags & JTC_SEARCH_LOCATE) {
		jt_rm_cell(table, row, col);
		cell->hdr.row = new_row;
		cell->hdr.col = new_col;
	}

	return 0;

}

static inline void _jt_free_cell(struct jt_cell *cell)
{
	if (cell->hdr.datalen && cell->data)
		free(cell->data);
	if (cell->hdr.clen && cell->comments)
		free(cell->comments);
	if (cell->hdr.fdatalen && cell->fdata)
		free(cell->fdata);
	free(cell);
}

static inline struct jt_cell *_jt_rm_cell_direct(struct jt_table *table,
						 struct jt_cell *del,
						 struct jt_cell *prev)
{

	struct jt_cell *next;

	if (del == table->first_cell) {
		table->first_cell = table->first_cell->next;
	} else if (del == table->last_cell) {
		table->last_cell = prev;
		table->last_cell->next = NULL;
	} else {
		prev->next = del->next;
	}

	if (table->hdr.last_cell_id == del->hdr.id - 1) {
		table->hdr.last_cell_id--;
	}

	next = del->next;

	_jt_free_cell(del);

	table->hdr.ncells--;

	return next;

}

int jt_rm_cell(struct jt_table *table, uint32_t row, uint32_t col)
{
	struct jt_cell *prev, *del;

	if (row > table->hdr.nrows - 1 || col > table->hdr.ncols - 1)
		return -JE_LIMIT;

	del = table->first_cell;
	prev = del;

	while (del) {

		if ((del->hdr.row == row) && (del->hdr.col == col)) {
			_jt_rm_cell_direct(table, del, prev);
			return 0;
		}

		prev = del;
		del = del->next;
	}

	return -JE_NOTFOUND;
}

int jt_rm_row(struct jt_table *table, uint32_t row, int remap)
{

	struct jt_cell *prev, *del;

	if (row > table->hdr.nrows - 1)
		return -JE_INV;

	del = table->first_cell;
	prev = del;

	while (del) {

		if (del->hdr.row == row) {
			del = _jt_rm_cell_direct(table, del, prev);
		} else {
			prev = del;
			del = del->next;

		}
	}

	//re-arrange cell rows

	if (remap) {

		for (del = table->first_cell; del; del = del->next) {
			if (del->hdr.row > row)
				del->hdr.row--;
		}

	}

	return 0;

}

int jt_rm_col(struct jt_table *table, uint32_t col, int remap)
{

	struct jt_cell *prev, *del;

	if (col > table->hdr.ncols - 1)
		return -JE_INV;

	del = table->first_cell;
	prev = del;

	while (del) {

		if (del->hdr.col == col) {
			del = _jt_rm_cell_direct(table, del, prev);
		} else {
			prev = del;
			del = del->next;

		}
	}

	//re-arrange cell rows

	if (remap) {

		for (del = table->first_cell; del; del = del->next) {
			if (del->hdr.col > col)
				del->hdr.col--;
		}

	}

	return 0;

}

void jt_insert_row(struct jt_table *table, uint32_t row)
{
	struct jt_cell *cell;
	table->hdr.nrows++;

	for (cell = table->first_cell; cell; cell = cell->next) {
		if (cell->hdr.row >= row)
			cell->hdr.row++;
	}

}

void jt_insert_col(struct jt_table *table, uint32_t col)
{
	struct jt_cell *cell;
	table->hdr.ncols++;

	for (cell = table->first_cell; cell; cell = cell->next) {
		if (cell->hdr.col >= col)
			cell->hdr.col++;
	}
}

int jt_table_set_size(struct jt_table *table, uint32_t nrows, uint32_t ncols)
{
	int passes = 0;
	uint32_t i;

	if (!nrows || !ncols)
		return -JE_FORBID;

	if (table->hdr.nrows < nrows) {
		passes++;
		table->hdr.nrows = nrows;
	}

	if (table->hdr.ncols < ncols) {
		passes++;
		table->hdr.ncols = ncols;
	}

	if (passes == 2)
		return 0;

	if (table->hdr.nrows > nrows) {
		for (i = nrows + 1; i <= table->hdr.nrows; i++) {
			jt_rm_row(table, i, 1);
		}

		table->hdr.nrows = nrows;

		if (passes)
			return 0;
		passes++;
	}

	if (table->hdr.ncols > ncols) {
		for (i = ncols + 1; i <= table->hdr.ncols; i++) {
			jt_rm_col(table, i, 1);
		}

		table->hdr.ncols = ncols;
	}

	return 0;
}

int jt_table_to_buf(uchar ** buf, uint32_t * bufsize, struct jt_table *table)
{

	struct jt_cell *cell;
	uint32_t pos;
	uint32_t hsize;
	uint32_t prevoff;

	hsize = sizeof(struct jt_table_hdr) +
	    sizeof(struct jt_cell_hdr) * table->hdr.ncells;

	*bufsize = 0UL;
	pos = 0UL;
	prevoff = hsize;
	for (cell = table->first_cell; cell; cell = cell->next) {
		*bufsize +=
		    cell->hdr.datalen + cell->hdr.clen + cell->hdr.fdatalen;
		cell->hdr.off = prevoff;
		prevoff +=
		    cell->hdr.datalen + cell->hdr.clen + cell->hdr.fdatalen;
	}

	*bufsize += hsize;

	*buf = (uchar *) malloc(*bufsize);

	memcpy((char *)*buf, (char *)&table->hdr, sizeof(struct jt_table_hdr));

	pos = sizeof(struct jt_table_hdr);

	for (cell = table->first_cell; cell; cell = cell->next) {

		_wdeb2(L"fdata(rec): %s", cell->fdata);

		memcpy(((char *)*buf) + pos, (char *)&cell->hdr,
		       sizeof(struct jt_cell_hdr));

		if (cell->hdr.datalen)
			memcpy(((char *)*buf) + cell->hdr.off,
			       (char *)cell->data, cell->hdr.datalen);

		if (cell->hdr.clen)
			memcpy(((char *)*buf) + cell->hdr.off +
			       cell->hdr.datalen, (char *)cell->comments,
			       cell->hdr.clen);

		if (cell->hdr.fdatalen) {
			memcpy(((char *)*buf) + cell->hdr.off +
			       cell->hdr.datalen + cell->hdr.clen,
			       (char *)cell->fdata, cell->hdr.fdatalen);
			_wdeb2(L"fdata(buf): %s, fdata(rec): %s",
			       buf + cell->hdr.off + cell->hdr.datalen +
			       cell->hdr.clen, cell->fdata);
		}

		pos += sizeof(struct jt_cell_hdr);
	}

	return 0;
}

void jt_free_table(struct jt_table *table)
{

	struct jt_cell *del;

	while (table->first_cell) {
		del = table->first_cell;
		table->first_cell = table->first_cell->next;
		_jt_free_cell(del);
	}
}

int jt_buf_to_table(struct jt_table *table, uchar * buf, uint32_t bufsize)
{

	struct jt_cell_hdr hdr;
	struct jt_cell *cell;
	uint32_t i, pos;
	int ret;

	memcpy((char *)&table->hdr, buf, sizeof(struct jt_table_hdr));

	_wdeb1(L"ncells = %u", table->hdr.ncells);

	pos = sizeof(struct jt_table_hdr);
	for (i = 0; i < table->hdr.ncells; i++) {

		_wdeb1(L"i = %u, ncells = %u", i, table->hdr.ncells);
		/*
		   if((cell = jt_create_cell()) == NULL){       
		   jt_free_table(table);
		   return -JE_MALOC;
		   }            
		 */
		memcpy((char *)&hdr, (char *)buf + pos,
		       sizeof(struct jt_cell_hdr));

		_wdeb1(L"passed 1, adding cell %u @ R:%u,C:%u",
		       hdr.id, hdr.row, hdr.col);

		cell = jt_create_cell(buf + hdr.off, hdr.datalen,
				      buf + hdr.off + hdr.datalen, hdr.clen,
				      buf + hdr.off + hdr.datalen + hdr.clen,
				      hdr.fdatalen,
				      hdr.type, hdr.merge_dir,
				      hdr.row, hdr.col);

		if (!cell) {
			jt_free_table(table);
			return -JE_MALOC;
		}

		/*
		   if(cell->hdr.datalen){
		   cell->data = (uchar*)malloc(cell->hdr.datalen);
		   if(!cell->data){
		   jt_free_table(table);
		   return -JE_MALOC;            
		   }
		   memcpy(      (char*)cell->data, (char*)buf + cell->hdr.off,
		   cell->hdr.datalen);
		   _wdeb1(L"passed 2");
		   }

		   if(cell->hdr.clen){
		   cell->comments = (uchar*)malloc(cell->hdr.clen);
		   if(!cell->comments){
		   jt_free_table(table);
		   return -JE_MALOC;
		   }
		   memcpy(      (char*)cell->comments,
		   (char*)buf + cell->hdr.off + cell->hdr.datalen,
		   cell->hdr.clen);
		   _wdeb1(L"passed 3");
		   }

		   if(cell->hdr.fdatalen){
		   cell->fdata = (uchar*)malloc(cell->hdr.fdatalen);
		   if(!cell->fdata){
		   jt_free_table(table);
		   return -JE_MALOC;
		   }
		   //_wdeb2(L"fdata: %s", buf+cell->hdr.off+cell->hdr.datalen+cell->hdr.clen);
		   memcpy(      (char*)cell->fdata,
		   (char*)buf + cell->hdr.off + cell->hdr.datalen + cell->hdr.clen,
		   cell->hdr.fdatalen);
		   _wdeb1(L"passed 4");
		   }            
		 */
		if ((ret = jt_add_cell(table, cell)) < 0) {
			jt_free_table(table);
			return ret;
		}
		//this reduces ++ from add_cell call
		table->hdr.ncells--;

		pos += sizeof(struct jt_cell_hdr);
	}

	return 0;
}

struct jt_cell *jt_get_row(struct jt_table *table, uint32_t row, uint32_t * n)
{
	struct jt_cell *cell_first = NULL, *cell_last, *entry;
	uint32_t i;
	*n = 0UL;
	for (i = 0; i < table->hdr.ncols; i++) {
		for (entry = table->first_cell; entry; entry = entry->next) {
			if ((entry->hdr.row == row) && (entry->hdr.col == i)) {
				entry->next_array = NULL;
				if (!cell_first) {
					cell_first = entry;
					cell_last = entry;
				} else {
					cell_last->next_array = entry;
					cell_last = cell_last->next_array;
				}
				(*n)++;
			}
		}
	}

	return cell_first;

}

struct jt_cell *jt_get_col(struct jt_table *table, uint32_t col, uint32_t * n)
{
	struct jt_cell *cell_first = NULL, *cell_last, *entry;
	uint32_t i;
	*n = 0UL;
	for (i = 0; i < table->hdr.nrows; i++) {
		for (entry = table->first_cell; entry; entry = entry->next) {
			if ((entry->hdr.col == col) && (entry->hdr.row == i)) {
				entry->next_array = NULL;
				if (!cell_first) {
					cell_first = entry;
					cell_last = entry;
				} else {
					cell_last->next_array = entry;
					cell_last = cell_last->next_array;
				}
				(*n)++;
			}
		}
	}

	return cell_first;

}

struct jt_cell *jt_enum_cells(struct jt_table *table, uchar * data,
			      uint32_t datalen, uchar type, uint32_t * n)
{
	struct jt_cell *cell_first = NULL, *cell_last, *entry;
	uint32_t i;
	*n = 0UL;
	for (i = 0; i < table->hdr.ncols; i++) {
		for (entry = table->first_cell; entry; entry = entry->next) {

			if ((datalen == entry->hdr.datalen) &&
			    ((type == JTC_TYPE_UNK) || type == entry->hdr.type)
			    && !memcmp(entry->data, data, datalen)

			    ) {

				entry->next_array = NULL;
				if (!cell_first) {
					cell_first = entry;
					cell_last = entry;
				} else {
					cell_last->next_array = entry;
					cell_last = cell_last->next_array;
				}
				(*n)++;
			}
		}
	}

	return cell_first;

}

void jt_sort_table(struct jt_table *table)
{

	struct jt_cell *cell_first = NULL, *cell_last, *entry;

	uint32_t i, j;
	for (i = 0; i < table->hdr.nrows; i++) {
		for (j = 0; j < table->hdr.ncols; j++) {
			for (entry = table->first_cell; entry;
			     entry = entry->next) {
				if ((entry->hdr.col == j)
				    && (entry->hdr.row == i)) {
					entry->next_array = NULL;
					if (!cell_first) {
						cell_first = entry;
						cell_last = entry;
					} else {
						cell_last->next_array = entry;
						cell_last =
						    cell_last->next_array;
					}
				}
			}
		}
	}

	for (entry = cell_first; entry; entry = entry->next_array) {
		entry->next = entry->next_array;
	}

	table->first_cell = cell_first;
	table->last_cell = cell_last;
}

static inline int _jt_is_match(struct jt_cell *cell,
			       uchar * data, uint32_t datalen,
			       uchar * comments, uint32_t clen,
			       uchar * fdata, uint32_t fdatalen,
			       uchar merge_dir, uchar JTC_SEARCH_flags)
{
	//return 0 on failure!

	_wdeb_look(L"searching for match...");

	if (JTC_SEARCH_flags & JTC_SEARCH_DATA) {
		_wdeb_look(L"data");
		if (cell->hdr.datalen != datalen) {
			_wdeb_look(L"length mismatch");
			return 0;
		}

		if (datalen) {
			if (memcmp(cell->data, data, datalen)) {
				_wdeb_look(L"comparision failed");
				return 0;
			}
		}
	}

	if (JTC_SEARCH_flags & JTC_SEARCH_COMMENTS) {
		if (cell->hdr.clen != clen) {
			return 0;
		}

		if (clen) {
			if (memcmp(cell->comments, comments, clen))
				return 0;
		}
	}

	if (JTC_SEARCH_flags & JTC_SEARCH_FORMAT) {
		if (cell->hdr.fdatalen != fdatalen) {
			return 0;
		}

		if (fdatalen) {
			if (memcmp(cell->fdata, fdata, fdatalen))
				return 0;
		}
	}

	if (JTC_SEARCH_flags & JTC_SEARCH_MERGE) {
		if (cell->hdr.merge_dir != merge_dir)
			return 0;
	}

	_wdeb_look(L"found a match");

	return 1;
}

uint32_t *jt_list_rows(struct jt_table * table,
		       uchar * data, uint32_t datalen,
		       uchar * comments, uint32_t clen,
		       uchar * fdata, uint32_t fdatalen,
		       uchar merge_dir, uchar JTC_SEARCH_flags, uint32_t * n)
{

	uint32_t row;
	struct jt_cell *cell, *first_cell, *last_cell;
	uint32_t *row_list;

	jt_sort_table(table);

	first_cell = NULL;
	cell = table->first_cell;
	*n = 0UL;
	for (row = 0; row <= table->last_cell->hdr.row; row++) {
		while (cell && cell->hdr.row == row) {
			_wdeb_look(L"row: %u, cell->row = %u, col = %u", row,
				   cell->hdr.row, cell->hdr.col);
			cell = cell->next;
		}

		if (!cell)
			break;

		if (_jt_is_match(cell, data, datalen, comments, clen,
				 fdata, fdatalen, merge_dir,
				 JTC_SEARCH_flags)) {

			cell->next_array = NULL;

			if (!first_cell) {
				_wdeb_look(L"adding as first");
				first_cell = cell;
				last_cell = cell;
				(*n)++;
				continue;
			} else {
				_wdeb_look(L"adding as last");
				last_cell->next_array = cell;
				last_cell = last_cell->next_array;
				(*n)++;
				continue;
			}
		}
	}

	_wdeb_look(L"nrows = %u", *n);

	if (!first_cell)
		return NULL;

	row_list = (uint32_t *) malloc((*n) * sizeof(uint32_t));
	if (!row_list) {
		*n = 0UL;
		return NULL;
	}

	row = 0UL;
	for (cell = first_cell; cell; cell = cell->next_array) {
		row_list[row++] = cell->hdr.row;
	}

	return row_list;
}

void jt_sort_table_by_col(struct jt_table *table)
{

	struct jt_cell *cell_first = NULL, *cell_last, *entry;

	uint32_t i, j;
	for (i = 0; i < table->hdr.ncols; i++) {
		for (j = 0; j < table->hdr.nrows; j++) {
			for (entry = table->first_cell; entry;
			     entry = entry->next) {
				if ((entry->hdr.col == i)
				    && (entry->hdr.row == j)) {
					entry->next_array = NULL;
					if (!cell_first) {
						cell_first = entry;
						cell_last = entry;
					} else {
						cell_last->next_array = entry;
						cell_last =
						    cell_last->next_array;
					}
				}
			}
		}
	}

	for (entry = cell_first; entry; entry = entry->next_array) {
		entry->next = entry->next_array;
	}

	table->first_cell = cell_first;
	table->last_cell = cell_last;
}

static inline int _jt_detect_eol_type(uchar * buf, size_t bufsize)
{
	size_t pos;
	for (pos = 0; pos <= bufsize; pos++) {
		if (buf[pos] == _JT_CR) {
			_wdeb_jtx(L"got cr @ %u", pos);
			if (pos < bufsize) {
				if (buf[pos + 1] == _JT_LF) {
					_wdeb_jtx(L"CRLF type detected");
					return JT_EOL_CRLF;
				} else {
					_wdeb_jtx(L"CR type detected");
					return JT_EOL_CR;
				}
			} else {
				_wdeb_jtx(L"CR type detected");
				return JT_EOL_CR;
			}
		}
	}
	return -JE_INV;
}

static inline int _jt_is_eol(uchar * buf, size_t pos, size_t bufsize,
			     int eol_type)
{
	if (buf[pos] == _JT_CR) {
		if (eol_type == JT_EOL_CRLF) {
			if (pos < bufsize) {
				if (buf[pos + 1] == _JT_LF) {
					_wdeb_jtx(L"found CRLF @ %u", pos);
					return 0;
				}
			} else {
				_wdeb_jtx(L"invalid entry @ %u", pos);
				return -JE_INV;
			}
		} else {
			_wdeb_jtx(L"found CR @ %u", pos);
			return 0;
		}
	}
	//_wdeb_jtx(L"not found [pos = %u]", pos);
	return -JE_NOTFOUND;

}

static inline size_t _jt_detect_cols(uchar * line, size_t len, char delim)
{

	size_t cnt;
	size_t ret = 0;
	for (cnt = 0; cnt < len; cnt++) {
		if (line[cnt] == delim) {
			ret++;
		}
	}

	return ++ret;

}

static inline size_t _jt_detect_line(uchar * buf, size_t line_start,
				     size_t size, int eol_type)
{
	size_t ret = line_start;
	while (ret < size) {
		if (!_jt_is_eol(buf, ret, size, eol_type))
			return ret;
		ret++;
	}

	return ret;
}

static inline size_t _jt_detect_field(uchar * buf, size_t field_start,
				      size_t bufsize, uchar delim, int eol_type)
{
	size_t ret = field_start;

	while (buf[ret] != delim) {
		ret++;
		if (ret >= bufsize || !_jt_is_eol(buf, ret, bufsize, eol_type)) {
			return ret;
		}
	}

	return ret;

}

int jt_text_to_table(uchar * buf, size_t bufsize, struct jt_table *table,
		     uint32_t table_id, uchar delim)
{

	off_t line_start, line_end;
	size_t field_start, field_end;
	struct jt_cell *cell;
	int eol_type;
	int table_init;
	int ret;
	uint32_t col;
	uint32_t ncols;

	_wdeb_jtx(L"detecting end-of-line type...");
	eol_type = _jt_detect_eol_type(buf, bufsize);
	if (eol_type < 0)
		return eol_type;

	line_start = 0UL;
	line_end = 0UL;
	table_init = 0;
	while (line_end < bufsize) {
		line_end = _jt_detect_line(buf, line_start, bufsize, eol_type);

		//len = line_end - line_start;

		_wdeb_jtx(L"line start = %u, line_end = %u", line_start,
			  line_end);

		if (!table_init) {
			ncols = _jt_detect_cols(buf, line_end - line_start,
						delim);
			if ((table_init = jt_create_table(table, 1, ncols,
							  table_id)) < 0)
				return table_init;
			table_init = 1;
		}

		if (line_end == line_start)
			break;

		field_start = line_start;
		for (col = 0; col < ncols; col++) {

			field_end = _jt_detect_field(buf,
						     field_start,
						     bufsize, delim, eol_type);

/*			
			cell = jt_create_cell();
			if(!cell){
				jt_free_table(table);
				return -JE_MALOC;
			}
			
			cell->hdr.row = table->hdr.nrows-1;
			cell->hdr.col = col;
			cell->hdr.datalen = field_end - field_start;			
			
			if(cell->hdr.datalen){
				cell->data = (uchar*)malloc(cell->hdr.datalen);
				memcpy(cell->data, buf+field_start, cell->hdr.datalen);
			}
*/

			cell = jt_create_cell((uchar *) buf + field_start,
					      field_end - field_start,
					      NULL, 0,
					      NULL, 0,
					      JTC_TYPE_EMPTY, JTC_MERGE_NO,
					      table->hdr.nrows - 1, col);

			_wdeb_jtx
			    (L"row:%u, col: %u, fstart: %u, fend: %u, flen: %u, data(rough): %s",
			     table->hdr.nrows - 1, col, field_start, field_end,
			     cell->hdr.datalen, cell->data);

			if ((ret = jt_add_cell(table, cell)) < 0) {
				jt_free_table(table);
				return ret;
			}

			field_start = field_end + 1;

		}

		table->hdr.nrows++;

		line_start = line_end + 1;
	}

	return 0;

}

int jt_table_to_text(struct jt_table *table, uchar ** buf, size_t * bufsize,
		     uchar delim, int eol)
{

	uint32_t row, col;
	uint32_t pos;
	struct jt_cell *cell;

	switch (eol) {
	case JT_EOL_CR:
		(*bufsize) = table->hdr.nrows;
		break;
	case JT_EOL_CRLF:
		(*bufsize) = table->hdr.nrows * 2;
		break;
	default:
		return -JE_TYPE;
	}

	jt_sort_table(table);

	for (cell = table->first_cell; cell; cell = cell->next) {
		*bufsize += cell->hdr.datalen;
		(*bufsize)++;	//for delim
	}

	*buf = (uchar *) malloc(*bufsize);
	if (!(*buf))
		return -JE_MALOC;

	cell = table->first_cell;
	pos = 0;
	for (row = 0; row < table->hdr.nrows; row++) {
		for (col = 0; col < table->hdr.ncols; col++) {
			if (!jt_get_cell(table, row, col, &cell)) {
				memcpy((*buf) + pos, cell->data,
				       cell->hdr.datalen);
				pos += cell->hdr.datalen;
			}
			(*buf)[pos++] = delim;
		}

		pos--;
		(*buf)[pos++] = _JT_CR;

		if (eol == JT_EOL_CRLF)
			(*buf)[pos++] = _JT_LF;
	}

	return 0;
}

int jt_file_to_table(struct jt_table *table, uint32_t id, wchar_t * filename,
		     uchar delim)
{
	int fd;
	char *fname;
	uchar *buf;
	struct stat s;
	int ret;

	_wdeb1(L"adding #%u", id);

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

	ret = jt_text_to_table(buf, s.st_size, table, id, delim);

	free(buf);

	if (ret < 0) {
		return ret;
	}

	table->name =
	    (wchar_t *) malloc((wcslen(filename) + 1) * sizeof(wchar_t));
	wcscpy(table->name, filename);

	return 0;
}

int jt_table_to_file(wchar_t * filename, struct jt_table *table, uchar delim,
		     int eol)
{

	uint32_t bufsize;
	uchar *buf;
	int ret;
	int fd;
	char *fname;

	fname = (char *)malloc(wtojcs_len(filename, JSS_MAXFILENAME));
	if (!fname)
		return -JE_MALOC;

	wtojcs(filename, (uchar *) fname, JSS_MAXFILENAME);

	fd = open(fname, O_WRONLY | O_CREAT);

	free(fname);

	if (fd < 0)
		return -JE_OPEN;

	ret = jt_table_to_text(table, &buf, &bufsize, delim, eol);
	if (ret < 0) {
		close(fd);
		return ret;
	}

	if (hard_write(fd, (char *)buf, bufsize) < bufsize) {
		ret = -JE_WRITE;
	}

	free(buf);
	close(fd);

	return ret;
}
