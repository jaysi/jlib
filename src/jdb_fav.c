#include "jdb.h"
#include "debug.h"

#define _wdeb_type _wdeb
#define _wdeb_load _wdeb
#define _wdeb_data_ptr	_wdeb
#define _wdeb_ch _wdeb

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

	memset(&blk->hdr, 0, sizeof(struct jdb_fav_blk_hdr));
	
	blk->hdr.type = JDB_BTYPE_TABLE_FAV;
	blk->write = 0UL;
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

	jdb_bid_t i, n;

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

	_wdeb_ch(L"increasing fav of %u", bid);

	if (!_jdb_find_fav(h, table, bid, &blk, &entry)){
		if(entry->nhits < JDB_ID_INVAL) entry->nhits++;
		_wdeb_ch(L"fav of %u is %u", bid, entry->nhits);
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

int _jdb_rm_fav_block(struct jdb_handle* h, struct jdb_table *table, jdb_bid_t bid){
	struct jdb_fav_blk *prev, *del;
	
	del = table->fav_list.first;
	
	while(del){
		
		if(del->bid == bid){
			if(del->bid == table->fav_list.first->bid){
				table->fav_list.first = table->fav_list.first->next;
				break;
			} else if(del->bid == table->fav_list.last->bid){
				prev->next = NULL;
				table->fav_list.last = prev;
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


int _jdb_rm_fav(struct jdb_handle *h, struct jdb_table* table, jdb_bid_t bid)
{

	struct jdb_fav_blk *blk;

	struct jdb_fav_blk_entry *entry;
	jdb_bent_t nful;
	int ret;
	
	if ((ret = _jdb_find_fav(h, table, bid, &blk, &entry)) < 0)
		return ret;

	_jdb_dec_map_nful_by_bid(h, blk->bid, 1);
	entry->bid = JDB_ID_INVAL;
	
	_jdb_get_map_nful_by_bid(h, blk->bid, &nful);
	
	if(!nful){
		_jdb_rm_map_bid_entry(h, blk->bid);
		_jdb_rm_fav_block(h, table, blk->bid);
	} else {	
		_JDB_SET_WR(h, blk, blk->bid, table, 1);
		//blk->write = 1;
	}

	return 0;

}

static inline int _jdb_list_fav(struct jdb_handle* h, struct jdb_table* table, jdb_bid_t* n,
	jdb_bid_t* bid_list){
	
	jdb_bid_t i, j, found;
	struct jdb_fav_blk *blk;
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
	struct jdb_map_blk_entry* m_ent;	
	
	n = h->conf.fav_load;
	
	_wdeb_load(L"loading favourite %u of blocks", n);
	
	if(!n) return 0;
	
	bid_list = (jdb_bid_t*)malloc(sizeof(jdb_bid_t)*n);
	if(!bid_list) return -JE_MALOC;
	ret = _jdb_list_fav(h, table, &n, bid_list);
	
	if(ret < 0){
		free(bid_list);
		return ret;
	}
	
#ifndef NDEBUG
	wprintf(L"fav list");
	for(ret = 0; ret < n; ret++){
		wprintf(L" [%u]:%u ", ret, bid_list[ret]);
	}
	wprintf(L"\n");
#endif	
	
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
		m_ent = _jdb_get_map_entry_ptr(h, blk->bid);
		if(!m_ent) continue;		
		ret = _jdb_read_data_blk(h, table, blk, m_ent->dtype);
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
		
	} else {
		table->data_list.first = first;
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

