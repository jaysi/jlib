#include "jdb.h"


size_t _jdb_base_dtype_size(uchar dtype)
{

/*				base types				
#define JDB_TYPE_EMPTY	0x00	//empty
#define JDB_TYPE_BYTE	0x01	//8-bits
#define JDB_TYPE_FIXED_BASE_START	JDB_TYPE_BYTE
#define JDB_TYPE_SHORT	0x02	//16-bits
#define JDB_TYPE_LONG	0x03	//32-bits
#define JDB_TYPE_LONG_LONG	0x04	//64-bits
#define JDB_TYPE_DOUBLE	0x05	//64-bits
#define JDB_TYPE_FIXED_BASE_END		JDB_TYPE_DOUBLE
#define JDB_TYPE_CHAR	0x06	//utf-8/ascii string
#define JDB_TYPE_JCS	0x07	//jaysi's charset
#define JDB_TYPE_WIDE	0x08	//wide charset, fixed 32-bit string
#define JDB_TYPE_FORMULA	0x09	//formula
#define JDB_TYPE_RAW	0x0a	//raw data
#define JDB_TYPE_NULL	0x0f	//null type, also shows the reserved area for base-types.
*/

	_wdeb_find(L"finding size of type 0x%02x", dtype);

	switch (dtype) {

	case JDB_TYPE_EMPTY:
		return 0;

	case JDB_TYPE_BYTE:
		return 1;

	case JDB_TYPE_SHORT:
		return 2;

	case JDB_TYPE_LONG:
		return 4;

	case JDB_TYPE_LONG_LONG:
	case JDB_TYPE_DOUBLE:
		return 8;
		
	case JDB_TYPE_CHAR:
		return 1;
	
	case JDB_TYPE_JCS:
		return 2;
	
	case JDB_TYPE_WIDE:
		return 4;
			
	case JDB_TYPE_RAW:
		return 1;
	
	case JDB_TYPE_NULL:
		return 0;

	default:
		return JDB_SIZE_INVAL;

	}

}

int _jdb_create_col_typedef(struct jdb_handle *h, struct jdb_table *table)
{

	struct jdb_col_typedef *blk;

	blk = (struct jdb_col_typedef *)malloc(sizeof(struct jdb_col_typedef));

	if (!blk)
		return -JE_MALOC;

	blk->entry = (struct jdb_col_typedef_blk_entry *)
	    malloc(sizeof(struct jdb_col_typedef_blk_entry) * h->hdr.col_typedef_bent);

	if (!blk->entry) {

		free(blk);

		return -JE_MALOC;

	}

	blk->bid =
	    _jdb_get_empty_map_entry(h, JDB_BTYPE_COL_TYPEDEF, 0, table->main.hdr.tid,
				     0);
	if(blk->bid == JDB_ID_INVAL){
		free(blk->entry);
		free(blk);
		return -JE_LIMIT;		
	}
	
	_jdb_lock_handle(h);

	_wdeb_data_ptr(L"max_blocks = %u", h->hdr.max_blocks);

	h->hdr.nblocks++;
	//h->hdr.ndata_ptrs++;
	_jdb_set_handle_flag(h, JDB_HMODIF, 0);
	_jdb_unlock_handle(h);		

	memset(&blk->hdr, 0, sizeof(struct jdb_col_typedef_blk_hdr));

	memset(blk->entry, 0,
	       sizeof(struct jdb_col_typedef_blk_entry) * h->hdr.col_typedef_bent);

	blk->hdr.type = JDB_BTYPE_COL_TYPEDEF;
	blk->write = 0UL;
	blk->next = NULL;

	_JDB_SET_WR(h, blk, blk->bid, table, 1);
	//blk->write = 1;

	if (!table->col_typedef_list.first) {

		table->col_typedef_list.first = blk;

		table->col_typedef_list.last = blk;

		table->col_typedef_list.cnt = 1UL;

	} else {

		table->col_typedef_list.last->next = blk;

		table->col_typedef_list.last = blk;

		table->col_typedef_list.cnt++;

	}

	//table->main.ncol_typedef++;

	_JDB_SET_WR(h, &table->main, table->table_def_bid, table, 0);
	//table->main.write = 1;

	return 0;

}

