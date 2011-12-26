#include "jdb.h"

#define _wdeb_io(f, a...)
#define _wdeb_crc _wdeb
#define _wdeb_map(f, a...)

int _jdb_write_block(struct jdb_handle *h, uchar * block, jdb_bid_t bid,
		     uchar flags)
{

	int ret;

	_wdeb_io(L"WRITE block < %u >", bid);

	if (h->hdr.crypt_type != JDB_CRYPT_NONE) {

		if ((ret = _jdb_encrypt(h, block, block, h->hdr.blocksize)) < 0)
			goto end;

	}

	//AIO not implemented now!
	ret =
	    _jdb_seek_write(h, block, h->hdr.blocksize,
			    _jdb_blk_off(bid, JDB_HDR_SIZE, h->hdr.blocksize));

 end:

	return ret;

}

int _jdb_read_block(struct jdb_handle *h, uchar * block, jdb_bid_t bid)
{

	int ret;

	_wdeb_io(L"READ block < %u >", bid);
	
	if(h->hdr.flags & JDB_O_WR_THREAD){
		_lock_mx(&h->rdmx);
	}

	ret =
	    _jdb_seek_read(h, block, h->hdr.blocksize,
			   _jdb_blk_off(bid, JDB_HDR_SIZE, h->hdr.blocksize));

	if(h->hdr.flags & JDB_O_WR_THREAD){
		_unlock_mx(&h->rdmx);	
	}			   

	if (ret < 0)
		return ret;

	if (h->hdr.crypt_type != JDB_CRYPT_NONE) {

		ret = _jdb_decrypt(h, block, block, h->hdr.blocksize);

	}

	return ret;

}

jdb_bid_t _jdb_get_empty_bid(struct jdb_handle * h, int zero_map_entry)
{

	jdb_bid_t bid;

	jdb_bent_t bent;

	jdb_bid_t map_no;

	struct jdb_map *map;

	for (map = h->map_list.first; map; map = map->next) {

		for (bent = 0; bent < h->hdr.map_bent; bent++) {

			if (map->entry[bent].blk_type == JDB_BTYPE_EMPTY)

				bid =
				    _jdb_calc_bid(map->bid, bent);

			//bid = map->entry[bent].bid;
			if (zero_map_entry)

				bzero((char *)&map->entry[bent],
				      sizeof(struct jdb_map_blk_entry));

			return bid;

		}

	}

	_jdb_lock_handle(h);

	if (h->hdr.nblocks < h->hdr.max_blocks) {

		h->hdr.nblocks++;

		_jdb_set_handle_flag(h, JDB_HMODIF, 0);

		_jdb_unlock_handle(h);

		return h->hdr.nblocks;

	} else {

		_jdb_unlock_handle(h);

		return JDB_ID_INVAL;

	}

}

int _jdb_write_map_blk(struct jdb_handle *h, struct jdb_map *map)
{

	int ret;

	uchar *buf;

	uint32_t crc32;

	buf = _jdb_get_blockbuf(h, JDB_BTYPE_MAP);

	if (!buf)
		return -JE_NOMEM;

	if (h->hdr.flags & JDB_O_PACK) {

		_jdb_pack_map(h, map, buf);

	} else {

		//buf = (uchar*)map;
		map->hdr.crc32 = 0UL;

		memcpy(buf, &map->hdr, sizeof(struct jdb_map_blk_hdr));

		memcpy(buf + sizeof(struct jdb_map_blk_hdr), map->entry,
		       h->hdr.map_bent * sizeof(struct jdb_map_blk_entry));

		if (h->hdr.crc_type != JDB_CRC_NONE) {

			crc32 = _jdb_crc32(buf, h->hdr.blocksize);

			_wdeb_crc(L"CRC32 = %u", crc32);

			memcpy(buf + sizeof(struct jdb_map_blk_hdr) -
			       sizeof(uint32_t), &crc32, sizeof(uint32_t));

		}

	}

	ret = _jdb_write_block(h, buf, map->bid, JDB_WR_NONEW);

	if (!(h->hdr.flags & JDB_O_AIO)) {

		if (!ret){
			_wdeb_io(L"setting write to 0");
			map->write = 0;
		}

		_jdb_release_blockbuf(h, buf);

	}

	_wdeb_map(L"returns %i", ret);

	return ret;

}

