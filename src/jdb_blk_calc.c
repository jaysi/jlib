#include "jdb.h"

jdb_bent_t _jdb_calc_bent(jdb_bsize_t blk_size, uint16_t ent_size,
			  uint16_t blk_hdr_size, uint16_t * pad)
{

	jdb_bent_t bent;

	/*
	this is always false, blk_size is 16bits and cannot be bigger than MBS!!
	if (blk_size > JDB_MAX_BSIZE)
		return JDB_BENT_INVAL;
	*/

	bent = (blk_size - blk_hdr_size) / ent_size;

	if (pad) {

		*pad = (blk_size - blk_hdr_size) - (ent_size * bent);

	}

	return bent;

}

size_t _jdb_data_len(struct jdb_handle *h, struct jdb_table * table,
		      jdb_data_t dtype, jdb_data_t * base_type, size_t * base_len)
{

	size_t ret;

	if (dtype <= JDB_TYPE_NULL) {
			
		_wdeb_find(L"base type 0x%02x...", dtype);

		ret = _jdb_base_dtype_size(dtype);

		*base_len = ret;

		*base_type = dtype;

		return ret;

	} else {
		_wdeb_find(L"extended type 0x%02x.", dtype);
		return _jdb_typedef_len(h, table, dtype, base_type, base_len);

	}

}

int _jdb_calc_data(jdb_bsize_t blocksize, jdb_bsize_t entsize,
		    jdb_bent_t * bent, jdb_bent_t * bmapsize)
{

	*bmapsize =
	    (blocksize -
	     sizeof(struct jdb_cell_data_blk_hdr)) / ((8 * entsize) + 1);

	*bent = 8 * (*bmapsize);

	if ((*bmapsize + (*bent) * entsize +
	     sizeof(struct jdb_cell_data_blk_hdr)) > blocksize)
		return -JE_FORBID;	
	
	_wdeb_calc(L"blocksize = %u, entsize = %u => blockentry = %u, bitmapsize = %u", blocksize, entsize, *bent, *bmapsize);

	return 0;

}