int _jdb_load_col_typedef(struct jdb_handle *h, struct jdb_table *table)
{

	jdb_bid_t *bid;

	jdb_bid_t i, n;

	int ret;

	struct jdb_col_typedef *blk;

	ret =
	    _jdb_list_map_match(h, JDB_BTYPE_COL_TYPEDEF, 0, table->main.hdr.tid, 0,
				JDB_MAP_CMP_BTYPE | JDB_MAP_CMP_TID, &bid, &n);

	if (ret < 0) {

		if (n)
			free(bid);

		return ret;

	}

	for (i = 0; i < n; i++) {

		blk = (struct jdb_col_typedef *)malloc(sizeof(struct jdb_col_typedef));

		if (!blk) {

			free(bid);

			return -JE_MALOC;

		}

		blk->entry = (struct jdb_col_typedef_blk_entry *)
		    malloc(sizeof(struct jdb_col_typedef_blk_entry) *
			   h->hdr.col_typedef_bent);

		if (!blk->entry) {

			free(bid);

			free(blk);

			return -JE_MALOC;

		}

		blk->next = NULL;

		blk->bid = bid[i];

		blk->write = 0;
		
		_wdeb_load(L"Loading block #%u", blk->bid);

		ret = _jdb_read_col_typedef_blk(h, blk);

		if (ret < 0) {

			free(bid);

			free(blk->entry);

			free(blk);

			return ret;

		}

		if (!table->col_typedef_list.first) {

			table->col_typedef_list.first = blk;

			table->col_typedef_list.last = blk;

			table->col_typedef_list.cnt = 1UL;

		} else {

			table->col_typedef_list.last->next = blk;

			table->col_typedef_list.last = table->col_typedef_list.last->next;

			table->col_typedef_list.cnt++;

		}

	}

	return 0;

}

int _jdb_write_col_typedef(struct jdb_handle *h, struct jdb_table *table)
{

	struct jdb_col_typedef *blk;

	int ret, ret2 = 0;

	for (blk = table->col_typedef_list.first; blk; blk = blk->next) {

		ret = _jdb_write_col_typedef_blk(h, blk);

		if (!ret2 && ret < 0)
			ret2 = ret;

	}

	return ret2;

}

void _jdb_free_col_typedef_list(struct jdb_table *table)
{

	struct jdb_col_typedef *del;

	while (table->col_typedef_list.first) {

		del = table->col_typedef_list.first;

		table->col_typedef_list.first = table->col_typedef_list.first->next;

		free(del->entry);

		free(del);

	}

}

int _jdb_create_typedef(struct jdb_handle *h, struct jdb_table *table)
{

	struct jdb_typedef_blk *blk;
	
	_wdeb_type(L"called");

	blk = (struct jdb_typedef_blk *)malloc(sizeof(struct jdb_typedef_blk));

	if (!blk)
		return -JE_MALOC;

	blk->entry = (struct jdb_typedef_blk_entry *)
	    malloc(sizeof(struct jdb_typedef_blk_entry) * h->hdr.typedef_bent);

	if (!blk->entry) {

		free(blk);

		return -JE_MALOC;

	}

	memset(blk->entry, 0,
	       sizeof(struct jdb_typedef_blk_entry) * h->hdr.typedef_bent);

	blk->bid =
	    _jdb_get_empty_map_entry(h, JDB_BTYPE_TYPE_DEF, 0,
				     table->main.hdr.tid, 0);

	if(blk->bid == JDB_ID_INVAL){
		free(blk->entry);
		free(blk);
		return -JE_LIMIT;		
	}
	
	_jdb_lock_handle(h);

	_wdeb_data_ptr(L"max_blocks = %u", h->hdr.max_blocks);

	h->hdr.nblocks++;
	//h->hdr.ndata_ptrs++;
	_jdb_set_handle_flag(h, JDB_HMODIF, 0);
	_jdb_unlock_handle(h);		


	memset(&blk->hdr, 0, sizeof(struct jdb_typedef_blk_hdr));
	
	blk->hdr.type = JDB_BTYPE_TYPE_DEF;
	blk->write = 0UL;

	blk->next = NULL;

	_JDB_SET_WR(h, blk, blk->bid, table, 1);
	//blk->write = 1;

	if (!table->typedef_list.first) {

		table->typedef_list.first = blk;

		table->typedef_list.last = blk;

		table->typedef_list.cnt = 1UL;

	} else {

		table->typedef_list.last->next = blk;

		table->typedef_list.last = blk;

		table->typedef_list.cnt++;

	}

	return 0;

}

