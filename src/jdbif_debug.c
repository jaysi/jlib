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

void jdbif_dump_fav_blk_hdr(struct jdb_handle* h, struct jdb_fav_blk_hdr hdr){
	wprintf
	    (L"\tType: 0x%02x\tFlags: 0x%02x\tCRC32: 0x%04x\n",
	     hdr.type, hdr.flags, hdr.crc32);			
}

void jdbif_dump_fav_blk_entries(struct jdb_handle* h, struct jdb_fav_blk* blk){
	jdb_bent_t i;
	wprintf(L"\t[slot]-> BID; HITS;\n");
	for(i = 0; i < h->hdr.fav_bent; i++){
		wprintf(L"\t[%u]-> %u:%u");
		if(!(i%4)) wprintf(L"\n");
		else wprintf(L"\t");
	}
}

void jdbif_dump_fav_blk(struct jdb_handle* h, jdb_bid_t bid){
	struct jdb_fav_blk blk;
	int ret;
	
	blk.bid = bid;
	
	wprintf(L"reading fav block...");
	
	ret = _jdb_read_fav_blk(h, &blk);
	
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);		
		return;

	} else
		wprintf(L"\t[DONE]\n");
	
	wprintf(L"Fav Block dump{\n");
	jdbif_dump_fav_blk_hdr(h, blk.hdr);
	jdbif_dump_fav_blk_entries(h, &blk);
	free(blk.entry);
	wprintf(L"}Fav Block dump;\n");
	
	
}

void jdbif_dump_map_hdr(struct jdb_handle *h, struct jdb_map_blk_hdr hdr)
{
	wprintf
	    (L"\tType: 0x%02x\tFlags: 0x%02x\tNSet: %u of %u\tCRC32: 0x%04x\n",
	     hdr.type, hdr.flags, hdr.nset, h->hdr.map_bent, hdr.crc32);
}

void jdbif_dump_map_entries_all(struct jdb_handle *h, struct jdb_map *map)
{
	jdb_bent_t i;
	wprintf
	    (L"\t[slot]-> block_type; data_type; full_entries; table_id; bid\n");
	for (i = 0; i < h->hdr.map_bent; i++) {
		wprintf(L"\t[%u]-> 0x%02x; 0x%02x; %u; %u; #%u",
			i, map->entry[i].blk_type, map->entry[i].dtype,
			map->entry[i].nful, map->entry[i].tid,
			map->bid + i + 1);

		if (!(i % 2))
			wprintf(L"\n");
		else
			wprintf(L"\t");
	}
}

void jdbif_dump_map_block(struct jdb_handle *h, jdb_bid_t bid)
{
	struct jdb_map map;
	int ret;

	map.entry =
	    (struct jdb_map_blk_entry *)malloc(h->hdr.map_bent *
					       sizeof(struct
						      jdb_map_blk_entry));
	map.bid = bid;

	wprintf(L"reading map block...");

	ret = _jdb_read_map_blk(h, &map);

	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		free(map.entry);
		return;

	} else
		wprintf(L"\t[DONE]\n");

	wprintf(L"Map Dump{\n");

	jdbif_dump_map_hdr(h, map.hdr);

	jdbif_dump_map_entries_all(h, &map);
	
	free(map.entry);

	wprintf(L"}Map Dump;\n");

}

void jdbif_dump_table_def_block(struct jdb_handle* h, jdb_bid_t bid){
	struct jdb_table table;
	int ret;
	table.table_def_bid = bid;
	
	wprintf(L"reading table_def block...");
	ret = _jdb_read_table_def_blk(h, &table);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");
	
	wprintf(L"Table Defintion Dump{\n");
	jdbif_dump_table_def(&table.main);
	_jdb_free_table_def(&table.main);
	wprintf(L"}Table Defintion Dump;\n");
}

void jdbif_dump_celldef_entry(struct jdb_handle* h, struct jdb_celldef_blk_entry* celldef){
	wprintf(L"R: %u\tC: %u\tDCRC: 0x%04x\tDTYP: 0x%02x\nDLEN: %u\tDPTR: %u\tDPTRE: %u\nACC: %i-%i-%i %i:%i:%i\n"
	, celldef->row, celldef->col,
	celldef->data_crc32, celldef->data_type,
	celldef->datalen, celldef->bid_entry, celldef->bent,
	(int)celldef->last_access_d[0]+BASE_YEAR,
	(int)celldef->last_access_d[1],
	(int)celldef->last_access_d[2],
	(int)celldef->last_access_t[0],
	(int)celldef->last_access_t[1],
	(int)celldef->last_access_t[2]);
}

void jdbif_dump_cell_def_block(struct jdb_handle* h, jdb_bid_t bid){
	struct jdb_celldef_blk blk;
	int ret;
	blk.bid = bid;
	
	wprintf(L"reading table_def block...");
	ret = _jdb_read_celldef_blk(h, &blk);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");
	
	wprintf(L"Cell Defintion Dump{\n");
	wprintf
	    (L"\tType: 0x%02x\tFlags: 0x%02x\tNSet: ? of %u\tCRC32: 0x%04x\n",
	     blk.hdr.type, blk.hdr.flags, h->hdr.celldef_bent, blk.hdr.crc32);	
	for(ret = 0; ret < h->hdr.celldef_bent; ret++){
		jdbif_dump_celldef_entry(h, &blk.entry[ret]);
	}
	wprintf(L"}Cell Defintion Dump;\n");
}


