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

void jdbif_dump_map_hdr(struct jdb_handle *h, struct jdb_map_blk_hdr hdr)
{
	wprintf
	    (L"\tType: 0x%02x\tFlags: 0x%02x\tNSet: %u of %u\tCRC32: 0x%04x\n",
	     hdr.type, hdr.flags, hdr.nset, h->hdr.map_bent, hdr.crc32);
}

void jdbif_dump_map_entries(struct jdb_handle *h, struct jdb_map *map)
{
	jdb_bent_t i;
	wprintf
	    (L"\tslot -> block_type; data_type; full_entries; table_id; bid\n");
	for (i = 0; i < h->hdr.map_bent; i++) {
		wprintf(L"\t%u -> 0x%02x; 0x%02x; %u; %u; #%u",
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

	jdbif_dump_map_entries(h, &map);
	
	free(map.entry);

	wprintf(L"}Map Dump;\n");

}

void jdbif_dump_tdef_main_block(struct jdb_handle* h, jdb_bid_t bid){
	struct jdb_table table;
	int ret;
	table.tdef_main_bid = bid;
	
	wprintf(L"reading tdef_main block...");
	ret = _jdb_read_tdef_main_blk(h, &table);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");
	
	wprintf(L"Table Defintion Dump{\n");
	jdbif_dump_tdef_main(&table.main);
	_jdb_free_tdef_main(&table.main);
	wprintf(L"}Table Defintion Dump;\n");
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
	jdbif_dump_typedef_entries(h, &blk);
	if(blk.entry) free(blk.entry);	
	wprintf(L"}Type Defintion Dump;\n");
	
}

void jdbif_dump_tdef_block(struct jdb_handle* h, jdb_bid_t bid){
	struct jdb_tdef blk;
	int ret;
	
	blk.bid = bid;
	blk.entry = (struct jdb_tdef_blk_entry*)malloc(sizeof(struct jdb_tdef_blk_entry)*h->hdr.tdef_bent);
	
	wprintf(L"reading typedef block...");
	ret = _jdb_read_tdef_blk(h, &blk);

	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");
	
	wprintf(L"Column-Type Dump{\n");
	jdbif_dump_tdef_hdr(h, blk.hdr);
	jdbif_dump_tdef_entries(h, &blk);
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
	case JDB_BTYPE_TDEF:
		//wprintf(L"Not Implemented!\n");
		jdbif_dump_tdef_block(h, bid);
		break;
	case JDB_BTYPE_TDEF_MAIN:
		jdbif_dump_tdef_main_block(h, bid);
		break;
	case JDB_BTYPE_CELLDEF:
		wprintf(L"Not Implemented!\n");
		break;
	case JDB_BTYPE_CELL_DATA_VAR:
		wprintf(L"Not Implemented!\n");
		break;
	case JDB_BTYPE_CELL_DATA_FIX:
		wprintf(L"Not Implemented!\n");
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
		jdbif_dump_map_entries(h, map);
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
		swprintf(name, 100, L"ta%u", i);		
		ret = jdb_create_table(h, name, 10, 10, 0, 0);
		if(ret < 0){
			wprintf(L"[%S, %i] ", name, ret);
		} else {
			wprintf(L"%S ", name);
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
						i%2==0?0:JDB_TYPEDEF_VDATA);
			if(ret < 0){
				wprintf(L"[I%i:L%i:F%x]@%S, R%i ", typeid, i, i%2==0?0:JDB_TYPEDEF_VDATA, table->main.name, ret);
			} else {
				wprintf(L"I%i:L%i:F%x@%S ", typeid, i, i%2==0?0:JDB_TYPEDEF_VDATA, table->main.name);
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
			ret = jdb_assign_col_type(h, table->main.name, typeid,
						JDB_TYPE_RAW, i, 
						i%2==0?0:JDB_TYPEDEF_VDATA);
			if(ret < 0){
				wprintf(L"[I%i:L%i:F%x]@%S, R%i ", typeid, i, i%2==0?0:JDB_TYPEDEF_VDATA, table->main.name, ret);
			} else {
				wprintf(L"I%i:L%i:F%x@%S ", typeid, i, i%2==0?0:JDB_TYPEDEF_VDATA, table->main.name);
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
	wscanf(L"%S", cmd);

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