int _jdb_find_typedef(struct jdb_handle *h,
			    struct jdb_table *table, uchar type_id,
			    struct jdb_typedef_blk **blk,
			    struct jdb_typedef_blk_entry **entry)
{

	jdb_bent_t bent;

	_wdeb_find(L"finding type 0x%02x", type_id);

	for (*blk = table->typedef_list.first; *blk; *blk = (*blk)->next) {

		for (bent = 0; bent < h->hdr.typedef_bent; bent++) {

			if ((*blk)->entry[bent].type_id == type_id) {

				*entry = &((*blk)->entry[bent]);

				return 0;

			}

		}

	}
	
	_wdeb_find(L"not found");

	return -JE_NOTFOUND;

}

int jdb_typedef_flags(struct jdb_handle* h, wchar_t* table_name, jdb_data_t type_id, uchar* flags){
	struct jdb_table *table;
	struct jdb_typedef_blk *blk;
	struct jdb_typedef_blk_entry *entry;
	int ret;

	if((ret = _jdb_table_handle(h, table_name, &table))<0) return ret;
	
	if((ret = _jdb_find_typedef(h, table, type_id, &blk, &entry)) < 0) return ret;
	
	*flags = entry->flags;
	
	return 0;

		
}

int _jdb_typedef_flags_by_ptr(struct jdb_handle* h, struct jdb_table *table, jdb_data_t type_id, uchar* flags){
	struct jdb_typedef_blk *blk;
	struct jdb_typedef_blk_entry *entry;
	int ret;
	
	if((ret = _jdb_find_typedef(h, table, type_id, &blk, &entry)) < 0) return ret;
	
	*flags = entry->flags;
	
	return 0;
	
}

int jdb_find_type_base(struct jdb_handle* h, wchar_t* table_name, jdb_data_t type_id, jdb_data_t* base){
	struct jdb_table *table;
	struct jdb_typedef_blk *blk;
	struct jdb_typedef_blk_entry *entry;
	int ret;

	if(type_id < JDB_TYPE_NULL){
		*base = type_id;
		return 0;
	}

	if((ret = _jdb_table_handle(h, table_name, &table))<0) return ret;
		
	if((ret = _jdb_find_typedef(h, table, type_id, &blk, &entry)) < 0) return ret;
	
	*base = entry->base;
	
	return 0;

}

size_t _jdb_typedef_len(struct jdb_handle * h, struct jdb_table * table,
			jdb_data_t type_id, jdb_data_t * base_type,
			size_t * base_len)
{

	jdb_bent_t bent;

	struct jdb_typedef_blk *blk;

	for (blk = table->typedef_list.first; blk; blk = blk->next) {

		for (bent = 0; bent < h->hdr.typedef_bent; bent++) {

			if (blk->entry[bent].type_id == type_id) {

				*base_len =
				    _jdb_base_dtype_size(blk->entry[bent].base);

				*base_type = blk->entry[bent].base;

				return blk->entry[bent].len;

			}

		}

	}

	return JDB_SIZE_INVAL;

}

int _jdb_load_typedef(struct jdb_handle *h, struct jdb_table *table)
{

	jdb_bid_t *bid;

	jdb_bid_t i, n;

	int ret;

	struct jdb_typedef_blk *blk;

	ret =
	    _jdb_list_map_match(h, JDB_BTYPE_TYPE_DEF, 0, table->main.hdr.tid,
				0, JDB_MAP_CMP_BTYPE | JDB_MAP_CMP_TID, &bid,
				&n);

	if (ret < 0) {

		if (n)
			free(bid);

		return ret;

	}

	for (i = 0; i < n; i++) {

		blk = (struct jdb_typedef_blk *)
		    malloc(sizeof(struct jdb_typedef_blk));

		if (!blk) {

			free(bid);

			return -JE_MALOC;

		}

		blk->entry = (struct jdb_typedef_blk_entry *)
		    malloc(sizeof(struct jdb_typedef_blk_entry) *
			   h->hdr.typedef_bent);

		if (!blk->entry) {

			free(bid);

			free(blk);

			return -JE_MALOC;

		}

		blk->next = NULL;

		blk->bid = bid[i];

		blk->write = 0;
		
		_wdeb_load(L"Loading block #%u", blk->bid);

		ret = _jdb_read_typedef_blk(h, blk);

		if (ret < 0) {

			free(bid);

			free(blk->entry);

			free(blk);

			return ret;

		}

		if (!table->typedef_list.first) {

			table->typedef_list.first = blk;

			table->typedef_list.last = blk;

			table->typedef_list.cnt = 1UL;

		} else {

			table->typedef_list.last->next = blk;

			table->typedef_list.last =
			    table->typedef_list.last->next;

			table->typedef_list.cnt++;

		}

	}

	return 0;

}

