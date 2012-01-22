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

#define VER	"0.0.1"
#define PROMPT_LEN	20

void jdbif_table_type_help()
{
	wprintf(L"\nSupported commands:\n\n");
	wprintf(L"\t1\tAdd typedef to the table\n");
	wprintf(L"\t2\tList typedefs of the table\n");
	wprintf(L"\t3\tRemove typedef from the table\n");
	wprintf(L"\t4\tAdd column type entry\n");
	wprintf(L"\t5\tList column types\n");
	wprintf(L"\t6\tRemove column type entry\n");
	wprintf(L"\tCLOSE\tClose table\n");
	wprintf(L"\tEXIT\tExit interface\n");
}

void jdbif_dump_typedef_hdr(struct jdb_handle *h, struct jdb_typedef_blk_hdr hdr){
	wprintf
	    (L"\tType: 0x%02x\tFlags: 0x%02x\tNSet: ? of %u\tCRC32: 0x%04x\n",
	     hdr.type, hdr.flags, h->hdr.typedef_bent, hdr.crc32);

}

void jdbif_dump_typedef_entries(struct jdb_handle *h, struct jdb_typedef_blk* blk){
	jdb_bent_t bent;
	wprintf(L"\t[slot]-> flags; type_id; base_type_id; type_length (chunksize if VAR flag is set)\n");
	for(bent = 0; bent < h->hdr.typedef_bent; bent++){
		if(blk->entry[bent].type_id != JDB_TYPE_EMPTY){
			wprintf(L"[%u]-> 0x%02x; 0x%02x; 0x%02x; %u", bent,
				blk->entry[bent].flags, blk->entry[bent].type_id,
				blk->entry[bent].base, blk->entry[bent].len);
			if (!(bent % 2))
				wprintf(L"\n");
			else
				wprintf(L"\t");
		}
	}
}

void jdbif_dump_col_typedef_hdr(struct jdb_handle* h, struct jdb_col_typedef_blk_hdr hdr){
	wprintf
	    (L"\tType: 0x%02x\tFlags: 0x%02x\tNSet: ? of %u\tCRC32: 0x%04x\n",
	     hdr.type, hdr.flags, h->hdr.col_typedef_bent, hdr.crc32);	
}

void jdbif_dump_col_typedef_entries(struct jdb_handle* h, struct jdb_col_typedef* blk){
	jdb_bent_t bent;
	wprintf(L"[slot]-> type_id; column_id\n");
	for(bent = 0; bent < h->hdr.col_typedef_bent; bent++){
		if(blk->entry[bent].type_id != JDB_TYPE_EMPTY){
			wprintf(L"[%u]-> 0x%02x; %u", bent, blk->entry[bent].type_id,
				blk->entry[bent].col);
			if (!(bent % 4))
				wprintf(L"\n");
			else
				wprintf(L"\t");
		}			
	}
}

void jdbif_table_type_add_typedef(struct jdb_handle* h){
	wchar_t name[256], flag_str[10];
	int ret;
	uchar type_id, base, flags;
	uint32_t len;

	wprintf(L"Table name: ");
	wscanf(L"%ls", name);
	wprintf(L"Type Id(%i ~ 255): ", JDB_TYPE_NULL+0x01);
	wscanf(L"%i", &ret);
	type_id = (uchar)ret;
	wprintf(L"Base Type(%i ~ %i): ", JDB_TYPE_BYTE, JDB_TYPE_NULL);
	wscanf(L"%i", &ret);
	base = (uchar)ret;

	wprintf(L"flags are: \n");
	wprintf(L"\tBIT0: variable data length,\n\t the length field will be your segment size in this case.\n");

 get_bit_str:
	flags = 0;
	wmemset(flag_str, L'\0', 10);
	wprintf(L"flags bit string: ");
	wscanf(L"%ls", flag_str);

	for (ret = 0; ret < wcslen(flag_str); ret++)
		if (flag_str[ret] == L'1')
			flags |= 0x0001 << ret;
		else if (flag_str[ret] == L'0') {

		} else {
			wprintf
			    (L"could not understand position %i in bit string, please use 0 or 1\n",
			     ret);
			goto get_bit_str;
		}
	
	if(flags & JDB_TYPE_VAR)
		wprintf(L"Segment Size: ");
	else
		wprintf(L"Data Length: ");
	wscanf(L"%u", &len);

	wprintf(L"creating typedef...");
	ret = jdb_add_typedef(h, name, type_id, base, len, flags);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");	
	
}

