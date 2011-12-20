#include "jss.h"
#include "j_if.h"
#include "jer.h"
#include "jtable.h"
#include "jcs.h"

#include <stdlib.h>
#include <locale.h>

#define VER	"0.0.1"

#define _EMPTY	0
#define _CHAR	1
#define _INT	2
#define _DOUBLE	3
#define _JCS	4
#define _W	5
#define _RAW	6
#define _FORMULA	7
#define _LIST	100

static struct __types {
	int no;
	const wchar_t *str;
} _type[] = {
	{
	_EMPTY, L"empty"}, {
	_CHAR, L"char"}, {
	_INT, L"int"}, {
	_DOUBLE, L"double"}, {
	_JCS, L"jcs"}, {
	_W, L"wide"}, {
	_RAW, L"raw"}, {
	_FORMULA, L"formula"}, {
	_LIST, L"list"}, {
	0, 0}
};

int lookup_type(wchar_t * type)
{
	struct __types *c;

	for (c = _type; c->str; c++)
		if (!wcscmp(c->str, type))
			return c->no;
	return -JE_NOTFOUND;
}

void lookup_list()
{
	struct __types *c;

	for (c = _type; c->str; c++)
		wprintf(L"%S\n", c->str);
}

wchar_t *_mdir_str(struct jt_cell *cell)
{
	switch (cell->hdr.merge_dir) {
	case JTC_MERGE_NO:
		return L"Not merged";
	case JTC_MERGE_U:
		return L"UP";
	case JTC_MERGE_D:
		return L"DOWN";
	case JTC_MERGE_L:
		return L"LEFT";
	case JTC_MERGE_R:
		return L"RIGHT";
	default:
		return L"UNKNOWN";
	}
}

const wchar_t *_ctype_str(struct jt_cell *cell)
{
	struct __types *c;

	for (c = _type; c->str; c++)
		if (c->no == cell->hdr.type)
			return c->str;
	return L"Unknown";
}

void table_help()
{
	wprintf(L"\nSupported commands:\n\n");
	wprintf(L"\tSTAT\tStatus of table\n");
	wprintf(L"\tADD\tAdd cell\n");
	wprintf(L"\tLIST\tList cells\n");
	wprintf(L"\tRM\tRemove cell\n");
	wprintf(L"\tRMROW\tRemove row\n");
	wprintf(L"\tRMCOL\tRemove colums\n");
	wprintf(L"\tVIEW\tView table\n");
	wprintf(L"\tCELL\tView cell\n");
	wprintf(L"\tEXPORT\tConvert table to file\n");
	wprintf(L"\tIMPORT\tConvert from file to table\n");
	wprintf(L"\tSEARCH\tLists row matches\n");
	wprintf(L"\tEXIT\tExit to jss interface\n\n");
}

static inline void _show_cell_data(struct jt_cell *cell)
{
	switch (cell->hdr.type) {
	case JTC_TYPE_EMPTY:
		wprintf(L"[EMPTY]");
		break;
	case JTC_TYPE_CHAR:
		wprintf(L"%s", cell->data);
		break;
	case JTC_TYPE_INT:
		wprintf(L"%i", atoi(cell->data));
		break;
	case JTC_TYPE_FLOAT:
		wprintf(L"%f", atof(cell->data));
		break;
	case JTC_TYPE_JCS:
		wprintf(L"%s", cell->data);
		break;
	case JTC_TYPE_W:
		wprintf(L"%S", (wchar_t *) cell->data);
		break;
	case JTC_TYPE_RAW:
		wprintf(L"[RAW] %s", cell->data);
		break;
	default:
		wprintf(L"[UNKNOWN]");
		break;
	}
}