int _jdb_write_typedef(struct jdb_handle *h, struct jdb_table *table)
{

	struct jdb_typedef_blk *blk;

	int ret, ret2 = 0;

	for (blk = table->typedef_list.first; blk; blk = blk->next) {

		ret = _jdb_write_typedef_blk(h, blk);

		if (!ret2 && ret < 0)
			ret2 = ret;

	}

	return ret2;

}

int jdb_add_typedef(struct jdb_handle *h, wchar_t* table_name,
		    jdb_data_t type_id, jdb_data_t base, uint32_t len,
		    uchar flags)
{

	struct jdb_typedef_blk *blk;
	struct jdb_typedef_blk_entry *entry;
	jdb_bid_t bid;
	jdb_bent_t bent;
	int ret;
	size_t base_size;	
	struct jdb_table* table;	
	
	//journalling
	uint64_t chid;
	size_t argsize[5];
	struct jdb_changlog_rec* rec;
	
	if (h->magic != JDB_MAGIC)
		return -JE_NOINIT;

	if (h->fd == -1)
		return -JE_NOOPEN;		

	if (type_id < JDB_TYPE_NULL || base > JDB_TYPE_NULL || base == JDB_TYPE_EMPTY)
		return -JE_FORBID;

	if (len > JDB_MAX_TYPE_SIZE)
		return -JE_LIMIT;
		
	if(base < JDB_TYPE_BYTE || base > JDB_TYPE_NULL)
		return -JE_FORBID;

	base_size = _jdb_base_dtype_size(base);

	if ((len % base_size) && base_size)
		return -JE_FORBID;
	
	rec = (struct jdb_changlog_rec*)malloc(sizeof(struct jdb_changlog_rec));
	if(!rec) return -JE_MALOC;
	chid = _jdb_get_chid(h, 1);
	argsize[0] = WBYTES(table_name);
	argsize[1] = sizeof(jdb_data_t);
	argsize[2] = sizeof(jdb_data_t);
	argsize[3] = sizeof(uint32_t);
	argsize[4] = sizeof(uchar);	
	_jdb_changelog_assm_rec(rec, chid, JDB_CMD_ADD_TYPEDEF, 0, 5, argsize, 
			table_name, type_id, base, len, flags);
	_jdb_changelog_reg(h, rec);
		
	if((ret = _jdb_table_handle(h, table_name, &table))<0){
		_jdb_changelog_reg_end(h, rec, ret);
		return ret;
	}

	if (!_jdb_find_typedef(h, table, type_id, &blk, &entry)){
		_jdb_changelog_reg_end(h, rec, -JE_EXISTS);
		return -JE_EXISTS;
	}

	bid =
	    _jdb_find_first_map_match(h, JDB_BTYPE_TYPE_DEF, 0,
				      table->main.hdr.tid,
				      h->hdr.typedef_bent - 1,
				      JDB_MAP_CMP_BTYPE | JDB_MAP_CMP_TID |
				      JDB_MAP_CMP_NFUL);

	_wdeb_type(L"bid #%u returned", bid);

	if (bid == JDB_ID_INVAL) {

		if ((ret = _jdb_create_typedef(h, table)) < 0){
			_jdb_changelog_reg_end(h, rec, ret);
			return ret;
		}

		blk = table->typedef_list.last;

	} else {
		/*
		blk = (struct jdb_typedef_blk*)malloc(sizeof(struct jdb_typedef_blk));
		if(!blk) return -JE_MALOC;
		blk->bid = bid;
		blk->entry = (struct jdb_typedef_blk_entry*)malloc(
				sizeof(struct jdb_typedef_blk_entry)*
				h->hdr.typedef_bent);
		if(!blk->entry){
			free(blk);
			return -JE_MALOC;
		}
		
		/*
		if ((ret = _jdb_read_typedef_blk(h, blk)) < 0){
			free(blk->entry);
			free(blk);
			return ret;
		}
		
		
		blk->next = NULL;
				
		if(!table->typedef_list.first){
			table->typedef_list.first = blk;
			table->typedef_list.cnt = 1UL;
		} else {
			table->typedef_list.last->next = blk;
			table->typedef_list.last = table->typedef_list.last->next;
			table->typedef_list.cnt++;
		}
		*/
		
		for(blk = table->typedef_list.first; blk; blk = blk->next){
			if(blk->bid == bid) break;
		}
		
		if(!blk){
			_jdb_changelog_reg_end(h, rec, -JE_UNK);
			return -JE_UNK;
		}


	}

	for (bent = 0; bent < h->hdr.typedef_bent; bent++) {

		if (blk->entry[bent].type_id == JDB_TYPE_EMPTY) {

			ret = _jdb_inc_map_nful_by_bid(h, blk->bid, 1);

			if (ret < 0){
				_jdb_changelog_reg_end(h, rec, ret);
				return ret;
			}

			blk->entry[bent].flags = flags;
			blk->entry[bent].type_id = type_id;
			blk->entry[bent].base = base;
			blk->entry[bent].len = len;

			_JDB_SET_WR(h, blk, blk->bid, table, 1);
			//blk->write = 1;
			
			_wdeb_type(L"returning");
			_jdb_changelog_reg_end(h, rec, 0);
			return 0;

		}
	}

	_jdb_changelog_reg_end(h, rec, -JE_UNK);
	return -JE_UNK;

}