void jdbif_table_type_list_typedefs(struct jdb_handle* h){

	wchar_t name[256];
	int ret;
	struct jdb_table* table;
	struct jdb_typedef_blk* blk;

	wprintf(L"Table name: ");
	wscanf(L"%ls", name);
	wprintf(L"searching table...");
	ret = _jdb_table_handle(h, name, &table);
	if (ret < 0) {
		wprintf(L"\t[FAIL]; Is it open?\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");	
	
	wprintf(L"Typedef List{\n");
	for(blk = table->typedef_list.first; blk; blk = blk->next){
		jdbif_dump_typedef_hdr(h, blk->hdr);
		jdbif_dump_typedef_entries(h, blk);
		wprintf(L"\n------------------------\n");
	}
	wprintf(L"}Typedef List;\n");

}

void jdbif_table_type_rm_typedef(struct jdb_handle* h){
	wchar_t name[256];
	int ret;
	uchar type_id;

	wprintf(L"Table name: ");
	wscanf(L"%ls", name);
	wprintf(L"Type Id(%i ~ 255): ", JDB_TYPE_NULL+1);
	wscanf(L"%i", &ret);
	type_id = (uchar)ret;

	wprintf(L"removing typedef...");
	ret = jdb_rm_typedef(h, name, type_id);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");	
	
}

void jdbif_table_type_assign_col_type(struct jdb_handle* h){

	wchar_t name[256];
	int ret;
	uchar type_id;
	uint32_t col;

	wprintf(L"Table name: ");
	wscanf(L"%ls", name);
	wprintf(L"Type Id(%i ~ 255): ", JDB_TYPE_NULL+1);
	wscanf(L"%i", &ret);
	type_id = (uchar)ret;
	wprintf(L"Column: ");
	wscanf(L"%u", &col);	

	wprintf(L"assigning type...");
	ret = jdb_assign_col_type(h, name, type_id, col);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");	
	
}

void jdbif_table_type_find_col_type(struct jdb_handle* h){

	wchar_t name[256];
	int ret;
	uchar type_id;
	uint32_t col;

	wprintf(L"Table name: ");
	wscanf(L"%ls", name);
	wprintf(L"Column: ");
	wscanf(L"%u", &col);
	
	wprintf(L"finding column type...");
	type_id = jdb_find_col_type(h, name, col);
	wprintf(L"\t[ %i (0x%02x) ]\n", (int)type_id, type_id);

}

void jdbif_table_type_rm_col_type(struct jdb_handle* h){

	wchar_t name[256];
	int ret;
	uchar type_id;
	uint32_t col;

	wprintf(L"Table name: ");
	wscanf(L"%ls", name);
	wprintf(L"Column: ");
	wscanf(L"%u", &col);
		
	wprintf(L"removing column type...");
	ret = jdb_rm_col_type(h, name, col);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");	
	
}



int jdbif_table_type(struct jdb_handle *h)
{

	wchar_t cmd[MAX_CMD];
	int ret;
	char jerr[MAX_ERR_STR];

	wprintf
	    (L"\nType sub-system, type \'HELP\' to get a list of commands.\n");

 prompt1:
	jdbif_set_prompt(L"Type >");
	jdbif_prompt();
	wscanf(L"%ls", cmd);

	switch (lookup_cmd(cmd)) {
	case HELP:
		jdbif_table_type_help();
		break;
	case _ONE:
		jdbif_table_type_add_typedef(h);
		break;
	case _TWO:
		jdbif_table_type_list_typedefs(h);
		break;
	case _THREE:
		jdbif_table_type_rm_typedef(h);
		break;
	case _FOUR:
		jdbif_table_type_assign_col_type(h);
		break;
	case _FIVE:
		jdbif_table_type_find_col_type(h);
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

