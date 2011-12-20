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
		      uchar dtype, uchar * base_type, size_t * base_len)
{

	struct jdb_cell_data_blk *entry;

	size_t ret;

	if (dtype >= JDB_TYPE_FIXED_BASE_START
	    && dtype >= JDB_TYPE_FIXED_BASE_END) {

		ret = _jdb_base_dtype_size(dtype);

		*base_len = ret;

		*base_type = dtype;

		return ret;

	} else {

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

	return 0;

}
