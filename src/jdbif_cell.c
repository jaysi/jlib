#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <locale.h>

#include "jdb.h"
#include "jdb_intern.h"
#include "jer.h"
#include "debug.h"
#include "j_if.h"
#include "jconst.h"
#include "jtime.h"

#define VER	"0.0.1"
#define PROMPT_LEN	20

void jdbif_cell_help()
{
	wprintf(L"\nSupported commands:\n\n");
	wprintf(L"\tADD\tAdd cell\n");
	wprintf(L"\tRM\tRemove cell\n");
	wprintf(L"\tFIND\tFind cell\n");
	wprintf(L"\tLIST\tList cells\n");
	wprintf(L"\tLISTALL\tList all cells\n");
	wprintf(L"\tEDIT\tEdit cell\n");
	wprintf(L"\tEXIT\tExit interface\n");
}

void jdbif_dump_celldef(struct jdb_handle* h, struct jdb_cell* cell){
	wprintf(L"R: %u\tC: %u\tDCRC: 0x%04x\tDTYP: 0x%02x\nDLEN: %u\tDPTR: %u\tDPTRE: %u\nACC: %i-%i-%i %i:%i:%i\n"
	, cell->celldef->row, cell->celldef->col,
	cell->celldef->data_crc32, cell->celldef->data_type,
	cell->celldef->datalen, cell->celldef->bid_entry, cell->celldef->bent,
	(int)cell->celldef->last_access_d[0]+BASE_YEAR,
	(int)cell->celldef->last_access_d[1],
	(int)cell->celldef->last_access_d[2],
	(int)cell->celldef->last_access_t[0],
	(int)cell->celldef->last_access_t[1],
	(int)cell->celldef->last_access_t[2]);
}

void jdbif_dump_dptr(struct jdb_handle* h, struct jdb_cell_data_ptr_blk_entry *dptr){
		wprintf(L"%u:%u:%u:%u:%u -> ",
		dptr->bid, dptr->bent, dptr->nent, dptr->nextdptrbid,
		dptr->nextdptrbent);
}
void jdbif_dump_cell_dptr(struct jdb_handle* h, struct jdb_cell* cell){
	struct jdb_cell_data_ptr_blk_entry *dptr;
	int i = 0;
	dptr = cell->dptr;
	wprintf(L"Data pointer list ( BID:FirstEnt:NoEnt:NextDptrBID:NextDptrEnt ){\n");
	while(dptr){
		jdbif_dump_dptr(h, dptr);
		if(!(i%4) && i)wprintf(L"\n");
		else wprintf(L"\t");		
		dptr = dptr->next;
	}
	wprintf(L"||\n");
	wprintf(L"}Data pointer list;\n");
}

void jdbif_dump_dptr_blk(struct jdb_handle* h, struct jdb_cell_data_ptr_blk* blk){
	struct jdb_cell_data_ptr_blk_entry *dptr;
	jdb_bent_t i;
	wprintf(L"Data pointer block ( BID:FirstEnt:NoEnt:NextDptrBID:NextDptrEnt ){\n");
	for(i = 0; i < h->hdr.dptr_bent; i++){
		jdbif_dump_dptr(h, &blk->entry[i]);
		if(!(i%4) && i)wprintf(L"\n");
		else wprintf(L"\t");		
	}
	wprintf(L"}Data pointer block;\n");
}

void jdbif_add_cell(struct jdb_handle* h){
	wchar_t table_name[100];
	uint32_t col, row;
	uchar* data;
	uint32_t datalen;
	uchar data_type;
	int ret;
	wchar_t yn[2];
	uchar filename[100];

	wprintf(L"table name: ");
	wscanf(L"%ls", table_name);
	wprintf(L"col: ");
	wscanf(L"%u", &col);
	wprintf(L"row: ");
	wscanf(L"%u", &row);
	wprintf(L"datalen ( type '0' for options ): ");
	wscanf(L"%u", &datalen);
	if(!datalen){
		wprintf(L"load from file (y/n)?");
		wscanf(L"%ls", yn);
		if(yn[0] == L'y'){
			
			if((ret = jif_read_file(filename, &data, &datalen)) < 0){
				jif_perr(ret);	
				return;
			}
		}
	} else {
		data = (uchar*)malloc(datalen);
		if(!data){
			wprintf(L"No Memmory");
			return;
		}
		wprintf(L"data: ");
		scanf("%s", data);
	}
	wprintf(L"data_type: ");
	wscanf(L"%i", &ret);
	data_type = (uchar)ret;

	wprintf(L"creating cell...");
	ret = jdb_create_cell(h, table_name, col, row, data, datalen, data_type);
		
	if(ret < 0){
		wprintf(L"[FAIL]\n");
		jif_perr(ret);
		return;
	}
	wprintf(L"[DONE]\n");
	
}

void jdbif_rm_cell(struct jdb_handle* h){
	wchar_t table_name[100];
	uint32_t col, row;
	int ret;
	
	wprintf(L"table name: ");
	wscanf(L"%ls", table_name);
	wprintf(L"col: ");
	wscanf(L"%u", &col);
	wprintf(L"row: ");
	wscanf(L"%u", &row);	
	
	wprintf(L"removing cell...");
	
	ret = jdb_rm_cell(h, table_name, col, row);
	
	if(ret < 0){
		wprintf(L"[FAIL]\n");
		jif_perr(ret);
		return;
	}
	wprintf(L"[DONE]\n");
}