void cell_view(struct jt_cell *cell)
{
	wchar_t *comments;
	wprintf(L"\nflags: 0x%x\n", cell->hdr.flags);
	wprintf(L"merge dir: %S\n", _mdir_str(cell));
	wprintf(L"id: %u\n", cell->hdr.id);
	wprintf(L"row: %u\n", cell->hdr.row);
	wprintf(L"col: %u\n", cell->hdr.col);
	wprintf(L"type: %S (0x%x)\n", _ctype_str(cell), cell->hdr.type);
	wprintf(L"data offset: %u\n", cell->hdr.off);
	wprintf(L"datalen: %u\n", cell->hdr.datalen);
	wprintf(L"comments' len: %u\n", cell->hdr.clen);
	wprintf(L"format data len: %u\n", cell->hdr.fdatalen);

	wprintf(L"data: ");
	_show_cell_data(cell);
	if (cell->hdr.clen) {
		comments = (wchar_t *) malloc(CWBYTES(cell->comments));
		jcstow(cell->comments, comments, 0);
		wprintf(L"\ncomments: %S\n", comments);
		free(comments);
	} else {
		wprintf(L"\nno comments\n");
	}

	if (cell->hdr.fdatalen) {
		wprintf(L"\nformatting data: %s\n", cell->fdata);
	} else {
		wprintf(L"\nno formatting data\n");
	}

}

void cell_list(struct jt_table *t)
{

	struct jt_cell *cell;
	for (cell = t->first_cell; cell; cell = cell->next) {
		cell_view(cell);
	}

}

void table_stat(struct jt_table *t)
{

	wprintf(L"Name: %S\n", t->name);
	wprintf(L"Id: %u\n", t->hdr.id);
	wprintf(L"Rows: %u\n", t->hdr.nrows);
	wprintf(L"Columns: %u\n", t->hdr.ncols);
	wprintf(L"Cells: %u\n", t->hdr.ncells);
	wprintf(L"Last cell id: %u\n", t->hdr.last_cell_id);

}

/*
static inline void _get_cell_data(struct jt_cell* cell){
	wchar_t str_type[20];
	int maxlen;
	
get_type:
	wprintf(L"Enter type (type 'list' to list): ");
	wscanf(L"%S", str_type);
	switch(lookup_type(str_type)){
		case _LIST:
			lookup_list();
			goto get_type;
			break;
		case _EMPTY:
			cell->hdr.type = JTC_TYPE_EMPTY;
			break;
		case _CHAR:
			cell->hdr.type = JTC_TYPE_CHAR;
		case _JCS:
			if(cell->hdr.type == JTC_TYPE_EMPTY)
				cell->hdr.type = JTC_TYPE_JCS;
		case _RAW:
			if(cell->hdr.type == JTC_TYPE_EMPTY)
				cell->hdr.type = JTC_TYPE_RAW;

			wprintf(L"MaxLen: ");
			wscanf(L"%i", &maxlen);
			cell->data = (uchar*)malloc(maxlen);
			wprintf(L"String: ");
			wscanf(L"%s", cell->data);
			cell->hdr.datalen = strlen(cell->data) + 1;
			break;
		case _W:
			cell->hdr.type = JTC_TYPE_W;
			wprintf(L"MaxLen (in bytes): ");
			wscanf(L"%i", &maxlen);
			cell->data = (uchar*)malloc(maxlen);
			wprintf(L"Wide String: ");
			wscanf(L"%S", (wchar_t*)cell->data);
			cell->hdr.datalen=(wcslen((wchar_t*)cell->data) + 1)*sizeof(wchar_t);
			break;
		
		default:
			wprintf(L"No implemented\n");
			break;
	}
}
*/

void view_table(struct jt_table *t)
{
	struct jt_cell *cell;
//      uint32_t i, j;

	wprintf(L"sorting table...\n");
	jt_sort_table(t);

	for (cell = t->first_cell; cell; cell = cell->next) {
		wprintf(L"[R%u, C%u]-> ", cell->hdr.row, cell->hdr.col);
		_show_cell_data(cell);
		wprintf(L"\n");
	}

/*	
	wprintf(L"[%S]\t", t->name);
	for(n = 0; n < t->hdr.ncols; n++)
		wprintf(L"COL[%u]\t", n);
	wprintf(L"\n");

	cell = t->first_cell;
	for(n = 0; n < t->hdr.nrows; n++){
			
		
		for(i = 0; i < t->hdr.ncols; i++){
			if(cell->hdr.row != n) break;
			if(cell->hdr.col == i){
				_show_cell_data(cell);
				wprintf(L"\t");
			} else {
				wprintf(L"\t", i, cell->hdr.col);
			}
			
			cell = cell->next;
			if(!cell) {
				return;
				wprintf(L"\n");
			}
		}

		wprintf(L"\n");
	}
*/

}

