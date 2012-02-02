#include "jdb.h"
#include "debug.h"

#define _wdeb_data_ptr	_wdeb
#define _wdeb_load	_wdeb

int _jdb_free_data_ptr_list(struct jdb_table *table)
{
	struct jdb_cell_data_ptr_blk *data_ptr;

	while (table->data_ptr_list.first) {
		data_ptr = table->data_ptr_list.first;
		table->data_ptr_list.first = table->data_ptr_list.first->next;
		free(data_ptr->entry);
		free(data_ptr);
	}

	return 0;
}

int _jdb_create_data_ptr(struct jdb_handle *h, struct jdb_table* table)
{
	struct jdb_cell_data_ptr_blk *data_ptr;
	jdb_bid_t bid;
	jdb_bent_t i;
	
	//_wdeb_data_ptr(L"data_ptr bid %u", bid);

	data_ptr = (struct jdb_cell_data_ptr_blk *)malloc(sizeof(struct jdb_cell_data_ptr_blk));
	if (!data_ptr) {

		return -JE_MALOC;
	}
	data_ptr->entry =
	    (struct jdb_cell_data_ptr_blk_entry *)malloc(sizeof(struct jdb_cell_data_ptr_blk_entry)
					       * h->hdr.dptr_bent);
	if (!data_ptr->entry) {

		free(data_ptr);
		return -JE_MALOC;
	}
	
	bid =
	    _jdb_get_empty_map_entry(h, JDB_BTYPE_CELL_DATA_PTR, 0,
				     table->main.hdr.tid, 0);
	if(bid == JDB_ID_INVAL){
		free(data_ptr->entry);
		free(data_ptr);
		return -JE_LIMIT;		
	}
	
	_jdb_lock_handle(h);

	_wdeb_data_ptr(L"max_blocks = %u", h->hdr.max_blocks);

	h->hdr.nblocks++;
	//h->hdr.ndata_ptrs++;
	_jdb_set_handle_flag(h, JDB_HMODIF, 0);
	_jdb_unlock_handle(h);	
	
	data_ptr->next = NULL;
	memset((void *)&data_ptr->hdr, '\0', sizeof(struct jdb_cell_data_ptr_blk_hdr));
	memset((void *)data_ptr->entry, 0xff,
	       sizeof(struct jdb_cell_data_ptr_blk_entry) * h->hdr.dptr_bent);

	data_ptr->hdr.type = JDB_BTYPE_CELL_DATA_PTR;
	
	data_ptr->nent = 0UL;
	data_ptr->bid = bid;
	data_ptr->write = 0UL;
	_JDB_SET_WR(h, data_ptr, bid, table, 1);
	//data_ptr->write = 1;

	for(i = 0; i < h->hdr.dptr_bent; i++){
		data_ptr->entry[i].parent = data_ptr;
	}
	
	if (!table->data_ptr_list.first) {
		table->data_ptr_list.first = data_ptr;
		table->data_ptr_list.last = data_ptr;
		table->data_ptr_list.cnt = 1UL;
	} else {
		//table->data_ptr_list.last->hdr.next = data_ptr->bid;
		table->data_ptr_list.last->next = data_ptr;
		table->data_ptr_list.last = table->data_ptr_list.last->next;
		table->data_ptr_list.cnt++;
	}

	return 0;

}

int _jdb_write_data_ptr(struct jdb_handle *h, struct jdb_table* table)
{
	int ret = 0, fret;
	struct jdb_cell_data_ptr_blk *data_ptr;
	for (data_ptr = table->data_ptr_list.first; data_ptr; data_ptr = data_ptr->next) {
		if (data_ptr->write) {
			fret = _jdb_write_data_ptr_blk(h, data_ptr);
			if (fret < 0) {
				ret = fret;
			} else {
				data_ptr->write = 0;
			}
		}
	}

	return ret;
}