void jdbif_find_cell(struct jdb_handle* h){
		
	wchar_t table_name[100];
	char filename[J_MAX_FILENAME8];
	uint32_t col, row;
	uchar* data;
	uint32_t datalen;
	jdb_data_t data_type, base;	
	int ret, unsign;
	wchar_t yn[2];
	struct jdb_cell* cell;
	
	wprintf(L"table name: ");
	wscanf(L"%ls", table_name);
	wprintf(L"col: ");
	wscanf(L"%u", &col);
	wprintf(L"row: ");
	wscanf(L"%u", &row);	
	
	wprintf(L"loading cell...");
	
/*
int jdb_load_cell(struct jdb_handle *h, wchar_t * table_name,
		uint32_t col, uint32_t row, uchar** data,
		uint32_t* datalen, uchar* data_type, int* unsign){
*/
	ret = jdb_load_cell(h, table_name, col, row, &data, &datalen, &data_type);
	
	if(ret < 0 && ret != -JE_EXISTS){
		wprintf(L"[FAIL]\n");
		jif_perr(ret);
		return;
	}
	wprintf(L"[DONE]\n");
	
	wprintf(L"datalen: %u, datatype: 0x%02x\n", datalen, data_type);
	
	wprintf(L"print data(y/n/f)?");
	wscanf(L"%ls", yn);
	
	switch(yn[0]){
		case L'y':
			wprintf(L"-=BEGIN=-\n");
			ret = jdb_find_type_base(h, table_name, data_type, &base);
			if(ret < 0){
				wprintf(L"[FAIL]\n");
				jif_perr(ret);
				return;				
			}
			jdbif_print_data(data, datalen, base);
			wprintf(L"\n-=END=-\n");
			break;
		case L'f':
			wprintf(L"filename: ");
			wscanf(L"%s", filename);
			ret = jif_write_file(filename, data, datalen);
						
			break;
		default:
			break;
				
	}
}

void jdbif_list_all_cells(struct jdb_handle* h){
	wchar_t table_name[100];
	int ret;
	uint32_t col, row, *li_col, *li_row, n, i;

	wprintf(L"table name: ");
	wscanf(L"%ls", table_name);

	wprintf(L"creating cell list...");
	ret = jdb_list_cells(h, table_name, JDB_ID_INVAL, JDB_ID_INVAL, &li_col, &li_row, &n);
	if(ret < 0){
		wprintf(L"[FAIL]\n");
		jif_perr(ret);
		return;
	}
	wprintf(L"[DONE]\n");
	wprintf(L"found %u cells, print in [col, row] order.\n", n);
	for(i = 0; i < n; i++){
		wprintf(L"[%u, %u]", li_col[i], li_row[i]);
		if(i && !(i%4)) wprintf(L"\n");
		else wprintf(L"\t");
	}

	if(n){
		free(li_col);
		free(li_row);
	}
	
}

void jdbif_list_cells(struct jdb_handle* h){
	wchar_t table_name[100];
	int ret;
	uint32_t col, row, *li_col, *li_row, n, i;

	wprintf(L"table name: ");
	wscanf(L"%ls", table_name);
	wprintf(L"col(Enter %u for all matches): ", JDB_ID_INVAL);
	wscanf(L"%u", &col);
	wprintf(L"row(Enter %u for all matches): ", JDB_ID_INVAL);
	wscanf(L"%u", &row);	

	wprintf(L"creating cell list...");
	ret = jdb_list_cells(h, table_name, col, row, &li_col, &li_row, &n);
	if(ret < 0){
		wprintf(L"[FAIL]\n");
		jif_perr(ret);
		return;
	}
	wprintf(L"[DONE]\n");
	wprintf(L"found %u matches, print in [col, row] order.\n", n);
	for(i = 0; i < n; i++){
		wprintf(L"[%u, %u]", li_col[i], li_row[i]);
		if(i && !(i%4)) wprintf(L"\n");
		else wprintf(L"\t");
	}

	if(n){
		free(li_col);
		free(li_row);
	}
	
}

int jdbif_table_cell(struct jdb_handle *h)
{

	wchar_t cmd[MAX_CMD];
	int ret;
	char jerr[MAX_ERR_STR];

	wprintf
	    (L"\tCell sub-system, type \'HELP\' to get a list of commands.\n");

 prompt1:
	jdbif_set_prompt(L"Cell >");
	jdbif_prompt();
	wscanf(L"%ls", cmd);

	switch (lookup_cmd(cmd)) {
	case HELP:
		jdbif_cell_help();
		break;
	case FIND:
		jdbif_find_cell(h);
		break;		
	case LIST:
		jdbif_list_cells(h);
		break;
	case LISTALL:
		jdbif_list_all_cells(h);
		break;	
	case STAT:
		//jdbif_stat_cell(h);
		break;
	case ADD:
		jdbif_add_cell(h);
		break;
	case RM:
		jdbif_rm_cell(h);		
		break;
	case EDIT:
		//jdbif_edit_cell(h);
		break;		
	case EXIT:
		return 0;
	default:
		wprintf(L"\nCommand not supported, try using \'HELP\'\n");
		break;
	}
	goto prompt1;
	return 0;

}