void cell_add(struct jt_table *t)
{

	wchar_t str[200];
	wchar_t cstr[100];
	int ret;
	uint32_t row, col;
	struct jt_cell *cell;
	char data[200];
	uchar *comments;
	uint32_t clen = 0UL;
	uchar *fmt;
	uint32_t flen = 0UL;

	wprintf(L"Row: ");
	wscanf(L"%u", &row);
	wprintf(L"Col: ");
	wscanf(L"%u", &col);
	wprintf(L"Has data ( y / n )? ");
	wscanf(L"%S", str);
	if (str[0] == 'y') {
		wprintf(L"data: ");
		wscanf(L"%s", data);
	} else {
		data[0] = '\0';
	}

	wprintf(L"Comments (Type 'no' for no comments): ");
	wscanf(L"%S", str);
	if (wcscmp(str, L"no")) {
		clen = wtojcs_len(str, 0) + 1;
		comments = (uchar *) malloc(clen);
		wtojcs(str, comments, clen);
	}

	wprintf(L"Format data (Type 'no' for no format): ");
	wscanf(L"%S", str);
	if (wcscmp(str, L"no")) {
		flen = wtojcs_len(str, 0) + 1;
		fmt = (uchar *) malloc(flen);
		wtojcs(str, fmt, flen);
	}

	wprintf(L"creating cell...");
	cell = jt_create_cell(data, strlen(data), comments, clen, fmt, flen,
			      JTC_TYPE_EMPTY, JTC_MERGE_NO, row, col);

	if (!cell) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(-JE_MALOC);
		return;
	}

	wprintf(L"\t[DONE]\n");

	wprintf(L"adding cell...");

	ret = jt_add_cell(t, cell);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;
	}

	wprintf(L"\t[DONE]\n");
}

void cell_rm(struct jt_table *t)
{
	uint32_t row, col;
	int ret;
	wprintf(L"row: ");
	wscanf(L"%u", &row);
	wprintf(L"column: ");
	wscanf(L"%u", &col);

	ret = jt_rm_cell(t, row, col);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;
	}

	wprintf(L"\t[DONE]\n");

}

void rm_row(struct jt_table *t)
{
	uint32_t row;
	int ret;
	wprintf(L"row: ");
	wscanf(L"%u", &row);
	wprintf(L"remap ( 0 = no, 1 = yes ): ");
	wscanf(L"%i", &ret);

	ret = jt_rm_row(t, row, ret);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;
	}

	wprintf(L"\t[DONE]\n");

}

void rm_col(struct jt_table *t)
{
	uint32_t col;
	int ret;
	wprintf(L"column: ");
	wscanf(L"%u", &col);
	wprintf(L"remap ( 0 = no, 1 = yes ): ");
	wscanf(L"%i", &ret);

	ret = jt_rm_col(t, col, ret);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;
	}

	wprintf(L"\t[DONE]\n");

}

void view_cell(struct jt_table *t)
{
	uint32_t row, col;
	struct jt_cell *cell;
	int ret;
	wprintf(L"row: ");
	wscanf(L"%u", &row);
	wprintf(L"column: ");
	wscanf(L"%u", &col);

	ret = jt_get_cell(t, row, col, &cell);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;
	}

	wprintf(L"\t[DONE]\n");

	cell_view(cell);

}

void export_table(struct jt_table *t)
{

	wchar_t filename[JSS_MAXFILENAME];
	int eol;
	uchar delim;
	int ret;

	wprintf(L"filename: ");
	wscanf(L"%S", filename);

 get_delim:
	wprintf(L"delimiter ( 1. space, 2. tab, 3. , ):");
	wscanf(L"%i", &eol);
	switch (eol) {
	case 1:
		delim = ' ';
		break;
	case 2:
		delim = '\t';
		break;
	case 3:
		delim = ',';
		break;
	default:
		wprintf(L"delimiter not supported in interface, try again\n");
		goto get_delim;
	}

 get_eol:
	wprintf(L"line termination ( 1. CR, 2. CRLF ):");
	wscanf(L"%i", &eol);
	switch (eol) {
	case JT_EOL_CR:
	case JT_EOL_CRLF:
		break;
	default:
		wprintf(L"line termination not supported, try again\n");
		goto get_eol;
	}

	wprintf(L"exporting table to %S...", filename);
	ret = jt_table_to_file(filename, t, delim, eol);

	if (ret < 0) {
		wprintf(L"[FAIL]\n");
		jif_perr(ret);
	} else {
		wprintf(L"[DONE]\n");
	}

}