int _jdb_rm_typedef_block(struct jdb_handle* h, struct jdb_table* table, jdb_bid_t bid){
	struct jdb_typedef_blk *prev, *del;
	
	del = table->typedef_list.first;
	
	while(del){
		
		if(del->bid == bid){
			if(del->bid == table->typedef_list.first->bid){
				table->typedef_list.first = table->typedef_list.first->next;
				break;
			} else if(del->bid == table->typedef_list.last->bid){
				prev->next = NULL;
				table->typedef_list.last = prev;
				break;
			} else {
				prev->next = del->next;
				break;
			}
		}
		
		prev = del;
		del = del->next;
	}
	
	if(!del) return -JE_NOTFOUND;
			
	free(del->entry);
	free(del);
	
	return 0;

}

int jdb_rm_typedef(	struct jdb_handle *h, wchar_t* table_name,
			jdb_data_t type_id)
{
	struct jdb_typedef_blk *blk;
	struct jdb_typedef_blk_entry *entry;
	int ret;	
	struct jdb_table *table;
	jdb_bent_t nful;
	
	//journalling
	uint64_t chid;
	size_t argsize[2];
	struct jdb_changelog_rec* rec;
	
	rec = (struct jdb_changelog_rec*)malloc(sizeof(struct jdb_changelog_rec));
	if(!rec) return -JE_MALOC;
	chid = _jdb_get_chid(h, 1);
	argsize[0] = WBYTES(table_name);
	argsize[1] = sizeof(jdb_data_t);
	_jdb_changelog_assm_rec(rec, chid, JDB_CMD_RM_TYPEDEF, 0, 2, argsize,
			table_name, type_id);
	_jdb_changelog_reg(h, rec);
	
	if((ret = _jdb_table_handle(h, table_name, &table))<0){
		_jdb_changelog_reg_end(h, rec, ret);
		return ret;
	}

	if ((ret = _jdb_find_typedef(h, table, type_id, &blk, &entry)) < 0){
		_jdb_changelog_reg_end(h, rec, ret);
		return ret;
	}

	_jdb_dec_map_nful_by_bid(h, blk->bid, 1);
	entry->type_id = JDB_TYPE_EMPTY;
	
	ret = _jdb_get_map_nful_by_bid(h, blk->bid, &nful);
	if(ret < 0) nful = 1;

	if(!nful){
		_jdb_rm_map_bid_entry(h, blk->bid);
		_jdb_rm_typedef_block(h, table, blk->bid);
	} else {	
		_JDB_SET_WR(h, blk, blk->bid, table, 1);
		//blk->write = 1
	}
	
	_jdb_changelog_reg_end(h, rec, 0);
	
	return 0;

}

jdb_data_t jdb_find_col_type(	struct jdb_handle * h, wchar_t* table_name,
				uint32_t col)
{

	jdb_bent_t bent;
	struct jdb_col_typedef *blk;
	struct jdb_table * table;

	if(_jdb_table_handle(h, table_name, &table)<0) return JDB_TYPE_EMPTY;

	if (col > table->main.hdr.ncols)
		return JDB_TYPE_EMPTY;	
	
	for (blk = table->col_typedef_list.first; blk; blk = blk->next) {

		for (bent = 0; bent < h->hdr.col_typedef_bent; bent++) {

			if (blk->entry[bent].col == col)
				return blk->entry[bent].type_id;

		}

	}

	return JDB_TYPE_EMPTY;

}