int _jdb_load_data_ptr(struct jdb_handle *h, struct jdb_table *table)
{

	jdb_bid_t *bid;

	jdb_bid_t i, j, n;

	jdb_bent_t k;

	int ret;

	struct jdb_cell_data_ptr_blk *blk;

	ret =
	    _jdb_list_map_match(h, JDB_BTYPE_CELL_DATA_PTR, 0, table->main.hdr.tid, 0,
				JDB_MAP_CMP_BTYPE | JDB_MAP_CMP_TID, &bid, &n);

	if (ret < 0) {

		if (n)
			free(bid);

		return ret;

	}

	for (i = 0; i < n; i++) {

		blk = (struct jdb_cell_data_ptr_blk*)malloc(sizeof(struct jdb_cell_data_ptr_blk));

		if (!blk) {

			free(bid);

			return -JE_MALOC;

		}

		blk->entry = (struct jdb_cell_data_ptr_blk_entry *)
		    malloc(sizeof(struct jdb_cell_data_ptr_blk_entry) *
			   h->hdr.dptr_bent);

		if (!blk->entry) {

			free(bid);

			free(blk);

			return -JE_MALOC;

		}

		blk->next = NULL;

		blk->bid = bid[i];

		blk->write = 0;
		
		_wdeb_load(L"Loading block #%u", blk->bid);

		ret = _jdb_read_data_ptr_blk(h, blk);

		if (ret < 0) {

			free(bid);

			free(blk->entry);

			free(blk);

			return ret;

		}

		for(k = 0; k < h->hdr.dptr_bent; k++){
			blk->entry[k].parent = blk;
		}

		if (!table->data_ptr_list.first) {

			table->data_ptr_list.first = blk;

			table->data_ptr_list.last = blk;

			table->data_ptr_list.cnt = 1UL;

		} else {

			table->data_ptr_list.last->next = blk;

			table->data_ptr_list.last = table->data_ptr_list.last->next;

			table->data_ptr_list.cnt++;

		}

	}

	return 0;

}

int _jdb_create_dptr_chain(struct jdb_handle* h, struct jdb_table* table,
				struct jdb_cell_data_ptr_blk_entry** list,
				jdb_bid_t needed,
				jdb_bid_t* first_bid, jdb_bent_t* first_bent){

	struct jdb_cell_data_ptr_blk* blk;
	struct jdb_cell_data_ptr_blk_entry* last, *entry;

	jdb_bid_t n = needed;
	jdb_bent_t bent, added_in_this_blk;
	jdb_bid_t i;
	int ret;
	*list = NULL;

again:	
	
	for(blk = table->data_ptr_list.first; blk; blk = blk->next){
		if(blk->nent < h->hdr.dptr_bent){
			added_in_this_blk = 0;				
			for(bent = 0; bent < h->hdr.dptr_bent; bent++){
				if(blk->entry[bent].bid == JDB_ID_INVAL){
					if(!(*list)){
						*list = &blk->entry[bent];
						last = *list;
						*first_bid = blk->bid;
						*first_bent = bent;						
													
					} else {
						last->next = &blk->entry[bent];
						last->nextdptrbid = blk->bid;
						last->nextdptrbent = bent;
						last = last->next;
					}
					
					blk->nent++;// OnUse
					added_in_this_blk++;
					n--;
					
					if(!n){
						_jdb_inc_map_nful_by_bid(h, blk->bid, added_in_this_blk);// OnUse						
						last->next = NULL;
						last->nextdptrbid = JDB_ID_INVAL;
						
						#ifndef NDEBUG
						wprintf(L"DPTR_CHAIN(first:%u[%u]):", *first_bid, *first_bent);
						for(entry = *list; entry; entry = entry->next){							
							wprintf(L"DATA:%u[%u]*%u, NEXT:%u[%u]",
							entry->bid, entry->bent, entry->nent, entry->nextdptrbid, entry->nextdptrbent);
						}
						wprintf(L"\n");
						#endif											
						
						return 0;
						
					}				
				}
			}
			_jdb_inc_map_nful_by_bid(h, blk->bid, added_in_this_blk);// OnUse
		}
	}

	if(!n){
		last->next = NULL;
		last->nextdptrbid = JDB_ID_INVAL;

		#ifndef NDEBUG
		wprintf(L"DPTR_CHAIN:");
		for(entry = *list; entry; entry = entry->next){
			wprintf(L"DATA:%u[%u]*%u, NEXT:%u[%u]",
			entry->bid, entry->bent, entry->nent,
			entry->nextdptrbid, entry->nextdptrbent);
		}
		wprintf(L"\n");
		#endif

		return 0;
	} else {//if reached here, all empty slots are filled and new blocks are needed
		
		ret = _jdb_create_data_ptr(h, table);
		if(ret < 0){//kinda undo code
			if(*list){				
				_jdb_dec_map_nful_by_bid(h, *first_bid, 1);
				for(blk = table->data_ptr_list.first; blk; blk = blk->next){
					if(blk->bid == *first_bid){
						blk->nent--;
						break;
					}
				}
			}

			while((*list)->nextdptrbid != JDB_ID_INVAL){
				_jdb_dec_map_nful_by_bid(h, (*list)->nextdptrbid, 1);
				for(blk = table->data_ptr_list.first; blk; blk = blk->next){
					if(blk->bid == (*list)->nextdptrbid){
						blk->nent--;
						break;
					}
				}				
				*list = (*list)->next;
			}
			
			return ret;
		}
		goto again;
		
	}
}				


