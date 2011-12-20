#include "jdb.h"

#define _wdeb_alloc _wdeb

int _jdb_create_data_blk(	struct jdb_handle* h, struct jdb_table* table,
				uchar dtype, uchar flags){
				
	struct jdb_cell_data_blk *blk;
	jdb_bent_t bent;
	size_t dsize, base_len;
	uchar base_type;

	if ((dsize =
	     _jdb_data_len(h, table, dtype, &base_type,
			    &base_len)) == JDB_SIZE_INVAL)
		return -JE_NOTFOUND;

	blk = (struct jdb_cell_data_blk *)
	    malloc(sizeof(struct jdb_cell_data_blk));

	if (!blk)
		return -JE_MALOC;

	if (_jdb_calc_data
	    (h->hdr.blocksize, dsize, &blk->maxent, &blk->bmapsize) < 0) {

		free(blk);

		return -JE_FORBID;

	}

	blk->next = NULL;

	if(flags & JDB_TYPEDEF_VDATA)
		blk->hdr.type = JDB_BTYPE_CELL_DATA_VAR;
	else
		blk->hdr.type = JDB_BTYPE_CELL_DATA_FIX;
	

	blk->bid =
	    _jdb_get_empty_map_entry(h, blk->hdr.type, dtype, table->main.hdr.tid,
				     0);

	if (blk->bid == JDB_ID_INVAL) {
		

		free(blk);

		return -JE_LIMIT;

	}

	blk->bitmap = (uchar *) malloc(blk->bmapsize);

	if (!blk->bitmap) {

		free(blk);

		return -JE_MALOC;

	}
	

	memset(blk->bitmap, '\0', blk->bmapsize);

	//type set already!

	blk->hdr.flags = 0x00;
	blk->hdr.dtype = dtype;
	
	_JDB_SET_WR(h, blk, blk->bid, table, 1);
	//blk->write = 1;

	blk->entsize = dsize;

	blk->base_len = base_len;

	blk->base_type = base_type;

	blk->nent = 0;

	blk->data_type = dtype;

	blk->datapool = (uchar *) malloc(blk->maxent * dsize);

	if (!blk->datapool) {

		free(blk->bitmap);

		free(blk);

		return -JE_MALOC;

	}

	if (!table->data_list.first) {

		table->data_list.first = blk;

		table->data_list.last = blk;

		table->data_list.cnt = 1;

	} else {

		table->data_list.last->next = blk;

		table->data_list.last = blk;

		table->data_list.cnt++;

	}

					
}

int _jdb_add_fdata(struct jdb_handle *h, struct jdb_table *table, uchar dtype,
			uchar * data, jdb_bid_t* databid, jdb_bent_t* databent)
{
	struct jdb_cell_data_blk *blk;
	jdb_bent_t bent;
	size_t dsize, base_len;
	uchar base_type;
	int ret;

	if ((dsize = _jdb_data_len(	h, table, dtype, &base_type,
					&base_len)) == JDB_SIZE_INVAL)
		return -JE_NOTFOUND;

	for (blk = table->data_list.first; blk; blk = blk->next) {

		if (blk->maxent > blk->nent && blk->data_type == dtype) {

			for (bent = 0; bent < blk->maxent; bent++) {

				if (!test_bit(blk->bitmap, bent)) {

					set_bit(blk->bitmap, bent);

					memcpy(blk->datapool + (bent * dsize),
					       data, dsize);

					blk->nent++;

					_JDB_SET_WR(h, blk, blk->bid, table, 1);
					//blk->write = 1;

					_jdb_inc_map_nful_by_bid(h, blk->bid, 1);
					
					*databid = blk->bid;
					*databent = bent;

					return 0;

				}

			}

		}

	}

	ret = _jdb_create_data_blk(h, table, dtype, 0);
	if(ret < 0) return ret;
	blk = table->data_list.last;

	memcpy(blk->datapool, data, dsize);
	set_bit(blk->bitmap, 0);

	_jdb_inc_map_nful_by_bid(h, blk->bid, 1);	

	//_JDB_SET_WR(h, blk, blk->bid, table, 1); already done in the create_blk() call
	
	*databid = blk->bid;
	*databent = 0;

	return 0;

}

