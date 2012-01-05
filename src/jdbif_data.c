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

void jdbif_print_data(uchar* buf, size_t bufsize, jdb_data_t base){
	size_t i;
	wprintf(L"Data {\n");
	switch(base){
		case JDB_TYPE_EMPTY:
			wprintf(L"[EMPTY]");
			break;
		case JDB_TYPE_BYTE:
			for(i = 0; i < bufsize; i++){				
				wprintf(L"%c", buf[i]);
			}
			break;
		case JDB_TYPE_SHORT:
			for(i = 0; i < bufsize; i += 2){
				wprintf(L"%s", (short)(*(buf+i)));
			}
			break;
		case JDB_TYPE_LONG:
			for(i = 0; i < bufsize; i += 4){
				wprintf(L"%l", (long)(*(buf+i)));
			}
			break;
		case JDB_TYPE_LONG_LONG:
			for(i = 0; i < bufsize; i += 8){
				wprintf(L"%d", (long long)(*(buf+i)));
			}
			break;
		case JDB_TYPE_DOUBLE:
			for(i = 0; i < bufsize; i += 8){
				wprintf(L"%d", (double)(*(buf+i)));
			}
			break;
		case JDB_TYPE_CHAR:
		case JDB_TYPE_JCS:
			wprintf(L"%s", buf);
			break;
		case JDB_TYPE_WIDE:
			wprintf(L"%ls", (wchar_t*)buf);
			break;
		case JDB_TYPE_RAW:
			wprintf(L"[RAW]");
			break;
		default:
			wprintf(L"Bad Type: 0x%02x", base);
			default:	
					
	}
	wprintf(L"\n} Data;\n");
}

void jdbif_dump_data_blk_hdr(struct jdb_handle* h, struct jdb_data_blk_hdr hdr){
	wprintf
	    (L"\tType: 0x%02x\tFlags: 0x%02x\tDataType: 0x%02x\tCRC32: 0x%04x\n",
	     hdr.type, hdr.flags, hdr.dtype, hdr.crc32);			
}

void jdbif_dump_data_blk_info(struct jdb_data_blk* blk){
	wprintf(L"\tEntrySize: %u\tBaseSize: %u\tBitmapSize: %u\tNoOfEntries: %u\n\tDataType: 0x%02x\tBaseType: 0x%02x\tMaxEntries: %u\n",
		blk->entsize, blk->base_len, blk->bmapsize, blk->nent, blk->data_type, blk->base_type, blk->maxent);
}

void jdbif_dump_data_blk_bmap(struct jdb_data_blk* blk){
	jif_dump_bmap_w(blk->bitmap, blk->bmapsize);
}

void jdbif_dump_data_block(struct jdb_handle* h, jdb_bid_t bid){
	struct jdb_data_blk blk;
	int ret;
	struct jdb_map_blk_entry* m_ent;
	
	blk.bid = bid;
	
	m_ent = _jdb_get_map_entry_ptr(h, bid);
	if(!m_ent) {
		wprintf(L"No Entry\n");
		return;
	}

	ret = _jdb_read_data_blk(h, table, &blk, m_ent->dtype);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");		
	return 0;
	
	wprintf(L"Data Block{\n");
	jdbif_dump_data_blk_hdr(h, blk->hdr);
	jdbif_dump_data_blk_info(blk);
	jdbif_dump_data_blk_bmap(blk);
	jdbif_print_data(blk->datapool, blk->bufsize, blk->base_type);
	wprintf(L"} Data Block;\n");
}

void jdbif_dump_data_blk_hdr(struct jdb_handle* h, struct jdb_data_blk_hdr hdr){
	wprintf
	    (L"\tType: 0x%02x\tFlags: 0x%02x\tDataType: 0x%02x\tCRC32: 0x%04x\n",
	     hdr.type, hdr.flags, hdr.dtype, hdr.crc32);			
}
/*
void jdbif_dump_data_ptr_chain(struct jdb_cell_data_ptr_blk_entry *first){
	struct jdb_cell_data_ptr_blk_entry *dptr;
	size_t i = 0UL;
	
	wprintf(L"Format: [Entry]:BID:BENT:NENT->NextDptrBid:NextDptrBent --\n");
	
	for(dptr = first; dptr; dptr = dptr->next){
		wprintf(L"[%u]:%u:%u->%u:%u -- ", i++,
			dptr->bid, dptr->bent, dptr->nent, dptr->nextdptrbid,
			dptr->nextdptrbent);		
	}
}

void jdbif_dump_data_ptr_block_hdr(struct jdb_cell_data_ptr_blk_hdr hdr){
	wprintf
	    (L"\tType: 0x%02x\tFlags: 0x%02x\tCRC32: 0x%04x\n",
	     hdr.type, hdr.flags, hdr.crc32);
}


void jdbif_dump_data_ptr_block_entries(	struct jdb_handle* h,
					struct jdb_cell_data_ptr_blk* blk){
	jdb_bent_t i;
	wprintf(L"Format: [Entry]:BID:BENT:NENT->NextDptrBid:NextDptrBent --\n");	
	for(i = 0; i < h->hdr.dptr_bent; i++){
		wprintf(L"[%u]:%u:%u->%u:%u -- ", i,blk->entry[i].bid, 
		blk->entry[i].bent, blk->entry[i].nent,
		blk->entry[i].nextdptrbid, blk->entry[i].nextdptrbent);
	}
	wprintf(L"\n");
}

void jdbif_dump_data_ptr_block(struct jdb_handle* h, jdb_bid_t bid){
	
}
*/

void jdbif_dump_fav_blk_hdr(struct jdb_handle* h, struct jdb_fav_blk_hdr hdr){
	wprintf
	    (L"\tType: 0x%02x\tFlags: 0x%02x\tCRC32: 0x%04x\n",
	     hdr.type, hdr.flags, hdr.crc32);			
}