int _jdb_load_dptr_chain(	struct jdb_handle* h, struct jdb_table* table,
				struct jdb_cell* cell,
				struct jdb_cell_data_ptr_blk_entry** list){
				
	struct jdb_cell_data_ptr_blk_entry* entry, *last, *first;
	struct jdb_cell_data_ptr_blk* blk;
	
	_wdeb_load(	L"called, starting with bid:%u, bent:%u",
			cell->celldef->bid_entry, cell->celldef->bent);
	
	if(cell->celldef->bid_entry == JDB_ID_INVAL || cell->celldef->bent == JDB_BENT_INVAL){
		return -JE_INV;
	}
	
	*list = NULL;
	
	//first = (struct jdb_cell_data_ptr_blk_entry*)malloc(sizeof(struct jdb_cell_data_ptr_blk_entry));
	//if(!first) return -JE_MALOC;
	
	//first->next = NULL;
	//first->nextdptrbid = cell->celldef->bid_entry;
	//first->nextdptrbent = cell->celldef->bent;
	
	//first->bid = cell->celldef->bid_entry;
	//first->bent = cell->celldef->bent;
	
	//resolve first
	
	_wdeb_load(L"first dptr bid = %u, first dptr bent = %u", cell->celldef->bid_entry, cell->celldef->bent);
	
	for(blk = table->data_ptr_list.first; blk; blk = blk->next){
		if(blk->bid == cell->celldef->bid_entry){
			first = &blk->entry[cell->celldef->bent];
			_wdeb_load(L"first data block bid = %u, first data block ent = %u; first->nextdptrbid = %u, first->nextdptrbent = %u",
				first->bid, first->bent, first->nextdptrbid,
				first->nextdptrbent);
			first->next = NULL;
			break;				
		}
	}
	
	last = first;
	while(last->nextdptrbid != JDB_ID_INVAL){
		for(blk = table->data_ptr_list.first; blk; blk = blk->next){
			if(last->nextdptrbid == blk->bid){
				
				entry = &blk->entry[last->nextdptrbent];
				
				entry->next = NULL;

				last->next = entry;
				last = last->next;
				
				_wdeb_load(L"last->nextdptrbid=%u, last->nextdptrbent=%u", last->nextdptrbid, last->nextdptrbent);
				
			}
		}
	}

	*list = first;

	return 0;
	
}

int _jdb_rm_dptr_chain(		struct jdb_handle* h, struct jdb_table* table,
				struct jdb_cell* cell,
				jdb_bid_t bid, jdb_bent_t bent){
				
	struct jdb_cell_data_ptr_blk_entry* next, first, *last;
	struct jdb_cell_data_ptr_blk* blk;
	
	first.next = NULL;
	first.nextdptrbid = bid;
	first.nextdptrbent = bent;
	last = &first;
	while(last->nextdptrbid != JDB_ID_INVAL){
		for(blk = table->data_ptr_list.first; blk; blk = blk->next){
			if(last->nextdptrbid == blk->bid){
				last = &blk->entry[last->nextdptrbent];
				blk->entry[last->nextdptrbent].bid = JDB_ID_INVAL;
				blk->nent--;
				_jdb_dec_map_nful_by_bid(h, blk->bid, 1);
			}
		}
	}

	//?what was that? *list = first.next;

	return 0;
	
}			
			