int _jdb_find_data_direct(struct jdb_handle *h, struct jdb_table *table,
		       uchar dtype, uchar * data)
{

	struct jdb_cell_data_blk *blk;

	jdb_bent_t bent;

	size_t dsize, base_len;

	uchar base_type;

	if ((dsize =
	     _jdb_data_len(h, table, dtype, &base_type,
			    &base_len)) == JDB_SIZE_INVAL)
		return -JE_NOTFOUND;

	for (blk = table->data_list.first; blk; blk = blk->next) {

		if (blk->nent && blk->data_type == dtype) {

			for (bent = 0; bent < blk->maxent; bent++) {

				if (test_bit(blk->bitmap, bent)
				    && !memcmp(blk->datapool + (bent * dsize),
					       data, dsize)) {

					return 0;

				}

			}

		}

	}

	return -JE_NOTFOUND;

}

static inline int _jdb_load_data_blk(struct jdb_handle* h, struct jdb_table* table, jdb_bid_t bid){
	struct jdb_cell_data_blk *blk;

	int ret;

	blk = (struct jdb_cell_data_blk *)
	    malloc(sizeof(struct jdb_cell_data_blk));
	if (!blk) {
		return -JE_MALOC;
	}
	blk->bitmap = NULL;
	blk->bid = bid;
	ret = _jdb_read_data_blk(h, blk);
	if (ret < 0) {
		return ret;
	}
	blk->next = NULL;
	if (!table->data_list.first) {
		table->data_list.first = blk;
		table->data_list.last = blk;
		table->data_list.cnt = 1UL;
	} else {
		table->data_list.last->next = blk;
		table->data_list.last = table->data_list.last->next;
		table->data_list.cnt++;
	}

	return 0;

}


static inline int _jdb_gimme_data_blk(struct jdb_handle* h, struct jdb_table* table, struct jdb_cell_data_blk** blk, jdb_bid_t bid){
	int ret;
	for(*blk = table->data_list.first; *blk; *blk = (*blk)->next){
		if((*blk)->bid == bid) return 0;
	}

	ret = _jdb_load_data_blk(h, table, bid);
	if(ret < 0) return ret;

	*blk = table->data_list.last;
	return 0;
		
}

int _jdb_rm_data(struct jdb_handle *h, struct jdb_table *table,
	      struct jdb_cell_data_blk *blk, jdb_bent_t bent)
{

	unset_bit(blk->bitmap, bent);

	blk->nent--;

	_JDB_SET_WR(h, blk, blk->bid, table, 1);
	//blk->write = 1;

	return _jdb_dec_map_nful_by_bid(h, blk->bid, 1);

}

int _jdb_rm_data_seg(struct jdb_handle *h, struct jdb_table *table,
	      jdb_bid_t bid, jdb_bent_t bent, jdb_bent_t nent){

	struct jdb_cell_data_blk *blk;
	int ret;
	jdb_bent_t i;

	ret = _jdb_gimme_data_blk(h, table, &blk, bid);
	if(ret < 0) return ret;
	
	if(nent != (jdb_bent_t)JDB_ID_INVAL){
		for(i = bent; i < bent + nent; i++){
			unset_bit(blk->bitmap, i);
		}
	} else memset(blk->bitmap, '\0', blk->bmapsize);

	if(nent != (jdb_bent_t)JDB_ID_INVAL) blk->nent-=nent;
	else blk->nent = 0;

	_JDB_SET_WR(h, blk, blk->bid, table, 1);
	//blk->write = 1;

	return _jdb_set_map_nful_by_bid(h, blk->bid, blk->nent);
	     
}	      

void _jdb_free_data_list(struct jdb_table *table)
{

	struct jdb_cell_data_blk *blk;

	while (table->data_list.first) {

		blk = table->data_list.first;

		table->data_list.first = table->data_list.first->next;

		if (blk->bitmap)
			free(blk->bitmap);

		if (blk->datapool)
			free(blk->datapool);

		free(blk);

	}

}

int _jdb_load_data(struct jdb_handle *h, struct jdb_table *table, uchar type)
{

	struct jdb_cell_data_blk *blk;

	jdb_bid_t *bid;

	jdb_bid_t nbid, i;

	int ret;

	ret =
	    _jdb_list_map_match(h, type, 0, table->main.hdr.tid, 0,
				JDB_MAP_CMP_TID | JDB_MAP_CMP_BTYPE, &bid,
				&nbid);

	if (ret < 0) {

		if (bid)
			free(bid);

		return ret;

	}

	for (i = 0; i < nbid; i++) {

		blk = (struct jdb_cell_data_blk *)
		    malloc(sizeof(struct jdb_cell_data_blk));

		if (!blk) {

			if (bid)
				free(bid);

			_jdb_free_data_list(table);

			//free created data block list
			return -JE_MALOC;

		}

		blk->bitmap = NULL;

		blk->bid = bid[i];

		ret = _jdb_read_data_blk(h, blk);

		if (ret < 0) {

			if (bid)
				free(bid);

			_jdb_free_data_list(table);

			//free created data block list
			return ret;

		}

		blk->next = NULL;

		if (!table->data_list.first) {

			table->data_list.first = blk;

			table->data_list.last = blk;

			table->data_list.cnt = 1UL;

		} else {

			table->data_list.last->next = blk;

			table->data_list.last = table->data_list.last->next;

			table->data_list.cnt++;

		}

	}

	if (bid)
		free(bid);

	return 0;

}