int jdb_assign_col_type(struct jdb_handle *h, wchar_t* table_name,
			jdb_data_t type_id, uint32_t col)
{

	struct jdb_typedef_blk *typedef_blk;
	struct jdb_typedef_blk_entry *typedef_entry;
	struct jdb_col_typedef *blk;
	struct jdb_table *table;
	jdb_bid_t bid;
	jdb_bent_t bent;
	int ret;
	
	//journalling
	uint64_t chid;
	size_t argsize[3];
	struct jdb_changelog_rec* rec;
	
	rec = (struct jdb_changelog_rec*)malloc(sizeof(struct jdb_changelog_rec));
	if(!rec) return -JE_MALOC;
	chid = _jdb_get_chid(h, 1);
	argsize[0] = WBYTES(table_name);
	argsize[1] = sizeof(jdb_data_t);
	argsize[2] = sizeof(uint32_t);
	_jdb_changelog_assm_rec(rec, chid, JDB_CMD_ASSIGN_COLTYPE, 0, 3, argsize,
			table_name, type_id, col);
	_jdb_changelog_reg(h, rec);
	
	if((ret = _jdb_table_handle(h, table_name, &table))<0){
		_jdb_changelog_reg_end(h, rec, ret);
		return ret;
	}

	if (col > table->main.hdr.ncols){
		_jdb_changelog_reg_end(h, rec, -JE_LIMIT);
		return -JE_LIMIT;
	}

	if (jdb_find_col_type(h, table_name, col) != JDB_TYPE_EMPTY){
		_jdb_changelog_reg_end(h, rec, -JE_EXISTS);
		return -JE_EXISTS;
	}

	if ((ret =
	     _jdb_find_typedef(h, table, type_id, &typedef_blk,
			       &typedef_entry)) < 0){
		_jdb_changelog_reg_end(h, rec, ret);
		return ret;
	}
		
	bid =
	    _jdb_find_first_map_match(h, JDB_BTYPE_COL_TYPEDEF, 0,
				      table->main.hdr.tid,
				      h->hdr.col_typedef_bent - 1,
				      JDB_MAP_CMP_BTYPE | JDB_MAP_CMP_TID |
				      JDB_MAP_CMP_NFUL);

	_wdeb_type(L"bid #%u returned", bid);

	if (bid == JDB_ID_INVAL) {

		if ((ret = _jdb_create_col_typedef(h, table)) < 0){
			_jdb_changelog_reg_end(h, rec, ret);
			return ret;
		}

		blk = table->col_typedef_list.last;

	} else {
	
		/*
		blk = (struct jdb_col_typedef*)malloc(sizeof(struct jdb_col_typedef));
		if(!blk) return -JE_MALOC;
		blk->bid = bid;
		blk->entry = (struct jdb_col_typedef_blk_entry*)malloc(
				sizeof(struct jdb_col_typedef_blk_entry)*
				h->hdr.col_typedef_bent);
		if(!blk->entry){
			free(blk);
			return -JE_MALOC;
		}
		
		if ((ret = _jdb_read_col_typedef_blk(h, blk)) < 0){
			free(blk->entry);
			free(blk);
			return ret;
		}
		
		blk->next = NULL;
				
		if(!table->col_typedef_list.first){
			table->col_typedef_list.first = blk;
			table->col_typedef_list.cnt = 1UL;
		} else {
			table->col_typedef_list.last->next = blk;
			table->col_typedef_list.last = table->col_typedef_list.last->next;
			table->col_typedef_list.cnt++;
		}
		*/
		
		for(blk = table->col_typedef_list.first; blk; blk = blk->next){
			if(blk->bid == bid) break;
		}
		
		if(!blk){
			_jdb_changelog_reg_end(h, rec, ret);
			return -JE_UNK;
		}
		
		
	}
	
	_wdeb_add(L"typedef block created");

	for (bent = 0; bent < h->hdr.col_typedef_bent; bent++) {

		if (blk->entry[bent].type_id == JDB_TYPE_EMPTY) {

			ret = _jdb_inc_map_nful_by_bid(h, blk->bid, 1);

			if (ret < 0){
				_jdb_changelog_reg_end(h, rec, ret);
				return ret;
			}

			blk->entry[bent].type_id = type_id;
			blk->entry[bent].col = col;

			_JDB_SET_WR(h, blk, blk->bid, table, 1);
			//blk->write = 1;
			
			table->main.hdr.ncol_typedef++;
			
			_JDB_SET_WR(h, &table->main, table->table_def_bid, table, 1);
			_jdb_changelog_reg_end(h, rec, 0);
			return 0;

		}
	}
	
	_jdb_changelog_reg_end(h, rec, -JE_UNK);
	
	return -JE_UNK;
	
}

