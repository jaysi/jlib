#include "jdb.h"
#define _wdeb_table_def(f, a...)
#define _wdeb_table(f, a...)
#define _wdeb_load _wdeb
#define _wdeb_data_ptr	_wdeb
#define _wdeb_wr _wdeb

#define _jdb_get_tid(wname) _jdb_hash((uchar*)wname, WBYTES(wname))

int jdb_find_table(struct jdb_handle *h, wchar_t * name)
{

	struct jdb_table *table;

	if (h->magic != JDB_MAGIC)

		return -JE_NOINIT;

	if (h->fd == -1)

		return -JE_NOOPEN;
		
	for(table = h->table_list.first; table; table = table->next){
		if(!wcscmp(name, table->main.name)) return 0;
	}

	if (_jdb_find_first_map_match
	    (h, 0, 0, _jdb_get_tid(name), 0, JDB_MAP_CMP_TID) != JDB_ID_INVAL)

		return 0;

	return -JE_NOTFOUND;

}

int
jdb_create_table(struct jdb_handle *h, wchar_t * name,
		 uint32_t nrows, uint32_t ncols, uchar flags, uint16_t indexes)
{	

	struct jdb_table *table;	

	jdb_tid_t tid;

	size_t namelen;
	
	_wdeb_table(L"creating table %S, R: %u, C: %u", name, nrows, ncols);

	if (h->magic != JDB_MAGIC)

		return -JE_NOINIT;

	if (h->fd == -1)

		return -JE_NOOPEN;

	/*
	   1. is exists?
	   2. create a table_def entry inside the map and save bid
	   3. register index entry
	 */
	tid = _jdb_get_tid(name);

	if (_jdb_find_first_map_match(h, 0, 0, tid, 0, JDB_MAP_CMP_TID)
	    != JDB_ID_INVAL)

		return -JE_EXISTS;

	if ((namelen =
	     wtojcs_len(name,
			h->hdr.blocksize - (sizeof(struct jdb_table_def_blk_hdr)
					    + 1))) == ((size_t) - 1))

		return -JE_TOOLONG;

	table = (struct jdb_table *)malloc(sizeof(struct jdb_table));

	if (!table)

		return -JE_MALOC;

	table->table_def_bid =
	    _jdb_get_empty_map_entry(h, JDB_BTYPE_TABLE_DEF, 0, tid, 0);

	if (table->table_def_bid == JDB_ID_INVAL) {

		_wdeb(L"failed to get table_def_bid");

		free(table);

		return -JE_LIMIT;

	}
	
	_jdb_lock_handle(h);

	_wdeb_data_ptr(L"max_blocks = %u", h->hdr.max_blocks);

	h->hdr.nblocks++;
	//h->hdr.ndata_ptrs++;
	_jdb_set_handle_flag(h, JDB_HMODIF, 0);
	_jdb_unlock_handle(h);		

	memset(&table->main.hdr, '\0', sizeof(struct jdb_table_def_blk_hdr));	
	
	//init table_def values

	table->nwrblk = 0UL;
	table->map_chg_list_size = 0UL;
	table->map_chg_ptr = 0UL;
	table->map_chg_list = NULL;
		
	table->main.hdr.type = JDB_BTYPE_TABLE_DEF;

	table->main.hdr.tid = tid;

	table->main.hdr.ncells = 0;

	table->main.hdr.ncol_typedef = 0UL;	

	table->main.hdr.tid = tid;

	table->main.hdr.ncols = ncols;

	table->main.hdr.nrows = nrows;

	table->main.hdr.flags = flags;

	table->main.hdr.indexes = indexes;	

	table->main.hdr.namelen = namelen;

	table->main.name = (wchar_t *) malloc(WBYTES(name));

	wcscpy(table->main.name, name);
	
	table->main.hdr.namelen = namelen;
	table->main.write = 0UL;
	_JDB_SET_WR(h, &table->main, table->table_def_bid, table, 1);
	//table->main.write = 1;

	_wdeb_table_def(L"col_typedef main bid: %u", table->table_def_bid);

	table->next = NULL;

	table->col_typedef_list.first = NULL;

	table->typedef_list.first = NULL;

	table->celldef_list.first = NULL;

	table->cell_list.first = NULL;

	table->data_ptr_list.first = NULL;

	table->data_list.first = NULL;

	table->fav_list.first = NULL;


	if (!h->table_list.first) {

		h->table_list.first = table;

		h->table_list.last = table;

		h->table_list.cnt = 1UL;

	} else {

		h->table_list.last->next = table;

		h->table_list.last = h->table_list.last->next;

		h->table_list.cnt++;

	}

	h->hdr.nblocks++;
	h->hdr.ntables++;

	return 0;

}