void jdbif_dump_typedef_entries_all(struct jdb_handle *h, struct jdb_typedef_blk* blk){
	jdb_bent_t bent;
	wprintf(L"\t[slot]-> flags; type_id; base_type_id; type_length (chunksize if VAR flag is set)\n");
	for(bent = 0; bent < h->hdr.typedef_bent; bent++){
		//if(blk->entry[bent].type_id != JDB_TYPE_EMPTY){
			wprintf(L"[%u]-> 0x%02x; 0x%02x; 0x%02x; %u", bent,
				blk->entry[bent].flags, blk->entry[bent].type_id,
				blk->entry[bent].base, blk->entry[bent].len);
			if (!(bent % 2))
				wprintf(L"\n");
			else
				wprintf(L"\t");
		//}
	}
}


void jdbif_dump_typedef_block(struct jdb_handle* h, jdb_bid_t bid){
	struct jdb_typedef_blk blk;
	int ret;
	
	blk.bid = bid;
	blk.entry = (struct jdb_typedef_blk_entry*)malloc(sizeof(struct jdb_typedef_blk_entry)*h->hdr.typedef_bent);
	
	wprintf(L"reading typedef block...");
	ret = _jdb_read_typedef_blk(h, &blk);

	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");
	
	wprintf(L"Type Defintion Dump{\n");
	jdbif_dump_typedef_hdr(h, blk.hdr);
	jdbif_dump_typedef_entries_all(h, &blk);
	if(blk.entry) free(blk.entry);	
	wprintf(L"}Type Defintion Dump;\n");
	
}

void jdbif_dump_col_typedef_entries_all(struct jdb_handle* h, struct jdb_col_typedef* blk){
	jdb_bent_t bent;
	wprintf(L"[slot]-> type_id; column_id\n");
	for(bent = 0; bent < h->hdr.col_typedef_bent; bent++){
		//if(blk->entry[bent].type_id != JDB_TYPE_EMPTY){
			wprintf(L"[%u]-> 0x%02x; %u", bent, blk->entry[bent].type_id,
				blk->entry[bent].col);
			if (!(bent % 4))
				wprintf(L"\n");
			else
				wprintf(L"\t");
		//}			
	}
}


void jdbif_dump_col_typedef_block(struct jdb_handle* h, jdb_bid_t bid){
	struct jdb_col_typedef blk;
	int ret;
	
	blk.bid = bid;
	blk.entry = (struct jdb_col_typedef_blk_entry*)malloc(sizeof(struct jdb_col_typedef_blk_entry)*h->hdr.col_typedef_bent);
	
	wprintf(L"reading typedef block...");
	ret = _jdb_read_col_typedef_blk(h, &blk);

	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");
	
	wprintf(L"Column-Type Dump{\n");
	jdbif_dump_col_typedef_hdr(h, blk.hdr);
	jdbif_dump_col_typedef_entries_all(h, &blk);
	if(blk.entry) free(blk.entry);	
	wprintf(L"}Column-Type Dump;\n");
	
}


void jdbif_dump_block(struct jdb_handle *h)
{

	int ret;
	jdb_bid_t bid;
	uchar btype;
	uchar flags;
	uchar *block = _jdb_get_blockbuf(h, JDB_BTYPE_VOID);

	wprintf(L"#bid? ");
	wscanf(L"%u", &bid);

	wprintf(L"reading raw block #%u...", bid);

	ret = _jdb_read_block(h, block, bid);

	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		_jdb_release_blockbuf(h, block);
		return;

	} else
		wprintf(L"\t[DONE]\n");

	btype = block[0];
	flags = block[1];
	_jdb_release_blockbuf(h, block);

	wprintf(L"Type: 0x%02x\tFlags: 0x%02x\n", btype, flags);

	switch (btype) {
	case JDB_BTYPE_MAP:
		jdbif_dump_map_block(h, bid);
		break;
	case JDB_BTYPE_TYPE_DEF:
		//wprintf(L"Not Implemented!\n");
		jdbif_dump_typedef_block(h, bid);
		break;
	case JDB_BTYPE_COL_TYPEDEF:
		//wprintf(L"Not Implemented!\n");
		jdbif_dump_col_typedef_block(h, bid);
		break;
	case JDB_BTYPE_TABLE_DEF:
		jdbif_dump_table_def_block(h, bid);
		break;
	case JDB_BTYPE_CELLDEF:
		jdbif_dump_cell_def_block(h, bid);
		break;
	case JDB_BTYPE_CELL_DATA_VAR:		
		jdbif_dump_data_block(h, bid);
		break;
	case JDB_BTYPE_CELL_DATA_FIX:	
		jdbif_dump_data_block(h, bid);
		break;
	case JDB_BTYPE_CELL_DATA_PTR:
		jdbif_dump_dptr_blk(h, bid);
		break;
	case JDB_BTYPE_INDEX:
		wprintf(L"Not Implemented!\n");
		//jdbif_dump_index_block(h, bid);
		break;
	case JDB_BTYPE_TABLE_FAV:
		jdbif_dump_fav_blk(h, bid);
		break;
	default:	
		wprintf(L"ERROR: block type 0x%02x not understood!\n", btype);
		break;
	}
}