void import_table(struct jt_table *t)
{
	wchar_t filename[JSS_MAXFILENAME];
	uchar *data;
	uint32_t id;
	int iid;
	int ret;
	char delim;

	wprintf(L"filename: ");
	wscanf(L"%S", filename);

	wprintf(L"table id: ");
	wscanf(L"%i", &iid);
	id = (uint32_t) iid;

 get_delim:
	wprintf(L"delimiter ( 1. space, 2. tab, 3. , ):");
	wscanf(L"%i", &iid);
	switch (iid) {
	case 1:
		delim = ' ';
		break;
	case 2:
		delim = '\t';
		break;
	case 3:
		delim = ',';
		break;
	default:
		wprintf(L"delimiter not supported, try again\n");
		goto get_delim;
	}

	wprintf(L"converting file %S...", filename);
	ret = jt_file_to_table(t, id, filename, delim);

	if (ret < 0) {
		wprintf(L"[FAIL]\n");
		jif_perr(ret);
	} else {
		wprintf(L"[DONE]\n");
	}

}

void search_table(struct jt_table *t)
{
	uchar data[1024];
	uint32_t nrows, *row_list, i;

	wprintf(L"data: ");
	wscanf(L"%s", data);

	wprintf(L"searching...");
	row_list = jt_list_rows(t, data, strlen(data), NULL, 0, NULL, 0, 0,
				JTC_SEARCH_DATA, &nrows);
	if (!row_list) {
		wprintf(L"\nNot found anything\n");
		return;
	}

	wprintf(L"\nmatches:\n");
	for (i = 0; i < nrows; i++) {
		wprintf(L"%u\n", row_list[i]);
	}
	free(row_list);
}

int table_jss(struct jt_table *t)
{
	wchar_t tname[200];
	wchar_t cmd[20];
	int ret;
	uint32_t row;
	uint32_t col;
	int iid;
	uint32_t id;

	if (t->hdr.nrows == 0) {
		wprintf(L"table not init\n");
		wprintf(L"Name (type import to skip) init: ");
		wscanf(L"%S", tname);
		if (!wcscmp(tname, L"import"))
			goto prompt;

		wprintf(L"rows: ");
		wscanf(L"%u", &row);

		wprintf(L"columns: ");
		wscanf(L"%u", &col);

		wprintf(L"id: ");
		wscanf(L"%i", &iid);
		id = (uint32_t) iid;

		wprintf(L"creating table...");

		ret = jt_create_table(t, row, col, id);
		if (ret < 0) {
			wprintf(L"[FAIL]\n");
			jif_perr(ret);
			return ret;
		} else {
			wprintf(L"[DONE]\n");
		}

		t->name =
		    (wchar_t *) malloc((wcslen(tname) + 1) * sizeof(wchar_t));
		wcscpy(t->name, tname);

	}

 prompt:
	wprintf(L"\nTableMain >");
	wscanf(L"%S", cmd);

	switch (lookup_cmd(cmd)) {
	case STAT:
		table_stat(t);
		break;
	case ADD:
		cell_add(t);
		break;
	case LIST:
		cell_list(t);
		break;
	case RM:
		cell_rm(t);
		break;
	case RMROW:
		rm_row(t);
		break;
	case RMCOL:
		rm_col(t);
		break;
	case VIEW:
		view_table(t);
		break;
	case CELL:
		view_cell(t);
		break;
	case EXPORT:
		export_table(t);
		break;
	case IMPORT:
		import_table(t);
		break;
	case SEARCH:
		search_table(t);
		break;
	case HELP:
		table_help();
		break;
	case EXIT:
		return 0;
	default:
		wprintf(L"\nCommand not supported, try using \'HELP\'\n");
		break;
	}
	goto prompt;
	return 0;

}