int _jdb_read_map_blk(struct jdb_handle *h, struct jdb_map *map)
{

	int ret;

	uchar *buf;

	uint32_t crc32;

	buf = _jdb_get_blockbuf(h, JDB_BTYPE_MAP);

	if (!buf)
		return -JE_NOMEM;

	ret = _jdb_read_block(h, buf, map->bid);

	if (buf[0] != JDB_BTYPE_MAP) {

		_jdb_release_blockbuf(h, buf);

		return -JE_TYPE;

	}

	if (ret < 0)
		goto end;

	if (h->hdr.flags & JDB_O_PACK) {

		ret =
		    _jdb_unpack_map(h, map, buf);

	} else {

		memcpy(&map->hdr, buf, sizeof(struct jdb_map_blk_hdr));

		memcpy(map->entry, buf + sizeof(struct jdb_map_blk_hdr),
		       h->hdr.map_bent * sizeof(struct jdb_map_blk_entry));

		ret = 0;

		if (h->hdr.crc_type != JDB_CRC_NONE) {

			crc32 = map->hdr.crc32;

			memset(buf + sizeof(struct jdb_map_blk_hdr) -
			       sizeof(uint32_t), '\0', sizeof(uint32_t));

			_wdeb_crc(L"CRC32 = %u, CALC CRC32 = %u", crc32,
				  _jdb_crc32(buf, h->hdr.blocksize));

			if (crc32 != _jdb_crc32(buf, h->hdr.blocksize))
				ret = -JE_CRC;

		}

	}

 end:

	_jdb_release_blockbuf(h, buf);

	_wdeb_map(L"returns %i", ret);

	return ret;

}

int _jdb_write_table_def_blk(struct jdb_handle *h, struct jdb_table *table)
{

	int ret;

	uint32_t crc32;

	uchar *buf = _jdb_get_blockbuf(h, JDB_BTYPE_TABLE_DEF);

	if (!buf)
		return -JE_NOMEM;

	if (h->hdr.flags & JDB_O_PACK){

		_jdb_pack_table_def(h, &table->main, buf);
/*		
#ifndef NDEBUG		
	h->wrbuf1 = _jdb_get_blockbuf(h, JDB_BTYPE_VOID);
	memcpy(h->wrbuf1, buf, h->hdr.blocksize);
#endif			
*/
	} else {

		memcpy(buf, &table->main.hdr,
		       sizeof(struct jdb_table_def_blk_hdr));

		wtojcs(table->main.name,
		       buf + sizeof(struct jdb_table_def_blk_hdr),
		       table->main.hdr.namelen);

		if (h->hdr.crc_type != JDB_CRC_NONE) {
			
			memset(buf+sizeof(struct jdb_table_def_blk_hdr)-sizeof(uint32_t), '\0', sizeof(uint32_t));
			crc32 = _jdb_crc32(buf, h->hdr.blocksize);			
			_wdeb_crc(L"CRC32 = 0x%08x", crc32);

		}

		       

	}
	ret = _jdb_write_block(h, buf, table->table_def_bid, JDB_WR_NONEW);

	if (!(h->hdr.flags & JDB_O_AIO)) {

		if (!ret){
			_wdeb_io(L"setting write to 0");
			table->main.write = 0;
		}

		_jdb_release_blockbuf(h, buf);

	}

	return ret;

}

int _jdb_read_table_def_blk(struct jdb_handle *h, struct jdb_table *table)
{

	int ret;
	
	uint32_t crc32;

	uchar *buf = _jdb_get_blockbuf(h, JDB_BTYPE_TABLE_DEF);

	if (!buf)
		return -JE_NOMEM;

	if ((ret = _jdb_read_block(h, buf, table->table_def_bid)) < 0)
		goto end;
/*
#ifndef NDEBUG
	if(memcmp(h->wrbuf1, buf, h->hdr.blocksize)){
		_wdeb(L"two memmory blocks are not same!");
	}
	free(h->wrbuf1);
#endif					
*/
	if (h->hdr.flags & JDB_O_PACK) {

		if ((ret = _jdb_unpack_table_def(h, &table->main, buf)) < 0) {

			_jdb_release_blockbuf(h, buf);

			return ret;

		}
		
	} else {
	
		memcpy(&table->main.hdr, buf,
		       sizeof(struct jdb_table_def_blk_hdr));
		       
		if (h->hdr.crc_type != JDB_CRC_NONE) {

			memcpy(&crc32, buf+sizeof(struct jdb_table_def_blk_hdr)-sizeof(uint32_t), sizeof(uint32_t));
			memset(buf+sizeof(struct jdb_table_def_blk_hdr)-sizeof(uint32_t), '\0', sizeof(uint32_t));

			_wdeb_crc(L"CRC32 = 0x%08x, CALC CRC32 = 0x%08x", crc32,
				  _jdb_crc32(buf, h->hdr.blocksize));

			if (crc32 != _jdb_crc32(buf, h->hdr.blocksize))
				return -JE_CRC;
				
			memcpy(buf+sizeof(struct jdb_table_def_blk_hdr)-sizeof(uint32_t), &crc32, sizeof(uint32_t));

		}		       

		table->main.name =
		    (wchar_t *) malloc((table->main.hdr.namelen + 1) *
				       sizeof(wchar_t));

		if (jcstow
		    (buf + sizeof(struct jdb_table_def_blk_hdr),
		     table->main.name,
		     table->main.hdr.namelen) == ((size_t) - 1)) {

			free(table->main.name);

			ret = -JE_NOJCS;

		}
		
	}

 end:
	if (!(h->hdr.flags & JDB_O_AIO) || ret < 0)

		_jdb_release_blockbuf(h, buf);

	return ret;

}