int _jdb_rm_col_typedef_block(struct jdb_handle* h, struct jdb_table *table, jdb_bid_t bid){
	struct jdb_col_typedef *prev, *del;
	
	del = table->col_typedef_list.first;
	
	while(del){
		
		if(del->bid == bid){
			if(del->bid == table->col_typedef_list.first->bid){
				table->col_typedef_list.first = table->col_typedef_list.first->next;
				break;
			} else if(del->bid == table->col_typedef_list.last->bid){
				prev->next = NULL;
				table->col_typedef_list.last = prev;
				break;
			} else {
				prev->next = del->next;
				break;
			}
		}
		
		prev = del;
		del = del->next;
	}
	
	if(!del) return -JE_NOTFOUND;
			
	free(del->entry);
	free(del);
	
	return 0;
	
}


int jdb_rm_col_type(struct jdb_handle *h, wchar_t* table_name, uint32_t col)
{

	//struct jdb_typedef_blk* typedef_blk;
	//struct jdb_typedef_blk_entry* typedef_entry;
	struct jdb_col_typedef *blk;
	jdb_bent_t bent;
	struct jdb_table *table;
	int ret;
	
	//journalling
	uint64_t chid;
	size_t argsize[2];
	struct jdb_changelog_rec* rec;
	
	rec = (struct jdb_changelog_rec*)malloc(sizeof(struct jdb_changelog_rec));
	if(!rec) return -JE_MALOC;
	chid = _jdb_get_chid(h, 1);
	argsize[0] = WBYTES(table_name);	
	argsize[1] = sizeof(uint32_t);
	_jdb_changelog_assm_rec(rec, chid, JDB_CMD_RM_COLTYPE, 0, 2, argsize,
			table_name, col);
	_jdb_changelog_reg(h, rec);
			
	
	if((ret = _jdb_table_handle(h, table_name, &table))<0){
		_jdb_changelog_reg_end(h, rec, ret);
		return ret;	
	}

	if (col > table->main.hdr.ncols){
		_jdb_changelog_reg_end(h, rec, -JE_LIMIT);
		return -JE_LIMIT;
	}

	//if((ret=_jdb_find_typedef(h, table, type_id, &typedef_blk, &typedef_entry)) < 0) return ret;

	for (blk = table->col_typedef_list.first; blk; blk = blk->next) {

		for (bent = 0; bent < h->hdr.col_typedef_bent; bent++) {

			if (blk->entry[bent].col == col) {

				ret = _jdb_dec_map_nful_by_bid(h, blk->bid, 1);

				if (ret < 0){
					_jdb_changelog_reg_end(h, rec, ret);
					return ret;
				}

				blk->entry[bent].type_id = JDB_TYPE_EMPTY;
				
				_jdb_get_map_nful_by_bid(h, blk->bid, &bent);
				
				if(!bent){
					_jdb_rm_map_bid_entry(h, blk->bid);
					_jdb_rm_col_typedef_block(h, table, blk->bid);
				} else {
					_JDB_SET_WR(h, blk, blk->bid, table, 1);
					//blk->write = 1;
				}
				
				table->main.hdr.ncol_typedef--;
				
				_JDB_SET_WR(h, &table->main, table->table_def_bid, table, 1);
				
				_jdb_changelog_reg_end(h, rec, 0);
				return 0;

			}

		}

	}
	
	_jdb_changelog_reg_end(h, rec, -JE_NOTFOUND);

	return -JE_NOTFOUND;

}

void _jdb_free_typedef_list(struct jdb_table *table)
{

	struct jdb_typedef_blk *del;

	while (table->typedef_list.first) {

		del = table->typedef_list.first;

		table->typedef_list.first = table->typedef_list.first->next;

		free(del->entry);

		free(del);

	}

}

