#include "jdb.h"

int _jdb_create_fav(struct jdb_handle *h, struct jdb_table *table)
{

	struct jdb_fav_blk *blk;
	
	_wdeb_type(L"called");

	blk = (struct jdb_fav_blk *)malloc(sizeof(struct jdb_fav_blk));

	if (!blk)
		return -JE_MALOC;

	blk->entry = (struct jdb_fav_blk_entry *)
	    malloc(sizeof(struct jdb_fav_blk_entry)*h->hdr.fav_bent);

	if (!blk->entry) {

		free(blk);

		return -JE_MALOC;

	}

	memset(blk->entry, 0xff,
	       sizeof(struct jdb_fav_blk_entry)*h->hdr.fav_bent);

	blk->bid =
	    _jdb_get_empty_map_entry(h, JDB_BTYPE_TABLE_FAV, 0,
				     table->main.hdr.tid, 0);

	memset(&blk->hdr, 0, sizeof(struct jdb_fav_blk_hdr));
	
	blk->hdr.type = JDB_BTYPE_TABLE_FAV;

	blk->next = NULL;

	_JDB_SET_WR(h, blk, blk->bid, table, 1);
	//blk->write = 1;

	if (!table->fav_list.first) {

		table->fav_list.first = blk;

		table->fav_list.last = blk;

		table->fav_list.cnt = 1UL;

	} else {

		table->fav_list.last->next = blk;

		table->fav_list.last = blk;

		table->fav_list.cnt++;

	}

	return 0;

}

int _jdb_find_fav(struct jdb_handle *h,
			    struct jdb_table *table, jdb_bid_t bid,
			    struct jdb_fav_blk **blk,
			    struct jdb_fav_blk_entry **entry)
{

	jdb_bent_t bent;

	for (*blk = table->fav_list.first; *blk; *blk = (*blk)->next) {

		for (bent = 0; bent < h->hdr.fav_bent; bent++) {

			if ((*blk)->entry[bent].bid == bid) {

				*entry = &((*blk)->entry[bent]);

				return 0;

			}

		}

	}

	return -JE_NOTFOUND;

}

int _jdb_load_fav(struct jdb_handle *h, struct jdb_table *table)
{

	jdb_bid_t *bid;

	jdb_bid_t i, j, n;

	int ret;

	struct jdb_fav_blk *blk;

	ret =
	    _jdb_list_map_match(h, JDB_BTYPE_TABLE_FAV, 0, table->main.hdr.tid,
				0, JDB_MAP_CMP_BTYPE | JDB_MAP_CMP_TID, &bid,
				&n);

	if (ret < 0) {

		if (n)
			free(bid);

		return ret;

	}

	for (i = 0; i < n; i++) {

		blk = (struct jdb_fav_blk *)
		    malloc(sizeof(struct jdb_fav_blk));

		if (!blk) {

			free(bid);

			return -JE_MALOC;

		}

		blk->entry = (struct jdb_fav_blk_entry *)
		    malloc(sizeof(struct jdb_fav_blk_entry) *
			   h->hdr.fav_bent);

		if (!blk->entry) {

			free(bid);

			free(blk);

			return -JE_MALOC;

		}

		blk->next = NULL;

		blk->bid = bid[i];

		blk->write = 0;
		
		_wdeb_load(L"Loading block #%u", blk->bid);

		ret = _jdb_read_fav_blk(h, blk);

		if (ret < 0) {

			free(bid);

			free(blk->entry);

			free(blk);

			return ret;

		}

		if (!table->fav_list.first) {

			table->fav_list.first = blk;

			table->fav_list.last = blk;

			table->fav_list.cnt = 1UL;

		} else {

			table->fav_list.last->next = blk;

			table->fav_list.last =
			    table->fav_list.last->next;

			table->fav_list.cnt++;

		}

	}

	return 0;

}

int _jdb_write_fav(struct jdb_handle *h, struct jdb_table *table)
{

	struct jdb_fav_blk *blk;

	int ret, ret2 = 0;

	for (blk = table->fav_list.first; blk; blk = blk->next) {

		ret = _jdb_write_fav_blk(h, blk);

		if (!ret2 && ret < 0)
			ret2 = ret;

	}

	return ret2;

}