int _jdb_write_typedef_blk(struct jdb_handle *h, struct jdb_typedef_blk *blk)
{

	int ret;

	uint32_t crc32;

	uchar *buf = _jdb_get_blockbuf(h, JDB_BTYPE_TYPE_DEF);

	if (!buf)
		return -JE_NOMEM;

	if (h->hdr.flags & JDB_O_PACK) {

		_jdb_pack_typedef(h, blk, buf);

	} else {

		blk->hdr.crc32 = 0UL;

		memcpy(buf, &blk->hdr, sizeof(struct jdb_typedef_blk_hdr));

		memcpy(buf + sizeof(struct jdb_typedef_blk_hdr), blk->entry,
		       h->hdr.typedef_bent *
		       sizeof(struct jdb_typedef_blk_entry));

		if (h->hdr.crc_type != JDB_CRC_NONE) {

			crc32 = _jdb_crc32(buf, h->hdr.blocksize);

			_wdeb_crc(L"CRC32 = %u", crc32);

			memcpy(buf + sizeof(struct jdb_typedef_blk_hdr) -
			       sizeof(uint32_t), &crc32, sizeof(uint32_t));

		}

	}

	ret = _jdb_write_block(h, buf, blk->bid, JDB_WR_NONEW);

	if (!(h->hdr.flags & JDB_O_AIO)) {

		if (!ret)
			blk->write = 0;

		_jdb_release_blockbuf(h, buf);

	}

	return ret;

}

int _jdb_read_typedef_blk(struct jdb_handle *h, struct jdb_typedef_blk *blk)
{

	int ret;

	uint32_t crc32;

	uchar *buf = _jdb_get_blockbuf(h, JDB_BTYPE_TYPE_DEF);

	if (!buf)
		return -JE_NOMEM;

	ret = _jdb_read_block(h, buf, blk->bid);

	if (ret < 0)
		goto end;

	if (h->hdr.flags & JDB_O_PACK) {

		ret =
		    _jdb_unpack_typedef(h, blk, buf);

	} else {

		memcpy(&blk->hdr, buf, sizeof(struct jdb_typedef_blk_hdr));

		memcpy(blk->entry, buf + sizeof(struct jdb_typedef_blk_hdr),
		       h->hdr.typedef_bent *
		       sizeof(struct jdb_typedef_blk_entry));

		ret = 0;

		if (h->hdr.crc_type != JDB_CRC_NONE) {

			crc32 = blk->hdr.crc32;

			memset(buf + sizeof(struct jdb_typedef_blk_hdr) -
			       sizeof(uint32_t), '\0', sizeof(uint32_t));

			_wdeb_crc(L"CRC32 = %u, CALC CRC32 = %u", crc32,
				  _jdb_crc32(buf, h->hdr.blocksize));

			if (crc32 != _jdb_crc32(buf, h->hdr.blocksize))
				ret = -JE_CRC;

		}

	}

	_jdb_release_blockbuf(h, buf);

 end:
	return ret;

}