int _jdb_write_data(struct jdb_handle *h, struct jdb_table *table)
{

	struct jdb_cell_data_blk *blk;

	int ret, ret2 = 0;

	for (blk = table->data_list.first; blk; blk = blk->next) {

		ret = _jdb_write_data_blk(h, blk);

		if (!ret2 && ret < 0)
			ret2 = ret;

	}

	return ret2;

}
static inline int _jdb_alloc_cell_data_a_eq_1(struct jdb_handle *h, struct jdb_table *table,
		     struct jdb_cell *cell,
		     struct jdb_typedef_blk_entry* typedef_entry,
		     jdb_bent_t nentries,
		     uchar* data, size_t datalen){

	jdb_bid_t bid;
	struct jdb_cell_data_blk* blk;
	int ret;
	jdb_bid_t first_dptr_bid;
	jdb_bent_t first_dptr_bent;	

	bid = _jdb_find_first_map_match(h, JDB_BTYPE_CELL_DATA_VAR,
		typedef_entry->type_id, table->main.hdr.tid, nentries,
		JDB_MAP_CMP_BTYPE | JDB_MAP_CMP_DTYPE | JDB_MAP_CMP_NFUL | JDB_MAP_CMP_TID);
	if(bid == JDB_ID_INVAL){
		ret = _jdb_create_data_blk(h, table, typedef_entry->type_id, JDB_TYPEDEF_VDATA);
		if(ret < 0) return ret;
		blk = table->data_list.last;
	} else {
		ret = _jdb_gimme_data_blk(h, table, &blk, bid);
		if(ret < 0) return ret;
	}

	cell->celldef->bid_entry = first_dptr_bid;
	cell->celldef->bent = first_dptr_bent;
	
	_JDB_SET_WR(h, cell->celldef->parent, cell->celldef->parent->bid, table, 1);
	//*(cell->celldef->write) = 1;

	memset((void*)blk->bitmap, 0xff, blk->bmapsize);
	memcpy((void*)blk->datapool, (void*)data, datalen);

	blk->nent = blk->maxent;

	//need one entry
	ret = _jdb_create_dptr_chain(h, table, &cell->dptr, 1, &first_dptr_bid, &first_dptr_bent);
	cell->dptr->bid = bid;
	cell->dptr->bent = 0;
	cell->dptr->nent = nentries;
	//dptr_list->nextdptrbid = JDB_ID_INVAL; these values are set in
	//the create_dptr_chain() call
	_JDB_SET_WR(h, cell->dptr->parent, cell->dptr->parent->bid, table, 1);	
	//*(cell->dptr->write) = 1;
	
	//cell->dptr = dptr_list;

	return 0;	
}

