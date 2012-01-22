#include "jdb.h"
#include "jpack.h"

/*
	all pack/unpack functions must set/check crc too!
	** ALL VALIDITY CHECKS MUST BE DONE BEFORE CALLING THIESE METHODS
*/

#define _wdeb_crc _wdeb

int _jdb_pack_hdr(struct jdb_hdr *hdr, uchar * buf)
{

/*
	uint16_t magic;
	uchar type;
	uint32_t ver;

	//configuration
	uint16_t flags;
	uchar crc_type;
	uchar crypt_type;

	uchar wr_retries;
	uchar rd_retries;

	jdb_bsize_t blocksize;	//must be multiplier of 2^10 (1024)

	//limits
	jdb_bid_t max_blocks;

	//block entries
	jdb_bent_t map_bent;
	jdb_bent_t typedef_bent;
	jdb_bent_t col_typedef_bent;
	jdb_bent_t celldef_bent;
	jdb_bent_t index1_bent;
	jdb_bent_t dptr_bent;
	jdb_bent_t fav_bent;
	jdb_bent_t index0_bent;
	
	jdb_bid_t map_list_buck;
	jdb_bid_t list_buck;
	jdb_bid_t fav_load;	

	//status
	jdb_bid_t nblocks;	//number of blocks
	jdb_bid_t nmaps;	//number of map blocks
	jdb_tid_t ntables;
	uint64_t nwr;		//total number of write requests
	
	uchar pwhash[32];	//sha256
	
	//PAD
	
	uint32_t crc32;		//header crc

*/

	pack(buf, "hclhcccchlhhhhhhhhllllllt",
		h->hdr.magic,
		h->hdr.type,
		h->hdr.ver,
		h->hdr.flags,
		h->hdr.crc_type,
		h->hdr.crypt_type,
		h->hdr.wr_retries,
		h->hdr.rd_retries,
		h->hdr.blocksize,
		h->hdr.max_blocks,
		h->hdr.map_bent,
		h->hdr.typedef_bent,
		h->hdr.col_typedef_bent,
		h->hdr.celldef_bent,
		h->hdr.index1_bent,
		h->hdr.dptr_bent,
		h->hdr.fav_bent,
		h->hdr.index0_bent,
		h->hdr.map_list_buck,
		h->hdr.list_buck,
		h->hdr.fav_load,
		h->hdr.nblocks,
		h->hdr.nmaps,
		h->hdr.ntables,
		h->hdr.nwr
		);
	memcpy(buf+		
	
	return 0;
}

int _jdb_unpack_hdr(struct jdb_hdr *hdr, uchar * buf)
{
	
	return 0;
}

int _jdb_pack_map(struct jdb_handle* h, struct jdb_map *blk, uchar * buf)
{
	jdb_bent_t i;
	jdb_bsize_t pos;
	uint32_t crc32;
	
	if(h->hdr.crc_type != JDB_CRC_NONE){
		//this is strange... but fixes CRC errors!!!		
		memset(buf, '\0', h->hdr.blocksize);
	}	

	pack(buf, "cchl",
	     blk->hdr.type, blk->hdr.flags, blk->hdr.nset, 0x00000000);

	pos = sizeof(struct jdb_map_blk_hdr);

	for (i = 0; i < h->hdr.map_bent; i++) {
		pack(buf + pos, "cchl",
		     blk->entry[i].blk_type,
		     blk->entry[i].dtype,
		     blk->entry[i].nful, blk->entry[i].tid);
		pos += sizeof(struct jdb_map_blk_entry);
	}

	if(h->hdr.crc_type != JDB_CRC_NONE){
		crc32 = _jdb_crc32(buf, h->hdr.blocksize);
		pack(buf + sizeof(struct jdb_map_blk_hdr) - sizeof(uint32_t), "l",
		     crc32);
	}

	return 0;
}

int _jdb_unpack_map(struct jdb_handle* h, struct jdb_map *blk, uchar * buf)
{
	jdb_bent_t i;
	jdb_bsize_t pos;
	uint32_t crc32;

	if(h->hdr.crc_type != JDB_CRC_NONE){
		unpack(buf + sizeof(struct jdb_map_blk_hdr) - sizeof(uint32_t), "l",
		       &crc32);
		pack(buf + sizeof(struct jdb_map_blk_hdr) - sizeof(uint32_t), "l",
		     0x00000000);
		if (crc32 != _jdb_crc32(buf, h->hdr.blocksize))
			return -JE_CRC;
	}

	unpack(buf, "cch", &blk->hdr.type, &blk->hdr.flags, &blk->hdr.nset);

	if(h->hdr.crc_type != JDB_CRC_NONE)
		blk->hdr.crc32 = crc32;
	
	if(blk->hdr.type != JDB_BTYPE_MAP) return -JE_TYPE;

	pos = sizeof(struct jdb_map_blk_hdr);

	for (i = 0; i < h->hdr.map_bent; i++) {
		unpack(buf + pos, "cchl",
		       &blk->entry[i].blk_type,
		       &blk->entry[i].dtype,
		       &blk->entry[i].nful, &blk->entry[i].tid);

		pos += sizeof(struct jdb_map_blk_entry);
	}

	return 0;
}

int _jdb_pack_table_def(struct jdb_handle* h, struct jdb_table_def_blk *blk,
			uchar * buf)
{
	
	uint32_t crc32;
/*	
	if(blk->hdr.namelen > h->hdr.blocksize - (sizeof(jdb_table_def_blk_hdr)+
		 1)) return -JE_TOOLONG;	
	if(blk->name){
		
	} else {
		if(blk->hdr.namelen) return -JE_FORBID;
	}
*/
	//memset(buf, '\0', h->hdr.blocksize);
	
	if(h->hdr.crc_type != JDB_CRC_NONE){
		//this is strange... but fixes CRC errors!!!		
		memset(buf, '\0', h->hdr.blocksize);
	}	
		
	pack(buf, "cchlhlltll",
	     blk->hdr.type,
	     blk->hdr.flags,
	     blk->hdr.indexes,
	     blk->hdr.tid,
	     blk->hdr.namelen,
	     blk->hdr.nrows,
	     blk->hdr.ncols, blk->hdr.ncells, blk->hdr.ncol_typedef, 0UL);
	     
	//_wdump(buf, sizeof(struct jdb_table_def_blk_hdr));

	if (blk->name) {
	
		if (wtojcs
		    (blk->name, buf + sizeof(struct jdb_table_def_blk_hdr),
		     blk->hdr.namelen) == ((size_t) - 1))
			return -JE_NOJCS;
	
	//	memcpy(buf + sizeof(struct jdb_table_def_blk_hdr), blk->name, blk->hdr.namelen+1);			
	}
	
	if(h->hdr.crc_type != JDB_CRC_NONE){
		crc32 = _jdb_crc32(buf, h->hdr.blocksize);
		_wdeb_crc(L"CRC32: 0x%08x", crc32);
	
		pack(buf + sizeof(struct jdb_table_def_blk_hdr) - sizeof(uint32_t), "l",
			crc32);
	}
		
	return 0;
}

int _jdb_unpack_table_def(struct jdb_handle* h, struct jdb_table_def_blk *blk,
			uchar * buf)
{

	uint32_t crc32;
	uint32_t calccrc32;
	
	if(h->hdr.crc_type != JDB_CRC_NONE){
		unpack(buf + sizeof(struct jdb_table_def_blk_hdr) - sizeof(uint32_t),
			"l", &crc32);

		pack(buf + sizeof(struct jdb_table_def_blk_hdr) - sizeof(uint32_t),
			"l", 0UL);
	
	//_wdump(buf, sizeof(struct jdb_table_def_blk_hdr));		

		calccrc32 = _jdb_crc32(buf, h->hdr.blocksize);
		

		if (crc32 != calccrc32){
			_wdeb_crc(L"CRC_ERROR! CRC: 0x%08x, CRC_CALC: 0x%08x",
					crc32, calccrc32);

			return -JE_HCRC;
		}
	}

	unpack(buf, "cchlhlltl",
	       &blk->hdr.type,
	       &blk->hdr.flags,
	       &blk->hdr.indexes,
	       &blk->hdr.tid,
	       &blk->hdr.namelen,
	       &blk->hdr.nrows,
	       &blk->hdr.ncols, &blk->hdr.ncells, &blk->hdr.ncol_typedef);
	       
	if(blk->hdr.type != JDB_BTYPE_TABLE_DEF) return -JE_TYPE;

	if(h->hdr.crc_type != JDB_CRC_NONE)
		blk->hdr.crc32 = crc32;

	if (blk->hdr.namelen) {
		blk->name =
		    (wchar_t *) malloc((blk->hdr.namelen + 1) *
				       sizeof(wchar_t));
				       
		if (jcstow
		    (buf + sizeof(struct jdb_table_def_blk_hdr), blk->name,
		     blk->hdr.namelen) == ((size_t) - 1)) {
			free(blk->name);
			blk->name = NULL;
			return -JE_NOJCS;
		}

//		memcpy(blk->name, buf+sizeof(struct jdb_table_def_blk_hdr), blk->hdr.namelen+1);		
	} else {
		blk->name = NULL;
	}

	return 0;
}


int _jdb_pack_typedef(struct jdb_handle* h, struct jdb_typedef_blk *blk,
			uchar * buf)
{
	jdb_bent_t i;
	jdb_bsize_t pos;
	uint32_t crc32;
/*	
	if(h->hdr.crc_type != JDB_CRC_NONE){
		//this is strange... but fixes CRC errors!!!		
		memset(buf, '\0', h->hdr.blocksize);
	}
*/
	if(h->hdr.crc_type != JDB_CRC_NONE){
		//this is strange... but fixes CRC errors!!!		
		memset(buf, '\0', h->hdr.blocksize);
	}

	pack(buf, "ccl", blk->hdr.type, blk->hdr.flags, 0x00000000);

	pos = sizeof(struct jdb_typedef_blk_hdr);

	for (i = 0; i < h->hdr.typedef_bent; i++) {
		pack(buf + pos, "cccl",
		     blk->entry[i].flags,
		     blk->entry[i].type_id,
		     blk->entry[i].base, blk->entry[i].len);
		pos += sizeof(struct jdb_typedef_blk_entry);
	}	

	if(h->hdr.crc_type != JDB_CRC_NONE){
		crc32 = _jdb_crc32(buf, h->hdr.blocksize);
		
		_wdeb_crc(L"CRC32: 0x%08x",
				crc32);
		
		pack(buf + sizeof(struct jdb_typedef_blk_hdr) - sizeof(uint32_t), "l",
		     crc32);
	}

	return 0;
}

int _jdb_unpack_typedef(struct jdb_handle* h, struct jdb_typedef_blk *blk,
			uchar * buf)
{
	jdb_bent_t i;
	jdb_bsize_t pos;
	uint32_t crc32;

	if(h->hdr.crc_type != JDB_CRC_NONE){
									
		unpack(buf + sizeof(struct jdb_typedef_blk_hdr) - sizeof(uint32_t), "l",
		       &crc32);
		pack(buf + sizeof(struct jdb_typedef_blk_hdr) - sizeof(uint32_t), "l",
		     0x00000000);
		     
		_wdeb_crc(L"CRC32: 0x%08x, CRC_CALC32: 0x%08x",
				crc32, _jdb_crc32(buf, h->hdr.blocksize));
				
		if (crc32 != _jdb_crc32(buf, h->hdr.blocksize))
			return -JE_CRC;
	}

	unpack(buf, "cc", &blk->hdr.type, &blk->hdr.flags);
	
	if(blk->hdr.type != JDB_BTYPE_TYPE_DEF) return -JE_TYPE;

	if(h->hdr.crc_type != JDB_CRC_NONE)
		blk->hdr.crc32 = crc32;

	pos = sizeof(struct jdb_typedef_blk_hdr);

	for (i = 0; i < h->hdr.typedef_bent; i++) {
		unpack(buf + pos, "cccl",
		       &blk->entry[i].flags,
		       &blk->entry[i].type_id,
		       &blk->entry[i].base, &blk->entry[i].len);
		pos += sizeof(struct jdb_typedef_blk_entry);
	}

	return 0;
}

int _jdb_pack_col_typedef(struct jdb_handle* h, struct jdb_col_typedef *blk,
			uchar * buf)
{
	jdb_bent_t i;
	jdb_bsize_t pos;
	uint32_t crc32;
	
	if(h->hdr.crc_type != JDB_CRC_NONE){
		//this is strange... but fixes CRC errors!!!		
		memset(buf, '\0', h->hdr.blocksize);
	}	

	pack(buf, "ccl", blk->hdr.type, blk->hdr.flags, 0x00000000);

	pos = sizeof(struct jdb_col_typedef_blk_hdr);

	for (i = 0; i < h->hdr.col_typedef_bent; i++) {
		pack(buf + pos, "cl", blk->entry[i].type_id, blk->entry[i].col);
		pos += sizeof(struct jdb_col_typedef_blk_entry);
	}
	
	if(h->hdr.crc_type != JDB_CRC_NONE){
		crc32 = _jdb_crc32(buf, h->hdr.blocksize);
		pack(buf + sizeof(struct jdb_col_typedef_blk_hdr) - sizeof(uint32_t), "l",
		     crc32);
	}
	
	return 0;
}

int _jdb_unpack_col_typedef(struct jdb_handle* h, struct jdb_col_typedef *blk,
				uchar * buf)
{
	jdb_bent_t i;
	jdb_bsize_t pos;
	uint32_t crc32;

	if(h->hdr.crc_type != JDB_CRC_NONE){
		unpack(buf + sizeof(struct jdb_col_typedef_blk_hdr) - sizeof(uint32_t), "l",
		       &crc32);
		pack(buf + sizeof(struct jdb_col_typedef_blk_hdr) - sizeof(uint32_t), "l",
		     0x00000000);
		if (crc32 != _jdb_crc32(buf, h->hdr.blocksize))
			return -JE_CRC;
	}
	
	unpack(buf, "cc", &blk->hdr.type, &blk->hdr.flags);
	
	if(blk->hdr.type != JDB_BTYPE_COL_TYPEDEF) return -JE_TYPE;

	if(h->hdr.crc_type != JDB_CRC_NONE)
		blk->hdr.crc32 = crc32;

	pos = sizeof(struct jdb_col_typedef_blk_hdr);

	for (i = 0; i < h->hdr.col_typedef_bent; i++) {
		unpack(buf + pos, "cl",
		       &blk->entry[i].type_id, &blk->entry[i].col);
		pos += sizeof(struct jdb_col_typedef_blk_entry);
	}

	return 0;
}


int _jdb_pack_celldef(	struct jdb_handle* h, struct jdb_celldef_blk *blk,
			uchar * buf)
{
	jdb_bent_t i;
	jdb_bsize_t pos;
	uint32_t crc32;
	
	if(h->hdr.crc_type != JDB_CRC_NONE){
		//this is strange... but fixes CRC errors!!!		
		memset(buf, '\0', h->hdr.blocksize);
	}	

	pack(buf, "ccl", blk->hdr.type, blk->hdr.flags, 0x00000000);

	pos = sizeof(struct jdb_celldef_blk_hdr);

	for (i = 0; i < h->hdr.celldef_bent; i++) {
		pack(buf + pos, "lllcllhcccccc",
		     blk->entry[i].row,
		     blk->entry[i].col,
		     blk->entry[i].data_crc32,
		     blk->entry[i].data_type,
		     blk->entry[i].datalen,
		     blk->entry[i].bid_entry,
		     blk->entry[i].bent,
		     blk->entry[i].last_access_d[0],
		     blk->entry[i].last_access_d[1],
		     blk->entry[i].last_access_d[2],
		     blk->entry[i].last_access_t[0],
		     blk->entry[i].last_access_t[1],
		     blk->entry[i].last_access_t[2]);
		pos += sizeof(struct jdb_celldef_blk_entry);
	}
	
	if(h->hdr.crc_type != JDB_CRC_NONE){
		crc32 = _jdb_crc32(buf, h->hdr.blocksize);
		pack(buf + sizeof(struct jdb_celldef_blk_hdr) - sizeof(uint32_t), "l",
		     crc32);
	}
	
	return 0;
}

int _jdb_unpack_celldef(struct jdb_handle* h, struct jdb_celldef_blk *blk,
			uchar * buf)
{
	jdb_bent_t i;
	jdb_bsize_t pos;
	uint32_t crc32;

	if(h->hdr.crc_type != JDB_CRC_NONE){
		unpack(buf + sizeof(struct jdb_celldef_blk_hdr) - sizeof(uint32_t), "l",
		       &crc32);
		pack(buf + sizeof(struct jdb_celldef_blk_hdr) - sizeof(uint32_t), "l",
		     0x00000000);
		if (crc32 != _jdb_crc32(buf, h->hdr.blocksize))
			return -JE_CRC;
	}

	unpack(buf, "cc", &blk->hdr.type, &blk->hdr.flags);
	
	if(blk->hdr.type != JDB_BTYPE_CELLDEF) return -JE_TYPE;

	if(h->hdr.crc_type != JDB_CRC_NONE)
		blk->hdr.crc32 = crc32;

	pos = sizeof(struct jdb_celldef_blk_hdr);

	for (i = 0; i < h->hdr.celldef_bent; i++) {
		unpack(buf + pos, "lllcllhcccccc",
		       &blk->entry[i].row,
		       &blk->entry[i].col,
		       &blk->entry[i].data_crc32,
		       &blk->entry[i].data_type,
		       &blk->entry[i].datalen,
		       &blk->entry[i].bid_entry,
		       &blk->entry[i].bent,
		       &blk->entry[i].last_access_d[0],
		       &blk->entry[i].last_access_d[1],
		       &blk->entry[i].last_access_d[2],
		       &blk->entry[i].last_access_t[0],
		       &blk->entry[i].last_access_t[1],
		       &blk->entry[i].last_access_t[2]);
		pos += sizeof(struct jdb_celldef_blk_entry);
	}

	return 0;
}

int _jdb_pack_data_ptr(struct jdb_handle* h, struct jdb_cell_data_ptr_blk *blk,
			uchar* buf)
{
	jdb_bent_t i;
	jdb_bsize_t pos;
	uint32_t crc32;
	
	if(h->hdr.crc_type != JDB_CRC_NONE){
		//this is strange... but fixes CRC errors!!!		
		memset(buf, '\0', h->hdr.blocksize);
	}	

	pack(buf, "ccl",
	     blk->hdr.type, blk->hdr.flags, 0x00000000);

	pos = sizeof(struct jdb_cell_data_ptr_blk_hdr);

	for (i = 0; i < h->hdr.dptr_bent; i++) {
		pack(buf + pos, "lhhlh",
		     blk->entry[i].bid,
		     blk->entry[i].bent,
		     blk->entry[i].nent,
		     blk->entry[i].nextdptrbid,
		     blk->entry[i].nextdptrbent);
		pos += sizeof(struct jdb_cell_data_ptr_blk_entry);
	}

	if(h->hdr.crc_type != JDB_CRC_NONE){
		crc32 = _jdb_crc32(buf, h->hdr.blocksize);
		pack(buf + sizeof(struct jdb_cell_data_ptr_blk_hdr) - sizeof(uint32_t), "l",
		     crc32);
	}

	return 0;
}

int _jdb_unpack_data_ptr(struct jdb_handle* h, struct jdb_cell_data_ptr_blk *blk,
			uchar* buf)
{
	jdb_bent_t i;
	jdb_bsize_t pos;
	uint32_t crc32;

	if(h->hdr.crc_type != JDB_CRC_NONE){
		unpack(buf + sizeof(struct jdb_cell_data_ptr_blk_hdr) - sizeof(uint32_t), "l",
		       &crc32);
		pack(buf + sizeof(struct jdb_cell_data_ptr_blk_hdr) - sizeof(uint32_t), "l",
		     0x00000000);
		if (crc32 != _jdb_crc32(buf, h->hdr.blocksize))
			return -JE_CRC;
	}

	unpack(buf, "cc", &blk->hdr.type, &blk->hdr.flags);

	if(h->hdr.crc_type != JDB_CRC_NONE)
		blk->hdr.crc32 = crc32;
	
	if(blk->hdr.type != JDB_BTYPE_CELL_DATA_PTR) return -JE_TYPE;

	pos = sizeof(struct jdb_cell_data_ptr_blk_hdr);

	for (i = 0; i < h->hdr.dptr_bent; i++) {
		unpack(buf + pos, "lhhlh",
		     &blk->entry[i].bid,
		     &blk->entry[i].bent,
		     &blk->entry[i].nent,
		     &blk->entry[i].nextdptrbid,
		     &blk->entry[i].nextdptrbent);

		pos += sizeof(struct jdb_cell_data_ptr_blk_entry);
	}

	return 0;
}


int _jdb_pack_data(struct jdb_handle* h, struct jdb_cell_data_blk *blk,
		    uchar * buf)
{
	jdb_bent_t i, j;
	unsigned long long d;
	uint32_t crc32;
	jdb_bsize_t databufsize;
	
	if(h->hdr.crc_type != JDB_CRC_NONE){
		//this is strange... but fixes CRC errors!!!		
		memset(buf, '\0', h->hdr.blocksize);
	}	

	pack(buf, "cccl", blk->hdr.type, blk->hdr.flags, blk->hdr.dtype, 0x00000000);

	buf += sizeof(struct jdb_cell_data_blk_hdr);

	for(i = 0; i < blk->bmapsize; i++){
		*buf++ = (blk->bitmap[i]>>0)&0xff;
	}	

	databufsize = blk->nent * blk->entsize;

	for (i = 0; i < blk->maxent; i++) {
		for (j = 0; j < databufsize; j += blk->base_len) {
			switch (blk->base_type) {
			case JDB_TYPE_BYTE:
			case JDB_TYPE_CHAR:
				*buf++ = (blk->datapool[j] >> 0) & 0xff;
				break;
			case JDB_TYPE_SHORT:
				packi16(buf,
					(unsigned short)(*(blk->datapool + j)));
				buf += 2;
				break;
			case JDB_TYPE_LONG:
			case JDB_TYPE_WIDE:
				packi32(buf,
					(unsigned long)(*(blk->datapool + j)));
				buf += 4;
				break;
			case JDB_TYPE_LONG_LONG:
				packi64(buf,
					(unsigned long
					 long)(*(blk->datapool + j)));
				buf += 8;
				break;
			case JDB_TYPE_DOUBLE:
				d = pack754_64((long
						double)(*(blk->datapool + j)));
				packi64(buf, d);
				buf += 8;
				break;
			default:
				*buf++ = blk->datapool[j];
				break;
			}
		}
	}

	if(h->hdr.crc_type != JDB_CRC_NONE){
		crc32 = _jdb_crc32(buf, h->hdr.blocksize);
		
		_wdeb_crc(L"crc of packed data: 0x%08x", crc32);
		
		pack(buf + sizeof(struct jdb_cell_data_blk_hdr) - sizeof(uint32_t),
		     "l", crc32);
	}

	return 0;

}

int _jdb_unpack_data(struct jdb_handle* h, struct jdb_table* table,
			struct jdb_cell_data_blk *blk, jdb_data_t type_id, uchar * buf)
{
	uint32_t crc32;
	jdb_bsize_t databufsize;
	unsigned long long d;
	size_t pos, i, j;
	struct jdb_typedef_blk* typedef_blk;
	struct jdb_typedef_blk_entry* typedef_entry;
	int ret;
	
	if(h->hdr.crc_type != JDB_CRC_NONE){
		unpack(buf + sizeof(struct jdb_cell_data_blk_hdr) - sizeof(uint32_t),
		       "l", &crc32);
		       
		_wdeb_crc(L"crc of packed data: 0x%08x", crc32);
		
		pack(buf + sizeof(struct jdb_cell_data_blk_hdr) - sizeof(uint32_t),
		     "l", 0x00000000);
		     
		_wdeb_crc(L"crc calculated is: 0x%08x", _jdb_crc32(buf, h->hdr.blocksize));
		     
		if (crc32 != _jdb_crc32(buf, h->hdr.blocksize))
			return -JE_CRC;
	}

	unpack(buf, "ccc", &blk->hdr.type, &blk->hdr.flags, &blk->hdr.dtype);
	
	if(blk->hdr.type != JDB_BTYPE_CELL_DATA_FIX && 
	blk->hdr.type != JDB_BTYPE_CELL_DATA_VAR) return -JE_TYPE;
	
	if(type_id > JDB_TYPE_NULL){
		ret = _jdb_find_typedef(h, table, type_id, &typedef_blk, &typedef_entry);
		if(ret < 0) return ret;
	
		blk->entsize = typedef_entry->len;
		blk->data_type = typedef_entry->type_id;
		blk->base_type = typedef_entry->base;
		blk->base_len = _jdb_base_dtype_size(typedef_entry->base);
	} else {
		blk->base_type = type_id;
		blk->data_type = type_id;
		blk->entsize = _jdb_base_dtype_size(type_id);
		blk->base_len = blk->entsize;
	}
	
	_jdb_get_map_nful_by_bid(h, blk->bid, &blk->nent);
	
	if(h->hdr.crc_type != JDB_CRC_NONE){
		blk->hdr.crc32 = crc32;
	}

	if(_jdb_calc_data(h->hdr.blocksize, blk->entsize, &blk->maxent, &blk->bmapsize) < 0)
		return -JE_FORBID;
		
	blk->bitmap = (uchar*)malloc(blk->bmapsize);
	if(!blk->bitmap) return -JE_MALOC;

	pos = sizeof(struct jdb_cell_data_blk_hdr);
	buf += pos;

	for(i = 0; i < blk->bmapsize; i++){
		blk->bitmap[i] = *buf++;
	}
	
	buf += blk->bmapsize;

	databufsize = blk->maxent * blk->entsize;

	blk->datapool = (uchar*)malloc(databufsize);
	if(!blk->datapool){
		free(blk->bitmap);
		return -JE_MALOC;
	}

	for (i = 0; i < blk->maxent; i++) {
		for (j = 0; j < databufsize; j += blk->base_len) {
			switch (blk->base_type) {
			case JDB_TYPE_BYTE:
			case JDB_TYPE_CHAR:
				blk->datapool[j] = *buf++;
				break;
			case JDB_TYPE_SHORT:
				*(blk->datapool + j) = unpacki16(buf);
				buf += 2;
				break;
			case JDB_TYPE_LONG:
			case JDB_TYPE_WIDE:
				*(blk->datapool + j) = unpacki32(buf);
				buf += 4;
				break;
			case JDB_TYPE_LONG_LONG:
				*(blk->datapool + j) = unpacki64(buf);
				buf += 8;
				break;
			case JDB_TYPE_DOUBLE:
				d = unpacki64(buf);
				*(blk->datapool + j) = unpack754_64(d);
				buf += 8;
				break;
			default:
				blk->datapool[j] = *buf++;
				break;
			}
		}
	}

	return 0;

}

int _jdb_pack_fav(struct jdb_handle* h, struct jdb_fav_blk *blk,
			uchar * buf)
{
	jdb_bent_t i;
	jdb_bsize_t pos;
	uint32_t crc32;
	
	if(h->hdr.crc_type != JDB_CRC_NONE){
		//this is strange... but fixes CRC errors!!!		
		memset(buf, '\0', h->hdr.blocksize);
	}	

	pack(buf, "ccl", blk->hdr.type, blk->hdr.flags, 0x00000000);

	pos = sizeof(struct jdb_fav_blk_hdr);

	for (i = 0; i < h->hdr.fav_bent; i++) {
		pack(buf + pos, "ll",
		     blk->entry[i].bid,
		     blk->entry[i].nhits);
		pos += sizeof(struct jdb_fav_blk_entry);
	}

	if(h->hdr.crc_type != JDB_CRC_NONE){
		crc32 = _jdb_crc32(buf, h->hdr.blocksize);
		pack(buf + sizeof(struct jdb_fav_blk_hdr) - sizeof(uint32_t), "l",
		     crc32);
	}

	return 0;
}

int _jdb_unpack_fav(struct jdb_handle* h, struct jdb_fav_blk *blk,
			uchar * buf)
{
	jdb_bent_t i;
	jdb_bsize_t pos;
	uint32_t crc32;

	if(h->hdr.crc_type != JDB_CRC_NONE){
		unpack(buf + sizeof(struct jdb_fav_blk_hdr) - sizeof(uint32_t), "l",
		       &crc32);
		pack(buf + sizeof(struct jdb_fav_blk_hdr) - sizeof(uint32_t), "l",
		     0x00000000);
		if (crc32 != _jdb_crc32(buf, h->hdr.blocksize))
			return -JE_CRC;
	}

	unpack(buf, "cc", &blk->hdr.type, &blk->hdr.flags);
	
	if(blk->hdr.type != JDB_BTYPE_TABLE_FAV) return -JE_TYPE;

	if(h->hdr.crc_type != JDB_CRC_NONE)
		blk->hdr.crc32 = crc32;

	pos = sizeof(struct jdb_fav_blk_hdr);

	for (i = 0; i < h->hdr.fav_bent; i++) {
		unpack(buf + pos, "ll",
		       &blk->entry[i].bid,
		       &blk->entry[i].nhits);
		pos += sizeof(struct jdb_fav_blk_entry);
	}

	return 0;
}