void _jdb_free_table_def(struct jdb_table *table)
{

	if (table->main.name)

		free(table->main.name);

}

int _jdb_load_table(struct jdb_handle *h, jdb_tid_t tid, uchar flags)
{

	struct jdb_table *table;

	int ret;

	table = (struct jdb_table *)malloc(sizeof(struct jdb_table));

	if (!table)

		return -JE_MALOC;

	table->table_def_bid =
	    _jdb_find_first_map_match(h, JDB_BTYPE_TABLE_DEF, 0, tid, 0,
				      JDB_MAP_CMP_TID | JDB_MAP_CMP_BTYPE);

	if (table->table_def_bid == JDB_ID_INVAL) {

		_wdeb(L"failed to get table_def_bid");

		free(table);

		return -JE_NOTFOUND;

	}

	table->col_typedef_list.first = NULL;

	table->typedef_list.first = NULL;

	table->celldef_list.first = NULL;

	table->cell_list.first = NULL;

	table->data_ptr_list.first = NULL;

	table->data_list.first = NULL;

	table->fav_list.first = NULL;	

	if ((ret = _jdb_read_table_def_blk(h, table)) < 0)

		return ret;

	table->main.write = 0;
	table->nwrblk = 0UL;
	table->map_chg_list_size = 0UL;
	table->map_chg_ptr = 0UL;
	table->map_chg_list = NULL;	
	
	_wdeb_load(L"Loading typedefs...");

	if ((ret = _jdb_load_typedef(h, table)) < 0) {

		_jdb_free_table_def(table);

		free(table);

		return ret;

	}
	
	_wdeb_load(L"Loading Tdefs...");

	if ((ret = _jdb_load_col_typedef(h, table)) < 0) {

		_jdb_free_table_def(table);

		_jdb_free_typedef_list(table);

		free(table);

		return ret;

	}

	_wdeb_load(L"Loading celldefs...");

	if ((ret = _jdb_load_celldef(h, table)) < 0) {

		_jdb_free_table_def(table);

		_jdb_free_typedef_list(table);

		_jdb_free_col_typedef_list(table);

		free(table);

		return ret;

	}
	
	_wdeb_load(L"Loading data ptrs...");
	if ((ret = _jdb_load_data_ptr(h, table)) < 0) {

		_jdb_free_table_def(table);

		_jdb_free_typedef_list(table);

		_jdb_free_col_typedef_list(table);

		_jdb_free_celldef_list(table);		

		free(table);

		return ret;

	}

	_wdeb_load(L"Loading fav list...");

	if ((ret = _jdb_load_fav(h, table)) < 0) {

		_jdb_free_table_def(table);

		_jdb_free_typedef_list(table);

		_jdb_free_col_typedef_list(table);

		_jdb_free_celldef_list(table);	

		_jdb_free_data_ptr_list(table);

		free(table);

		return ret;

	}
	
	_wdeb_load(L"Loading fav blocks...");
	
	if((ret = _jdb_load_fav_blocks(h, table)) < 0){
		_jdb_free_table_def(table);

		_jdb_free_typedef_list(table);

		_jdb_free_col_typedef_list(table);

		_jdb_free_celldef_list(table);	

		_jdb_free_data_ptr_list(table);

		_jdb_free_fav_list(table);

		free(table);

		return ret;
		
	}

	_wdeb_load(L"Loading cell headers...");

	if((ret = _jdb_load_cells(h, table, 0)) < 0){
		_jdb_free_table_def(table);

		_jdb_free_typedef_list(table);

		_jdb_free_col_typedef_list(table);

		_jdb_free_celldef_list(table);	

		_jdb_free_data_ptr_list(table);

		_jdb_free_fav_list(table);

		_jdb_free_data_list(table);				

		free(table);

		return ret;
		
	}

	
/*
	if (flags & JDB_TABLE_LOAD_DATA) {
		_wdeb(L"WARNING: NOT LOADING ALL CELL DATA");
		ret = -JE_IMPLEMENT;
		/*
		if ((ret = _jdb_load_all_cell_data(h, table)) < 0) {

			_jdb_free_table_def(table);

			_jdb_free_typedef_list(table);

			_jdb_free_col_typedef_list(table);

			_jdb_free_data_ptr_list(table);

			_jdb_free_celldef_list(table);

			_jdb_free_fav_list(&table);

			free(table);

			return ret;
		/*
		}


	}
*/	
	table->next = NULL;
	if(!h->table_list.first){
		_wdeb_load(L"added %ls as first", table->main.name);
		h->table_list.first = table;
		h->table_list.last = table;
		h->table_list.cnt = 1UL;
	} else {
		h->table_list.last->next = table;
		h->table_list.last = table;
		h->table_list.cnt++;
		_wdeb_load(L"added %ls as last, cnt = %u", table->main.name, h->table_list.cnt);
	}	

	return 0;

}