static inline int _jdb_alloc_cell_data_a_lt_1(struct jdb_handle *h, struct jdb_table *table,
		     struct jdb_cell *cell,
		     struct jdb_typedef_blk_entry* typedef_entry,
		     jdb_bent_t nchunks, jdb_bent_t nentries,
		     uchar* data, size_t datalen, int first){
	jdb_bid_t bid;
	struct jdb_cell_data_blk* blk;
	int ret;
	jdb_bent_t* blk_ent_list, blk_ent_ptr, dptr_needed;
	jdb_bid_t first_dptr_bid;
	jdb_bent_t first_dptr_bent;
	size_t pos;
	struct jdb_cell_data_ptr_blk_entry* dptr_list, *dptr_entry, *dptr_last;

	bid = _jdb_find_first_map_match(h, JDB_BTYPE_CELL_DATA_VAR,
		typedef_entry->type_id, table->main.hdr.tid, nchunks,
		JDB_MAP_CMP_BTYPE | JDB_MAP_CMP_DTYPE |
		JDB_MAP_CMP_NFUL | JDB_MAP_CMP_TID);
	if(bid == JDB_ID_INVAL){
		ret = _jdb_create_data_blk(h, table, typedef_entry->type_id, JDB_TYPEDEF_VDATA);
		if(ret < 0) return ret;
		blk = table->data_list.last;
	} else {
		ret = _jdb_gimme_data_blk(h, table, &blk, bid);
		if(ret < 0) return ret;
	}

	//need ? entries

	blk_ent_list = (jdb_bent_t*)malloc(sizeof(jdb_bent_t)*nchunks);
	if(!blk_ent_list) return -JE_MALOC;
	
	blk_ent_ptr = 0;		
	for(ret = 0; ret < nentries; ret++){
		if(!test_bit(blk->bitmap, ret)){
			blk_ent_list[blk_ent_ptr] = ret;
			_wdeb_alloc(L"got empty bit @ %i", ret);
			blk_ent_ptr++;
			
		}
	}

	_wdeb_alloc(L"block entry ptr %u", blk_ent_ptr);

	dptr_needed = 1;

	for(ret = 0; ret < blk_ent_ptr; ret++){
		if(ret && (blk_ent_list[ret] == blk_ent_list[ret-1]+1)){
			
		} else {
			dptr_needed++;
		}
	}

	_wdeb_alloc(L"needed data ptrs %u", dptr_needed);
	
	ret = _jdb_create_dptr_chain(h, table, &dptr_list, dptr_needed, &first_dptr_bid, &first_dptr_bent);
	if(ret < 0){
		free(blk_ent_list);
		return ret;
	}		

	pos = 0;

	for(ret = 0; ret < nentries; ret++){
		if(!test_bit(blk->bitmap, ret)){			
			set_bit(blk->bitmap, ret);
			memcpy(blk->datapool + ret*typedef_entry->len, data + pos, typedef_entry->len);
			pos += typedef_entry->len;			
			blk->nent++;
		}		
	}
	_jdb_inc_map_nful_by_bid(h, blk->bid, nentries);	

	cell->celldef->bid_entry = first_dptr_bid;
	cell->celldef->bent = first_dptr_bent;
	_JDB_SET_WR(h, cell->celldef->parent, cell->celldef->parent->bid, table, 1);
	//*(cell->celldef->write) = 1;

	dptr_entry = dptr_list;

	dptr_entry->bid = bid;
	_JDB_SET_WR(h, dptr_entry->parent, dptr_entry->parent->bid, table, 1);
	//*(dptr_entry->write) = 1;
	dptr_entry->bent = blk_ent_list[0];
	dptr_entry->nent = 1;
	_wdeb_alloc(L"(NEW)dptr->bid = %u", bid);
	
	for(ret = 0; ret < blk_ent_ptr; ret++){
		if(blk_ent_list[ret] == blk_ent_list[ret+1]-1){
			_wdeb_alloc(L"continues segment @ %i ( %u -> %u, N++: %u )",
					ret, blk_ent_list[ret],
					blk_ent_list[ret+1],
					dptr_entry->nent++);
			dptr_entry->nent++;
		} else {
			dptr_entry = dptr_entry->next;
			_wdeb_alloc(L"(NEW)dptr->bid = %u", bid);				
			dptr_entry->bid = bid;
			_JDB_SET_WR(h, dptr_entry->parent, dptr_entry->parent->bid, table, 1);
			//*(dptr_entry->write) = 1;
			dptr_entry->bent = blk_ent_list[ret];
			cell->celldef->bid_entry = first_dptr_bid;

		}
	}
	
	cell->dptr = dptr_list;

	free(blk_ent_list);
	return 0;
}		   

