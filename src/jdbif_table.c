#include "jdb.h"
#include "jdb_intern.h"
#include "jer.h"
#include "debug.h"
#include "j_if.h"
#include "jconst.h"

void jdbif_table_help()
{
	wprintf(L"\nSupported commands:\n\n");
	wprintf(L"\tFIND\tFind table\n");
	wprintf(L"\tLIST\tList of database tables\n");
	wprintf(L"\tLLIST\tList of loaded tables\n");
	wprintf(L"\tSTAT\tStatus of a table\n");
	wprintf(L"\tADD\tAdd table\n");
	wprintf(L"\tRM\tRemove table\n");
	wprintf(L"\tREN\tRename a table\n");	
	wprintf(L"\tOPEN\tOpen table\n");
	wprintf(L"\tTYPE\tType operation sub-system\n");
	wprintf(L"\tCELL\tCell management sub-system\n");
	wprintf(L"\tSYNC\tSync table\n");
	wprintf(L"\tSYNCALL\tSync all tables\n");
	wprintf(L"\tCLOSE\tClose table\n");
	wprintf(L"\tEXIT\tExit interface\n");
}

void jdbif_list_tables(struct jdb_handle *h)
{
	jdb_bid_t n;
	jcslist_entry *list, *entry;
	int ret;

	wprintf(L"getting list...");
	ret = jdb_list_tables(h, &list, &n);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");

	wprintf(L"Table List{\n");
	ret = 0;
	for (entry = list; entry; entry = entry->next) {
		wprintf(L"\t%ls\n", entry->wcs);
		ret++;
	}
	wprintf(L"}Table List ( %i tables );\n", ret);
	jcslist_free(list);
}

void jdbif_llist_tables(struct jdb_handle *h)
{
	struct jdb_table *table;	
	wprintf(L"Loaded Table List (cnt = %u) {\n", h->table_list.cnt);
	for (table = h->table_list.first; table; table = table->next) {
		wprintf(L"\t%ls - TID: %u\n", table->main.name, table->main.hdr.tid);
	}
	wprintf(L"}Loaded Table List;\n");
}

void jdbif_dump_table_def(struct jdb_table_def_blk *table_def)
{

	wprintf(L"\tName: %ls\n", table_def->name);
	wprintf(L"\tTID: %u\n", table_def->hdr.tid);
	wprintf(L"\tnamelen (in jcs): %u\n", table_def->hdr.namelen);
	wprintf(L"\trows: %u, columns: %u\n", table_def->hdr.nrows,
		table_def->hdr.ncols);
	wprintf(L"\tcells: %u\n", table_def->hdr.ncells);
	wprintf(L"\tncol_typedef: %u\n", table_def->hdr.ncol_typedef);
	wprintf(L"\tcrc32: 0x%08x\n", table_def->hdr.crc32);
}

void jdbif_stat_table(struct jdb_handle *h)
{
	wchar_t name[256];
	int ret;
	struct jdb_table *table;

	wprintf(L"Table name: ");
	wscanf(L"%ls", name);
	
	wprintf(L"Searching for table...");
	
	for(table = h->table_list.first; table; table = table->next){
		if(!wcscmp(table->main.name, name)) break;
	}
	
	if(!table){
		wprintf(L"\t[FAIL]\n");
		wprintf(L"Not found in memmory, try openning table.\n");
		return;
	}
	
	wprintf(L"\t[DONE]\n");
	
	wprintf(L"Table Defintion Dump{\n");
	jdbif_dump_table_def(&table->main);
	wprintf(L"}Table Defintion Dump;\n");
}

void jdbif_add_table(struct jdb_handle* h){

	wchar_t name[256];
	int ret;
	struct jdb_table *table;
	uint32_t rows, cols;

	wprintf(L"Table name: ");
	wscanf(L"%ls", name);
	wprintf(L"rows: ");
	wscanf(L"%u",&rows);
	wprintf(L"cols: ");
	wscanf(L"%u",&cols);	

	wprintf(L"creating table...");
	ret = jdb_create_table(h, name, rows, cols, 0, 0);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");	
	
}