int jdb_list_tables(struct jdb_handle *h, jcslist_entry ** list, jdb_bid_t * n)
{

	jcslist_entry *entry, *last;

	int ret;

	jdb_bid_t *bid, i;

	struct jdb_table table;
	
	ret =
	    _jdb_list_map_match(h, JDB_BTYPE_TABLE_DEF, 0, 0, 0,
				JDB_MAP_CMP_BTYPE, &bid, n);

	if (ret < 0)

		return ret;

	*list = NULL;

	for (i = 0; i < *n; i++) {

		table.table_def_bid = bid[i];

		ret = _jdb_read_table_def_blk(h, &table);

		if (!ret) {

			entry = (jcslist_entry *) malloc(sizeof(jcslist_entry));

			if (!entry) {

				if (*list)

					jcslist_free(*list);

				return -JE_MALOC;

			}

			entry->next = NULL;

			entry->wcs = table.main.name;

			if (!(*list)) {

				*list = entry;

				last = entry;

			} else {

				last->next = entry;

				last = last->next;

			}

		}

	}

	return 0;

}

int _jdb_sync_table_by_ptr(struct jdb_handle* h, struct jdb_table* table){
	struct jdb_cell_data_blk* data_blk;
	struct jdb_cell_data_ptr_blk* data_ptr_blk;
	struct jdb_celldef_blk* celldef_blk;
	struct jdb_col_typedef* col_typedef_blk;
	struct jdb_typedef_blk* typedef_blk;
	struct jdb_fav_blk* fav_blk;
	int ret = 0, ret2;
	
	if(h->hdr.flags & JDB_O_WR_THREAD){
		return _jdb_request_table_write(h, table);
	}
	
	_wdeb_wr(L"writing data blocks...");
		
	for(data_blk = table->data_list.first; data_blk; data_blk = data_blk->next){
		if(data_blk->write){
			ret2 = _jdb_write_data_blk(h, data_blk);
			if(!ret && ret2) ret = ret2;
		}
	}
	
	_wdeb_wr(L"ret = %i, writing data pointers...", ret);

	for(data_ptr_blk = table->data_ptr_list.first; data_ptr_blk; data_ptr_blk = data_ptr_blk->next){
		if(data_ptr_blk->write){
			ret2 = _jdb_write_data_ptr_blk(h, data_ptr_blk);
			if(!ret && ret2) ret = ret2;
		}
	}	
	
	_wdeb_wr(L"ret = %i, writing celldef blocks...", ret);
	
	for(celldef_blk = table->celldef_list.first; celldef_blk; celldef_blk = celldef_blk->next){
		if(celldef_blk->write){
			ret2 = _jdb_write_celldef_blk(h, celldef_blk);
			if(!ret && ret2) ret = ret2;
		}
	}
	
	_wdeb_wr(L"ret = %i, writing column type definitions...", ret);
	
	for(col_typedef_blk = table->col_typedef_list.first; col_typedef_blk; col_typedef_blk = col_typedef_blk->next){
		if(col_typedef_blk->write){
			ret2 = _jdb_write_col_typedef_blk(h, col_typedef_blk);
			if(!ret && ret2) ret = ret2;
		}
	}

	_wdeb_wr(L"ret = %i, writing type definition blocks...", ret);

	for(typedef_blk = table->typedef_list.first; typedef_blk; typedef_blk = typedef_blk->next){
		if(typedef_blk->write){
			ret2 = _jdb_write_typedef_blk(h, typedef_blk);
			if(!ret && ret2) ret = ret2;
		}
	}

	_wdeb_wr(L"ret = %i, writing fav blocks...", ret);

	for(fav_blk = table->fav_list.first; fav_blk; fav_blk = fav_blk->next){
		if(fav_blk->write){
			ret2 = _jdb_write_fav_blk(h, fav_blk);
			if(!ret && ret2) ret = ret2;
		}
	}
	
	_wdeb_wr(L"ret = %i, writing table definition blocks...", ret);	
	
	if(table->main.write){
		ret2 = _jdb_write_table_def_blk(h, table);
		if(!ret && ret2) ret = ret2;
	}
	
	_wdeb_wr(L"ret = %i", ret);
	
	return ret;
}