int _jdb_write_col_typedef_blk(struct jdb_handle *h, struct jdb_col_typedef *blk)
{

	int ret;

	uint32_t crc32;

	uchar *buf = _jdb_get_blockbuf(h, JDB_BTYPE_COL_TYPEDEF);

	if (!buf)
		return -JE_NOMEM;

	if (h->hdr.flags & JDB_O_PACK) {

		_jdb_pack_col_typedef(h, blk, buf);

	} else {

		blk->hdr.crc32 = 0UL;

		memcpy(buf, &blk->hdr, sizeof(struct jdb_col_typedef_blk_hdr));

		memcpy(buf + sizeof(struct jdb_col_typedef_blk_hdr), blk->entry,
		       h->hdr.col_typedef_bent * sizeof(struct jdb_col_typedef_blk_entry));

		if (h->hdr.crc_type != JDB_CRC_NONE) {

			crc32 = _jdb_crc32(buf, h->hdr.blocksize);

			_wdeb_crc(L"CRC32 = %u", crc32);

			memcpy(buf + sizeof(struct jdb_col_typedef_blk_hdr) -
			       sizeof(uint32_t), &crc32, sizeof(uint32_t));

		}

	}

	ret = _jdb_write_block(h, buf, blk->bid, JDB_WR_NONEW);

	if (!(h->hdr.flags & JDB_O_AIO)) {

		if (!ret)
			blk->write = 0;

		_jdb_release_blockbuf(h, buf);

	}

	return ret;

}

int _jdb_read_col_typedef_blk(struct jdb_handle *h, struct jdb_col_typedef *blk)
{

	int ret;

	uint32_t crc32;

	uchar *buf = _jdb_get_blockbuf(h, JDB_BTYPE_COL_TYPEDEF);

	if (!buf)
		return -JE_NOMEM;

	ret = _jdb_read_block(h, buf, blk->bid);

	if (ret < 0)
		goto end;

	if (h->hdr.flags & JDB_O_PACK) {

		ret =
		    _jdb_unpack_col_typedef(h, blk, buf);

	} else {

		memcpy(&blk->hdr, buf, sizeof(struct jdb_col_typedef_blk_hdr));

		memcpy(blk->entry, buf + sizeof(struct jdb_col_typedef_blk_hdr),
		       h->hdr.col_typedef_bent * sizeof(struct jdb_col_typedef_blk_entry));

		ret = 0;

		if (h->hdr.crc_type != JDB_CRC_NONE) {

			crc32 = blk->hdr.crc32;

			memset(buf + sizeof(struct jdb_col_typedef_blk_hdr) -
			       sizeof(uint32_t), '\0', sizeof(uint32_t));

			_wdeb_crc(L"CRC32 = %u, CALC CRC32 = %u", crc32,
				  _jdb_crc32(buf, h->hdr.blocksize));

			if (crc32 != _jdb_crc32(buf, h->hdr.blocksize))
				ret = -JE_CRC;

		}

	}

 end:
	if (!(h->hdr.flags & JDB_O_AIO) || ret < 0)

		_jdb_release_blockbuf(h, buf);

	return ret;

}


int _jdb_write_celldef_blk(struct jdb_handle *h, struct jdb_celldef_blk *blk)
{

	int ret;

	uchar *buf = _jdb_get_blockbuf(h, JDB_BTYPE_CELLDEF);

	if (!buf)
		return -JE_NOMEM;

	_jdb_pack_celldef(h, blk, buf);

	ret = _jdb_write_block(h, buf, blk->bid, JDB_WR_NONEW);

	if (!(h->hdr.flags & JDB_O_AIO)) {

		if (!ret)
			blk->write = 0;

		_jdb_release_blockbuf(h, buf);

	}

	return ret;

}

int _jdb_read_celldef_blk(struct jdb_handle *h, struct jdb_celldef_blk *blk)
{

	int ret;

	uchar *buf = _jdb_get_blockbuf(h, JDB_BTYPE_CELLDEF);

	if (!buf)
		return -JE_NOMEM;

	ret = _jdb_read_block(h, buf, blk->bid);

	if (ret < 0)
		goto end;

	ret =
	    _jdb_unpack_celldef(h, blk, buf);

 end:
	if (!(h->hdr.flags & JDB_O_AIO) || ret < 0)

		_jdb_release_blockbuf(h, buf);

	return ret;

}

int _jdb_read_data_ptr_blk(struct jdb_handle *h, struct jdb_cell_data_ptr_blk *blk)
{

	int ret;

	uchar *buf = _jdb_get_blockbuf(h, JDB_BTYPE_CELL_DATA_PTR);

	if (!buf)
		return -JE_NOMEM;

	ret = _jdb_read_block(h, buf, blk->bid);

	if (ret < 0)
		goto end;

	ret = _jdb_unpack_data_ptr(h, blk, buf);

 end:
	if (!(h->hdr.flags & JDB_O_AIO) || ret < 0)

		_jdb_release_blockbuf(h, buf);

	return ret;

}