void jdbif_close_table(struct jdb_handle* h){

	wchar_t name[256];
	int ret;
	struct jdb_table *table;
	uint32_t rows, cols;

	wprintf(L"Table name: ");
	wscanf(L"%ls", name);

	wprintf(L"closing table...");
	ret = jdb_close_table(h, name);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");	
	
}

void jdbif_open_table(struct jdb_handle* h){

	wchar_t name[256];
	wchar_t flag_str[9];
	int ret;
	struct jdb_table *table;
	uint32_t rows, cols;
	uchar flags;

	wprintf(L"Table name: ");
	wscanf(L"%ls", name);

	wprintf(L"flags are: \n");
	wprintf(L"\tBIT0: bind columns\n");
	wprintf(L"\tBIT1: bind rows\n");
	wprintf(L"\tBIT2: enforce column type\n");
	wprintf(L"\tBIT3: load data on openning\n");

 get_bit_str:
	flags = 0;
	wmemset(flag_str, L'\0', 9);
	wprintf(L"flags bit string: ");
	wscanf(L"%ls", flag_str);

	for (ret = 0; ret < wcslen(flag_str); ret++)
		if (flag_str[ret] == L'1')
			flags |= 0x01 << ret;
		else if (flag_str[ret] == L'0') {

		} else {
			wprintf
			    (L"could not understand position %i in bit string, please use 0 or 1\n",
			     ret);
			goto get_bit_str;
		}

	wprintf(L"opening table...");
	ret = jdb_open_table(h, name, flags);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");	
	
}


void jdbif_sync_table(struct jdb_handle* h){

	wchar_t name[256];
	int ret;
	struct jdb_table *table;
	uint32_t rows, cols;

	wprintf(L"Table name: ");
	wscanf(L"%ls", name);

	wprintf(L"syncing table...");
	ret = jdb_sync_table(h, name);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");	
	
}

void jdbif_find_table(struct jdb_handle* h){

	wchar_t name[256];
	int ret;
	struct jdb_table *table;
	uint32_t rows, cols;

	wprintf(L"Table name: ");
	wscanf(L"%ls", name);

	wprintf(L"searching for table...");
	ret = jdb_find_table(h, name);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");
	
}



int jdbif_table(struct jdb_handle *h)
{

	wchar_t cmd[MAX_CMD];
	int ret;
	char jerr[MAX_ERR_STR];

	wprintf
	    (L"\nTable sub-system, type \'HELP\' to get a list of commands.\n");

 prompt1:
	jdbif_set_prompt(L"Table >");
	jdbif_prompt();
	wscanf(L"%ls", cmd);

	switch (lookup_cmd(cmd)) {
	case HELP:
		jdbif_table_help();
		break;
	case FIND:
		jdbif_find_table(h);
		break;		
	case LIST:
		jdbif_list_tables(h);
		break;
	case LLIST:
		jdbif_llist_tables(h);
		break;
	case STAT:
		jdbif_stat_table(h);
		break;
	case ADD:
		jdbif_add_table(h);
		break;
	case RM:
		//jdbif_rm_table(h);
		wprintf(L"Not Implemented Now!\n");
		break;
	case CLOSE:
		jdbif_close_table(h);
		break;
	case SYNC:
		jdbif_sync_table(h);
		break;
	case SYNCALL:
		wprintf(L"syncing all tables...");
		ret = jdb_sync_tables(h);
		if (ret < 0) {
			wprintf(L"\t[FAIL]\n");
			jif_perr(ret);

		} else
			wprintf(L"\t[DONE]\n");	
		break;
	case OPEN:
		jdbif_open_table(h);
		break;
	case REN:
		wprintf(L"Not Implemented Now!\n");
		break;
		
	case TYPE:
		//wprintf(L"Not Implemented Now!\n");
		jdbif_table_type(h);
		break;
	case CELL:
		jdbif_table_cell(h);
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