void jdbif_dump_map(struct jdb_handle *h)
{
	struct jdb_map *map;
	map = h->map_list.first;
	while (map) {
		jdbif_dump_map_hdr(h, map->hdr);
		jdbif_dump_map_entries_all(h, map);
		map = map->next;
	}
}

void jdbif_taddz(struct jdb_handle* h){
	wchar_t name[100];
	unsigned int n, i;
	int ret;
	
	wprintf(L"how many tables? ");
	wscanf(L"%u", &n);
	
	for(i = 1; i <= n; i++){
		//swprintf(name, 100, L"ta%u", i);
		//ret = jdb_create_table(h, name, 10, 10, 0, 0);
		ret = -JE_IMPLEMENT;
		if(ret < 0){
			wprintf(L"[%ls, %i] ", name, ret);
		} else {
			wprintf(L"%ls ", name);
		}
		if(i%5) wprintf(L"\n");
	}	
}

void jdbif_type_addz(struct jdb_handle* h){
	unsigned int n, i;
	int ret;
	struct jdb_table* table;
	uchar typeid = JDB_TYPE_NULL + 0x01;
	
	wprintf(L"how many types to add to each table (1 ~ %i)? ", 255 - JDB_TYPE_NULL);
	wscanf(L"%u", &n);
	
	for(table = h->table_list.first; table; table = table->next){
		for(i = 1; i <= n; i++){
			ret = jdb_add_typedef(h, table->main.name, typeid,
						JDB_TYPE_RAW, i, 
						i%2==0?0:JDB_TYPE_VAR);
			if(ret < 0){
				wprintf(L"[I%i:L%i:F%x]@%ls, R%i ", typeid, i, i%2==0?0:JDB_TYPE_VAR, table->main.name, ret);
			} else {
				wprintf(L"I%i:L%i:F%x@%ls ", typeid, i, i%2==0?0:JDB_TYPE_VAR, table->main.name);
			}
			if(i%3) wprintf(L"\n");
		}
	}		
}

void jdbif_tc_addz(struct jdb_handle* h){
	unsigned int n, i;
	int ret;
	struct jdb_table* table;
	uchar typeid = JDB_TYPE_NULL + 0x01;
	
	wprintf(L"how many column-types to add to each table ? ");
	wscanf(L"%u", &n);
	
	for(table = h->table_list.first; table; table = table->next){
		for(i = 1; i <= n; i++){
			/*
			ret = jdb_assign_col_type(h, table->main.name, typeid,
						JDB_TYPE_RAW, i, 
						i%2==0?0:JDB_TYPEDEF_VDATA);
			*/
			ret = -JE_IMPLEMENT;
			if(ret < 0){
				wprintf(L"[I%i:L%i:F%x]@%ls, R%i ", typeid, i, i%2==0?0:JDB_TYPE_VAR, table->main.name, ret);
			} else {
				wprintf(L"I%i:L%i:F%x@%ls ", typeid, i, i%2==0?0:JDB_TYPE_VAR, table->main.name);
			}
			if(i%3) wprintf(L"\n");
		}
	}		
}


int jdbif_debug_help()
{
	wprintf(L"\nSupported commands:\n\n");
	wprintf(L"\tBDUMP\tDump block\n");
	wprintf(L"\tMAPDUMP\tDump map\n");
	wprintf(L"\tTADDZ\tAdd Many tables!\n");
	wprintf(L"\tTYPADDZ\tAdd Many types!\n");
	wprintf(L"\tTCADDZ\tAdd Many column-type!\n");
	wprintf(L"\tEXIT\tExit sub-system\n");
}

int jdbif_debug(struct jdb_handle *h)
{

	wchar_t cmd[MAX_CMD];
	int ret;
	char jerr[MAX_ERR_STR];

	wprintf
	    (L"\ndebugging sub-system, type \'HELP\' to get a list of commands.\n");

 prompt1:
	jdbif_set_prompt(L"Debug >");
	jdbif_prompt();
	wscanf(L"%ls", cmd);

	switch (lookup_cmd(cmd)) {
	case HELP:
		jdbif_debug_help();
		break;
	case BDUMP:
		jdbif_dump_block(h);
		break;
	case MAPDUMP:
		jdbif_dump_map(h);
		break;
	case TADDZ:
		jdbif_taddz(h);
		break;
	case TYPADDZ:
		jdbif_type_addz(h);
		break;
	case TCADDZ:
		jdbif_tc_addz(h);
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