int
_jdb_alloc_cell_data(struct jdb_handle *h, struct jdb_table *table,
		     struct jdb_cell *cell, struct jdb_typedef_blk* typedef_blk,
		     struct jdb_typedef_blk_entry* typedef_entry,
		     uchar* data, size_t datalen, int first)
{

	uint32_t a;
	jdb_bent_t nentries, bmapsize, first_dptr_bent;
	jdb_bid_t bid, *bid_list, nbids, needed_full_blocks, first_dptr_bid;
	size_t type_len, nchunks, pos, copysize;
	int ret;
	jdb_bent_t* blk_ent_list, blk_ent_ptr, dptr_needed;

	struct jdb_cell_data_ptr_blk_entry* dptr_list, *dptr_entry, *dptr_last;
	struct jdb_cell_data_blk* blk;

	/*
	   calculate a = nchunks / nentries
	   if(a == 1) search for an empty block, if not found create a new one
	   if(a < 1) search for a block for empty_entries > a, if not found create a new one
	   if(a > 1) is a combine of two above cases
	   
	   data type detection:
	   search celldef to see if there is a datatype declared
	   if not successful, search for column type
	   else try to detect type
	   else use RAW
	   
	 */
	nchunks = datalen / typedef_entry->len;
	if (datalen % typedef_entry->len)
		nchunks++;					 
	 
	ret = _jdb_calc_data(h->hdr.blocksize, typedef_entry->len, &nentries,
				&bmapsize);
	if(ret < 0) return ret;
	
	a = nchunks / nentries;
	
	if(a == 1){

		return _jdb_alloc_cell_data_a_eq_1(h, table, cell,
			typedef_entry, nentries, data, datalen);
		
	} else if(a < 1){

		return _jdb_alloc_cell_data_a_lt_1(h, table, cell,
			typedef_entry, nchunks, nentries, data, datalen, 1);
				
	} else {
		//create needed empty blocks
		bid_list = NULL;
		needed_full_blocks = nchunks / nentries;
		ret = _jdb_list_map_match(h, JDB_BTYPE_CELL_DATA_VAR,
					typedef_entry->type_id, table->main.hdr.tid,
					nentries,
					JDB_MAP_CMP_BTYPE | JDB_MAP_CMP_DTYPE |
					JDB_MAP_CMP_NFUL | JDB_MAP_CMP_TID,
					&bid_list, &nbids);
		if(needed_full_blocks > nbids){
			bid_list = (jdb_bid_t*)realloc(bid_list, sizeof(jdb_bid_t)*needed_full_blocks);
			for(bid = nbids; bid < needed_full_blocks - nbids; bid++){
				ret = _jdb_create_data_blk(h, table, typedef_entry->type_id, JDB_TYPEDEF_VDATA);
				bid_list[bid] = table->data_list.last->bid;
			}
		}
		
		ret = _jdb_create_dptr_chain(h, table, &dptr_list, needed_full_blocks, &first_dptr_bid, &first_dptr_bent);
		cell->dptr = dptr_list;
		cell->celldef->bid_entry = first_dptr_bid;
		cell->celldef->bent = first_dptr_bent;
		_JDB_SET_WR(h, cell->celldef->parent, cell->celldef->parent->bid, table, 1);
		//*(cell->celldef->write) = 1;
		
		pos = 0;
		
		for(bid = 0; bid < needed_full_blocks; bid++){
			ret = _jdb_gimme_data_blk(h, table, &blk, bid_list[bid]);
			if(ret < 0){
				//remove bid_list
				return _jdb_rm_block_list(h, table, bid_list, bid);
			}

			copysize = nentries*typedef_entry->len;
			
			memset(blk->bitmap, 0xff, blk->bmapsize);
			memcpy(blk->datapool, data + pos, copysize);

			pos += copysize;
			
			blk->nent = blk->maxent;

			//need one entry
			dptr_list->bid = bid_list[bid];
			dptr_list->bent = 0;
			dptr_list->nent = nentries;
			//dptr_list->nextdptrbid = JDB_ID_INVAL; these values are set in
			//the create_dptr_chain() call
			_JDB_SET_WR(h, dptr_list->parent, dptr_list->parent->bid, table, 1);
			//*(dptr_list->write) = 1;
			dptr_list = dptr_list->next;
			if(!dptr_list->next) dptr_last = dptr_list;
		}

		if(nchunks % nentries){ //add last chunks too!

			nchunks = nchunks - ((nchunks/nentries)*nentries);

			_wdeb_alloc(L"remaining number of entries %u (nchunks %u)", nentries, nchunks);

			bid = _jdb_find_first_map_match(h, JDB_BTYPE_CELL_DATA_VAR,
				typedef_entry->type_id, table->main.hdr.tid, nchunks,
				JDB_MAP_CMP_BTYPE | JDB_MAP_CMP_DTYPE |
				JDB_MAP_CMP_NFUL | JDB_MAP_CMP_TID);
			if(bid == JDB_ID_INVAL){
				ret = _jdb_create_data_blk(h, table, typedef_entry->type_id, JDB_TYPEDEF_VDATA);
				if(ret < 0) return ret;
				blk = table->data_list.last;
			} else {
				ret = _jdb_gimme_data_blk(h, table, &blk, bid);
				if(ret < 0) return ret;
			}

			//need ? entries

			blk_ent_list = (jdb_bent_t*)malloc(sizeof(jdb_bent_t)*nchunks);
			if(!blk_ent_list) return -JE_MALOC;
		
			blk_ent_ptr = 0;		
			for(ret = 0; ret < nentries; ret++){
				if(!test_bit(blk->bitmap, ret)){
					blk_ent_list[blk_ent_ptr] = ret;
					_wdeb_alloc(L"got empty bit @ %i", ret);
					blk_ent_ptr++;
				
				}
			}

			_wdeb_alloc(L"block entry ptr %u", blk_ent_ptr);

			dptr_needed = 1;

			for(ret = 0; ret < blk_ent_ptr; ret++){
				if(ret && (blk_ent_list[ret] == blk_ent_list[ret-1]+1)){
				
				} else {
					dptr_needed++;
				}
			}

			_wdeb_alloc(L"needed data ptrs %u", dptr_needed);
		
			ret = _jdb_create_dptr_chain(h, table, &dptr_list, dptr_needed, &first_dptr_bid, &first_dptr_bent);
			if(ret < 0){
				free(blk_ent_list);
				return ret;
			}

			//glue two lists
			dptr_last->next = dptr_list;
			dptr_last->nextdptrbid = first_dptr_bid;
			dptr_last->nextdptrbent = first_dptr_bid;			

			for(ret = 0; ret < nentries; ret++){
				if(!test_bit(blk->bitmap, ret)){			
					set_bit(blk->bitmap, ret);
					memcpy(blk->datapool + ret*typedef_entry->len, data + pos, typedef_entry->len);
					pos += typedef_entry->len;				
					blk->nent++;
				}
			}
			_jdb_inc_map_nful_by_bid(h, blk->bid, nentries);

			//cell->celldef->bid_entry = first_dptr_bid;
			//cell->celldef->bent = first_dptr_bent;
			//*(cell->celldef->write) = 1;

			dptr_entry = dptr_list;

			dptr_entry->bid = bid;
			_JDB_SET_WR(h, dptr_entry->parent, dptr_entry->parent->bid, table, 1);
			//*(dptr_entry->write) = 1;
			dptr_entry->bent = blk_ent_list[0];
			dptr_entry->nent = 1;
			_wdeb_alloc(L"(NEW)dptr->bid = %u", bid);
		
			for(ret = 0; ret < blk_ent_ptr; ret++){
				if(blk_ent_list[ret] == blk_ent_list[ret+1]-1){
					_wdeb_alloc(L"continues segment @ %i ( %u -> %u, N++: %u )",
							ret, blk_ent_list[ret],
							blk_ent_list[ret+1],
							dptr_entry->nent++);
					dptr_entry->nent++;
				} else {
					dptr_entry = dptr_entry->next;				
					_wdeb_alloc(L"(NEW)dptr->bid = %u", bid);				
					dptr_entry->bid = bid;
					_JDB_SET_WR(h, dptr_entry->parent, dptr_entry->parent->bid, table, 1);
					//*(dptr_entry->write) = 1;
					dptr_entry->bent = blk_ent_list[ret];
				}
			}
			
			free(blk_ent_list);
			
			return 0;

			
		}
		
		
	}
	
	return -JE_UNK;
}

int _jdb_load_cell_data(struct jdb_handle* h, struct jdb_table* table, struct jdb_cell* cell){

	int ret;
	struct jdb_cell_data_ptr_blk_entry* dptr_list, *dptr_entry, *dptr_last;
	size_t pos;
	struct jdb_cell_data_blk* blk;
	size_t copysize, copypos;

	ret = _jdb_load_dptr_chain(h, table, cell, &dptr_list);
	if(ret < 0) return ret;
	
	cell->data = (uchar*)malloc(cell->celldef->datalen);
	if(!cell->data) return -JE_MALOC;
	
	cell->dptr = dptr_list;

	pos = 0;

	for(dptr_entry = dptr_list; dptr_entry; dptr_entry = dptr_entry->next){
		ret = _jdb_gimme_data_blk(h, table, &blk, dptr_entry->bid);		
		if(ret < 0){
			free(cell->data);
			return ret;
		}

		_jdb_inc_fav(h, table, blk->bid);

		copysize = dptr_entry->nent*blk->entsize;
		copypos = dptr_entry->bent*blk->entsize;

		memcpy(cell->data + pos, blk->datapool + copypos, copysize);
		pos += copysize;		
	}

	return 0;

	
}