int _jdb_inc_fav(struct jdb_handle *h, struct jdb_table* table,
		    jdb_bid_t bid)
{

	struct jdb_fav_blk *blk;

	struct jdb_fav_blk_entry *entry;

	jdb_bid_t fav_blk_bid;

	jdb_bent_t bent;

	int ret;

	size_t base_size;
	
//	struct jdb_table* table;	

	if (!_jdb_find_fav(h, table, bid, &blk, &entry)){
		if(entry->nhits < JDB_ID_INVAL) entry->nhits++;
		return 0;
	}
		

	fav_blk_bid =
	    _jdb_find_first_map_match(h, JDB_BTYPE_TABLE_FAV, 0,
				      table->main.hdr.tid,
				      h->hdr.fav_bent - 1,
				      JDB_MAP_CMP_BTYPE | JDB_MAP_CMP_TID |
				      JDB_MAP_CMP_NFUL);

	_wdeb_type(L"bid #%u returned", bid);

	if (fav_blk_bid == JDB_ID_INVAL) {

		if ((ret = _jdb_create_fav(h, table)) < 0)
			return ret;

		blk = table->fav_list.last;

	} else {		
		for(blk = table->fav_list.first; blk; blk = blk->next){
			if(blk->bid == fav_blk_bid) break;
		}
		
		
		if(!blk) return -JE_UNK;


	}

	for (bent = 0; bent < h->hdr.fav_bent; bent++) {

		if (blk->entry[bent].bid == JDB_ID_INVAL) {

			ret = _jdb_inc_map_nful_by_bid(h, blk->bid, 1);

			if (ret < 0)
				return ret;

			blk->entry[bent].bid = bid;

			blk->entry[bent].nhits = 1UL;

			_JDB_SET_WR(h, blk, blk->bid, table, 1);
			//blk->write = 1;
			
			_wdeb_type(L"returning");

			return 0;

		}
	}

	return -JE_UNK;

}

int _jdb_rm_fav(struct jdb_handle *h, struct jdb_table* table, jdb_bid_t bid)
{

	struct jdb_fav_blk *blk;

	struct jdb_fav_blk_entry *entry;

	int ret;
	
	if ((ret = _jdb_find_fav(h, table, bid, &blk, &entry)) < 0)
		return ret;

	entry->bid = JDB_ID_INVAL;

	_JDB_SET_WR(h, blk, blk->bid, table, 1);
	//blk->write = 1;

	return _jdb_dec_map_nful_by_bid(h, blk->bid, 1);

}

static inline int _jdb_list_fav(struct jdb_handle* h, struct jdb_table* table, jdb_bid_t* n,
	jdb_bid_t* bid_list){
	
	jdb_bid_t i, j, found;
	struct jdb_fav_blk *blk;
	struct jdb_fav_blk_entry *entry;
	jdb_bid_t* hit_list, sm, sm_i;

	hit_list = (jdb_bid_t*)malloc(sizeof(jdb_bid_t)*(*n));
	if(!hit_list) return -JE_MALOC;
	
	found = 0;
	sm_i = 0;
	sm = JDB_ID_INVAL;
	for(blk = table->fav_list.first; blk; blk = blk->next){
		for(i = 0; i < h->hdr.fav_bent; i++){
			if(blk->entry[i].bid != JDB_ID_INVAL){
				if(found < *n){
					bid_list[found] = blk->entry[i].bid;
					hit_list[found] = blk->entry[i].nhits;
					if(hit_list[found] < sm){
						sm = hit_list[found];
						sm_i = found;
					}
					found++;
				} else {					
					if(blk->entry[i].nhits > sm){
						//replace smallest value with nhits
						bid_list[sm_i] = blk->entry[i].bid;
						hit_list[sm_i] = blk->entry[i].nhits;
						sm = blk->entry[i].nhits;
						
						//reset sm value
						for(j = 0; j < *n; j++){
							if(hit_list[j] < sm){
								sm = hit_list[j];
								sm_i = j;
							}
						}
					}	
				}
			}
		}
	}

	if(found < *n) *n = found;

	return 0;
	
}

int _jdb_load_fav_blocks(struct jdb_handle* h, struct jdb_table* table){
	jdb_bid_t n, i;
	jdb_bid_t* bid_list;
	int ret = 0;
	struct jdb_cell_data_blk* blk, *first, *last;
	
	n = h->conf.fav_load;
	
	bid_list = (jdb_bid_t*)malloc(sizeof(jdb_bid_t)*n);
	if(!bid_list) return -JE_MALOC;
	ret = _jdb_list_fav(h, table, &n, bid_list);
	if(ret < 0){
		free(bid_list);
		return ret;
	}
	
	if(!n) return 0;
	
	first = NULL;
	last = NULL;
	blk = NULL;
	
	for(i = 0; i < n; i++){
		if(!blk){
			blk = (struct jdb_cell_data_blk*)malloc(sizeof(struct jdb_cell_data_blk));
			if(!blk){
				ret = -JE_MALOC;
				goto end;
			}
		}

		blk->bid = bid_list[i];
		ret = _jdb_read_data_blk(h, blk);
		if(ret < 0)
			continue;		
		
		blk->next = NULL;
		if(!first){
			first = blk;
			last = blk;
		} else {
			last->next = blk;
			last = blk;
		}

		blk = NULL;
	}
	
end:
	if(bid_list) free(bid_list);
	if(ret < 0){
		//set me free!
		while(first){
			blk = first;
			first = first->next;
			free(blk->bitmap);
			free(blk->datapool);
			free(blk);
		}
		
	}

	return ret;

	
}

void _jdb_free_fav_list(struct jdb_table *table)
{

	struct jdb_fav_blk *del;

	while (table->fav_list.first) {

		del = table->fav_list.first;

		table->fav_list.first = table->fav_list.first->next;

		free(del->entry);

		free(del);

	}

}