int _jdb_write_data_ptr_blk(struct jdb_handle *h, struct jdb_cell_data_ptr_blk *blk)
{

	int ret;

	uchar *buf = _jdb_get_blockbuf(h, JDB_BTYPE_CELL_DATA_PTR);

	if (!buf)
		return -JE_NOMEM;

	_jdb_pack_data_ptr(h, blk, buf);

	ret = _jdb_write_block(h, buf, blk->bid, JDB_WR_NONEW);

	if (!(h->hdr.flags & JDB_O_AIO)) {

		if (!ret)
			blk->write = 0;

		_jdb_release_blockbuf(h, buf);

	}

	return ret;

}


int _jdb_read_data_blk(	struct jdb_handle *h, struct jdb_table* table,
			struct jdb_cell_data_blk *blk, jdb_data_t type_id)
{

	int ret;

	uchar *buf = _jdb_get_blockbuf(h, JDB_BTYPE_CELL_DATA);

	if (!buf)
		return -JE_NOMEM;

	ret = _jdb_read_block(h, buf, blk->bid);

	if (ret < 0)
		goto end;

	ret = _jdb_unpack_data(h, table, blk, type_id, buf);

 end:
	if (!(h->hdr.flags & JDB_O_AIO) || ret < 0)

		_jdb_release_blockbuf(h, buf);

	return ret;

}

int _jdb_write_data_blk(struct jdb_handle *h, struct jdb_cell_data_blk *blk)
{

	int ret;

	uchar *buf = _jdb_get_blockbuf(h, JDB_BTYPE_CELL_DATA);

	if (!buf)
		return -JE_NOMEM;

	_jdb_pack_data(h, blk, buf);

	ret = _jdb_write_block(h, buf, blk->bid, JDB_WR_NONEW);

	if (!(h->hdr.flags & JDB_O_AIO)) {

		if (!ret)
			blk->write = 0;

		_jdb_release_blockbuf(h, buf);

	}

	return ret;

}

int _jdb_write_fav_blk(struct jdb_handle *h, struct jdb_fav_blk *blk)
{

	int ret;

	uint32_t crc32;

	uchar *buf = _jdb_get_blockbuf(h, JDB_BTYPE_TABLE_FAV);

	if (!buf)
		return -JE_NOMEM;

	if (h->hdr.flags & JDB_O_PACK) {

		_jdb_pack_fav(h, blk, buf);

	} else {

		blk->hdr.crc32 = 0UL;

		memcpy(buf, &blk->hdr, sizeof(struct jdb_fav_blk_hdr));

		memcpy(buf + sizeof(struct jdb_fav_blk_hdr), blk->entry,
		       h->hdr.fav_bent *
		       sizeof(struct jdb_fav_blk_entry));

		if (h->hdr.crc_type != JDB_CRC_NONE) {

			crc32 = _jdb_crc32(buf, h->hdr.blocksize);

			_wdeb_crc(L"CRC32 = %u", crc32);

			memcpy(buf + sizeof(struct jdb_fav_blk_hdr) -
			       sizeof(uint32_t), &crc32, sizeof(uint32_t));

		}

	}

	ret = _jdb_write_block(h, buf, blk->bid, JDB_WR_NONEW);

	if (!(h->hdr.flags & JDB_O_AIO)) {

		if (!ret)
			blk->write = 0;

		_jdb_release_blockbuf(h, buf);

	}

	return ret;

}

int _jdb_read_fav_blk(struct jdb_handle *h, struct jdb_fav_blk *blk)
{

	int ret;

	uint32_t crc32;

	uchar *buf = _jdb_get_blockbuf(h, JDB_BTYPE_TABLE_FAV);

	if (!buf)
		return -JE_NOMEM;

	ret = _jdb_read_block(h, buf, blk->bid);

	if (ret < 0)
		goto end;

	if (h->hdr.flags & JDB_O_PACK) {

		ret =
		    _jdb_unpack_fav(h, blk, buf);

	} else {

		memcpy(&blk->hdr, buf, sizeof(struct jdb_fav_blk_hdr));

		memcpy(blk->entry, buf + sizeof(struct jdb_fav_blk_hdr),
		       h->hdr.fav_bent *
		       sizeof(struct jdb_fav_blk_entry));

		ret = 0;

		if (h->hdr.crc_type != JDB_CRC_NONE) {

			crc32 = blk->hdr.crc32;

			memset(buf + sizeof(struct jdb_fav_blk_hdr) -
			       sizeof(uint32_t), '\0', sizeof(uint32_t));

			_wdeb_crc(L"CRC32 = %u, CALC CRC32 = %u", crc32,
				  _jdb_crc32(buf, h->hdr.blocksize));

			if (crc32 != _jdb_crc32(buf, h->hdr.blocksize))
				ret = -JE_CRC;

		}

	}

	_jdb_release_blockbuf(h, buf);

 end:
	return ret;

}