int jdb_sync_tables(struct jdb_handle* h){
	struct jdb_table* table;
	int ret = 0, ret2;
	for(table = h->table_list.first; table; table = table->next){		
		ret2 = _jdb_sync_table_by_ptr(h, table);
		if(!ret && ret2) ret = ret2;
	}
	
	return ret;
}

int jdb_sync_table(struct jdb_handle *h, wchar_t * name)
{
	struct jdb_table* table;	
	for(table = h->table_list.first; table; table = table->next){
		if(!wcscmp(table->main.name, name)) break;
	}
	
	if(table)
		return _jdb_sync_table_by_ptr(h, table);
	else
		return -JE_NOTFOUND;

}

int jdb_open_table(struct jdb_handle *h, wchar_t * name, uchar flags)
{
	struct jdb_table* table;
	jdb_tid_t tid;
	jdb_bid_t bid;
	int ret;
	
	for(table = h->table_list.first; table; table = table->next){
		if(!wcscmp(table->main.name, name)) return -JE_ISOPEN;
	}
	
	tid = _jdb_get_tid(name);
	
	return _jdb_load_table(h, tid, flags);
	
}

int jdb_close_table(struct jdb_handle *h, wchar_t * name)
{
	int ret;
	struct jdb_table* entry, *prev;
	
	ret = jdb_sync_table(h, name);
	if(ret < 0) return ret;
	
	prev = h->table_list.first;
	entry = prev;
	
	while(entry){
	
		if(!wcscmp(name, entry->main.name)){
			if(entry == h->table_list.first){
				h->table_list.first = h->table_list.first->next;
			} else if(entry == h->table_list.last){
				h->table_list.last = prev;
			} else {
				prev->next = entry->next;
			}
			h->table_list.cnt--;
			break;
		}
	
		prev = entry;
		entry = entry->next;
		
	}
	
	if(!entry) return -JE_UNK;
	
	_jdb_free_cell_list(entry); //put before everything
	_jdb_free_typedef_list(entry);
	_jdb_free_col_typedef_list(entry);
	_jdb_free_data_ptr_list(entry);
	_jdb_free_data_list(entry);
	_jdb_free_celldef_list(entry);
	_jdb_free_fav_list(entry);
	_jdb_free_table_def(entry);	
	
	return 0;
}

int jdb_rm_table(struct jdb_handle *h, wchar_t * name)
{
	return -JE_IMPLEMENT;
	h->hdr.ntables--;
}

int _jdb_table_handle(struct jdb_handle* h, wchar_t* name, struct jdb_table** table){

	if (h->magic != JDB_MAGIC)

		return -JE_NOINIT;

	if (h->fd == -1)

		return -JE_NOOPEN;
		
	for(*table = h->table_list.first; *table; *table = (*table)->next){
		if(!wcscmp(name, (*table)->main.name)) return 0;
	}
	
	return -JE_NOTFOUND;

}

int _jdb_find_table_by_tid(struct jdb_handle* h, jdb_tid_t tid, struct jdb_table** table){

	if (h->magic != JDB_MAGIC)

		return -JE_NOINIT;

	if (h->fd == -1)

		return -JE_NOOPEN;
		
	for(*table = h->table_list.first; *table; *table = (*table)->next){
		if(tid == (*table)->main.hdr.tid) return 0;
	}
	
	if (_jdb_find_first_map_match
	    (h, 0, 0, tid, 0, JDB_MAP_CMP_TID) != JDB_ID_INVAL)

		return -JE_NOOPEN;	
	
	return -JE_NOTFOUND;
	
}
