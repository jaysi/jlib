#include "jdb.h"

static inline int _jdb_check_datalen(uchar dtype, uint32_t datalen)
{

	switch (dtype) {
	
	case JDB_TYPE_EMPTY:
		if(datalen != 0) return -JE_SIZE;
		
		return 0;

	case JDB_TYPE_BYTE:

		if (datalen != 1)

			return -JE_SIZE;

		return 0;

	case JDB_TYPE_SHORT:

		if (datalen != 2)

			return -JE_SIZE;

		return 0;

	case JDB_TYPE_LONG:

		if (datalen != 4)

			return -JE_SIZE;

		return 0;

	case JDB_TYPE_LONG_LONG:

		if (datalen != 8)

			return -JE_SIZE;

		return 0;

	case JDB_TYPE_DOUBLE:	//refer to jpack.c
		if (datalen < 8)

			return -JE_SIZE;

		return 0;
	
	default:
		return 0;	
	
	}

	return -JE_UNK;

}


int _jdb_create_celldef(struct jdb_handle *h, struct jdb_table *table)
{

	struct jdb_celldef_blk *blk;

	jdb_bent_t j;

	blk = (struct jdb_celldef_blk *)malloc(sizeof(struct jdb_celldef_blk));

	if (!blk)

		return -JE_MALOC;

	blk->entry = (struct jdb_celldef_blk_entry *)

	    malloc(sizeof(struct jdb_celldef_blk_entry) * h->hdr.celldef_bent);

	if (!blk->entry) {

		free(blk);

		return -JE_MALOC;

	}

	blk->bid =
	    _jdb_get_empty_map_entry(h, JDB_BTYPE_CELLDEF, 0,
				     table->main.hdr.tid, 0);

	bzero((char *)&blk->hdr, sizeof(struct jdb_celldef_blk_hdr));

	memset((char *)blk->entry, 0xff,
	       sizeof(struct jdb_celldef_blk_entry) * h->hdr.celldef_bent);

	blk->next = NULL;
	
	blk->hdr.type = JDB_BTYPE_CELLDEF;

	_JDB_SET_WR(h, blk, blk->bid, table, 1);
	//blk->write = 1;

	for (j = 0; j < h->hdr.celldef_bent; j++) {

		blk->entry[j].parent = blk;

	}

	if (!table->celldef_list.first) {

		table->celldef_list.first = blk;

		table->celldef_list.last = blk;

		table->celldef_list.cnt = 1UL;

	} else {

		table->celldef_list.last->next = blk;

		table->celldef_list.last = blk;

		table->celldef_list.cnt++;

	}

	return 0;

}

struct jdb_cell *_jdb_find_cell_by_pos(struct jdb_handle *h, struct
				       jdb_table
				       *table, uint32_t row, uint32_t col)
{

	struct jdb_cell *cell;

	for (cell = table->cell_list.first; cell; cell = cell->next) {		
		if (cell->celldef->row == row && cell->celldef->col == col)
			return cell;
	}

	return NULL;

}

int _jdb_load_celldef(struct jdb_handle *h, struct jdb_table *table)
{

	jdb_bid_t *bid;

	jdb_bid_t i, j, n;

	int ret;

	struct jdb_celldef_blk *blk;

	ret =
	    _jdb_list_map_match(h, JDB_BTYPE_CELLDEF, 0,
				  table->main.hdr.tid,
				  0, JDB_MAP_CMP_BTYPE | JDB_MAP_CMP_TID, &bid, &n);

	if (ret < 0) {

		if (n)

			free(bid);

		return ret;

	}

	for (i = 0; i < n; i++) {

		blk = (struct jdb_celldef_blk *)

		    malloc(sizeof(struct jdb_celldef_blk));

		if (!blk) {

			free(bid);

			return -JE_MALOC;

		}

		blk->entry = (struct jdb_celldef_blk_entry *)

		    malloc(sizeof(struct jdb_celldef_blk_entry) *
			   h->hdr.celldef_bent);

		if (!blk->entry) {

			free(bid);

			free(blk);

			return -JE_MALOC;

		}

		for (j = 0; j < h->hdr.celldef_bent; j++) {

			blk->entry[j].parent = blk;

		}

		blk->next = NULL;

		blk->bid = bid[i];

		blk->write = 0;		

		ret = _jdb_read_celldef_blk(h, blk);

		if (ret < 0) {

			free(bid);

			free(blk->entry);

			free(blk);

			return ret;

		}

		if (!table->celldef_list.first) {

			table->celldef_list.first = blk;

			table->celldef_list.last = blk;

			table->celldef_list.cnt = 1UL;

		} else {

			table->celldef_list.last->next = blk;

			table->celldef_list.last =
			    table->celldef_list.last->next;

			table->celldef_list.cnt++;

		}

	}

	return 0;

}

int _jdb_write_celldef(struct jdb_handle *h, struct jdb_table *table)
{

	struct jdb_celldef_blk *blk;

	int ret, ret2 = 0;

	for (blk = table->celldef_list.first; blk; blk = blk->next) {

		ret = _jdb_write_celldef_blk(h, blk);

		if (!ret2 && ret < 0)

			ret2 = ret;

	}

	return ret2;

}

static inline void _jdb_free_celldef_list(struct jdb_table
					  *table)
{

	struct jdb_celldef_blk *del;

	while (table->celldef_list.first) {

		del = table->celldef_list.first;

		table->celldef_list.first = table->celldef_list.first->next;

		free(del->entry);

		free(del);

	}

}

int _jdb_load_cells(struct jdb_handle *h, struct jdb_table *table, int loaddata)
{
	struct jdb_celldef_blk *blk;
	struct jdb_cell *cell;
	int ret;
	jdb_bent_t i;

	for (blk = table->celldef_list.first; blk; blk = blk->next) {

		for (i = 0; i < h->hdr.celldef_bent; i++) {

			if (blk->entry[i].row != JDB_ID_INVAL
			    && blk->entry[i].col != JDB_ID_INVAL) {

				if (_jdb_find_cell_by_pos
				    (h, table, blk->entry[i].row,
				     blk->entry[i].col))

					return -JE_CORRUPT;

				cell = (struct jdb_cell *)

				    malloc(sizeof(struct jdb_cell));

				if (!cell)

					return -JE_MALOC;

				cell->next = NULL;

				cell->celldef = &blk->entry[i];

				if (loaddata) {

					ret = _jdb_load_cell_data(h, table, cell);					

					if (ret < 0) {

						free(cell);

						return ret;

					}

					cell->data_stat = JDB_CELL_DATA_UPTODATE;

				} else {
					cell->data_stat = JDB_CELL_DATA_NOTLOADED;
				}

				if (!table->cell_list.first) {

					table->cell_list.first = cell;

					table->cell_list.last = cell;

					table->cell_list.cnt = 1UL;

				} else {

					table->cell_list.last->next = cell;

					table->cell_list.last = cell;

					table->cell_list.cnt++;

				}

			}

		}

	}

	return 0;

}

static inline int
_jdb_add_celldef(struct jdb_handle *h,
		 struct jdb_table *table,
		 uint32_t row,
		 uint32_t col,
		 uchar dtype,
		 uint32_t datalen,
		 jdb_bid_t data_bid_entry,
		 jdb_bent_t data_bent, struct jdb_celldef_blk** blk,
		 struct jdb_celldef_blk_entry **entry)
{

	jdb_bent_t bent;

	int ret;

	for (*blk = table->celldef_list.first; *blk; *blk = (*blk)->next) {

		for (bent = 0; bent < h->hdr.celldef_bent; bent++) {

			if ((*blk)->entry[bent].row == JDB_ID_INVAL
			    || (*blk)->entry[bent].col == JDB_ID_INVAL) {				

				break;

			}

		}

	}

	if (!(*blk)) {

		if ((ret = _jdb_create_celldef(h, table)) < 0)

			return ret;

		*blk = table->celldef_list.last;

		bent = 0;

	}

	if ((ret = _jdb_inc_map_nful_by_bid(h, (*blk)->bid, 1)) < 0){		
		return ret;
	}

	_JDB_SET_WR(h, *blk, (*blk)->bid, table, 1);
	//(*blk)->write = 1;

	(*blk)->entry[bent].row = row;

	(*blk)->entry[bent].col = col;

	(*blk)->entry[bent].data_type = dtype;

	(*blk)->entry[bent].datalen = datalen;

	(*blk)->entry[bent].bid_entry = data_bid_entry;

	(*blk)->entry[bent].bent = data_bent;

	*entry = &(*blk)->entry[bent];

	return 0;

}

int
_jdb_rm_celldef(struct jdb_handle *h, struct jdb_table *table,
		uint32_t row, uint32_t col)
{

	struct jdb_celldef_blk *blk;

	struct jdb_celldef_blk_entry *entry;

	jdb_bent_t bent;

	int ret;

	for (blk = table->celldef_list.first; blk; blk = blk->next) {

		for (bent = 0; bent < h->hdr.celldef_bent; bent++) {

			if (blk->entry[bent].row == row
			    && blk->entry[bent].col == col) {

				_jdb_dec_map_nful_by_bid(h, blk->bid, 1);

				blk->entry[bent].row = JDB_ID_INVAL;

				_JDB_SET_WR(h, blk, blk->bid, table, 1);
				//blk->write = 1;

				return 0;

			}

		}

	}

	return -JE_NOTFOUND;

}

int jdb_add_cell(struct jdb_handle *h,
	     wchar_t* table_name,
	     uchar * data, uint32_t datalen,
	     jdb_data_t dtype, uint32_t row, uint32_t col)
{

	int ret;

	struct jdb_cell *cell;
	
	struct jdb_celldef_blk*	celldef_blk;
	struct jdb_celldef_blk_entry *celldef_entry;
	struct jdb_typedef_blk*	typedef_blk;
	struct jdb_typedef_blk_entry* typedef_entry;
	struct jdb_table *table;

	uchar base_type;

	size_t base_len;
	
	if((ret = _jdb_table_handle(h, table_name, &table))<0) return ret;	

	if (row > table->main.hdr.nrows) {

		if (table->main.hdr.flags & JDB_TABLE_BIND_ROWS)

			return -JE_LIMIT;

	}

	if (col > table->main.hdr.ncols) {

		if (table->main.hdr.flags & JDB_TABLE_BIND_COLS)

			return -JE_LIMIT;

	}
	
	if (_jdb_find_cell_by_pos(h, table, row, col))

		return -JE_EXISTS;	

	if (dtype > JDB_TYPE_NULL) {

		if ((ret = _jdb_find_typedef(h, table, dtype, &typedef_blk, &typedef_entry)) < 0)

			return ret;
			
		if(!(typedef_entry->flags & JDB_TYPEDEF_VDATA)){ //fixed
			if(	datalen > typedef_entry->len ||
				datalen%typedef_entry->len)
					return -JE_SIZE;
		}

	} else {

		if ((ret = _jdb_check_datalen(dtype, datalen)) < 0)

			return ret;

	}

	if (table->main.hdr.flags & JDB_TABLE_ENFORCE_COL_TYPE) {

		if (dtype != jdb_find_col_type(h, table_name, col))

			return -JE_TYPE;

	}


	//add definition, leave data entry pointers invalid

	if ((ret =
	     _jdb_add_celldef(h, table, row, col, dtype, datalen,
			      JDB_ID_INVAL, JDB_BENT_INVAL, &celldef_blk,
			      &celldef_entry))<0){

		return ret;

	}

	//allocate memmory

	cell = (struct jdb_cell *)malloc(sizeof(struct jdb_cell));

	if (!cell) {

		_jdb_rm_celldef(h, table, row, col);

		return -JE_MALOC;

	}

	cell->next = NULL;
	cell->celldef = celldef_entry;

	if (datalen) {

		cell->data = (uchar *) malloc(datalen);

		if (!cell->data) {

			_jdb_rm_celldef(h, table, row, col);

			free(cell);

			return -JE_MALOC;

		}

		memcpy(cell->data, data, datalen);
		
		if(typedef_entry->flags & JDB_TYPEDEF_VDATA){

			ret = _jdb_alloc_cell_data(	h, table, cell,
							typedef_blk,
							typedef_entry,
							data,
							datalen, 1);
		
			if(ret < 0){
				free(cell->data);
				free(cell);
				_jdb_rm_celldef(h, table, row, col);
				return ret;
			}
		} else {
			ret = _jdb_add_fdata(h, table, dtype, data,
				&cell->celldef->bid_entry, &cell->celldef->bent);
					
			if(ret < 0){
				free(cell->data);
				free(cell);
				_jdb_rm_celldef(h, table, row, col);
				return ret;
			}
			
			cell->dptr = NULL;
				
		}

	} else {

		cell->data = NULL;

		cell->dptr = NULL;
		
		ret = 0;

	}

	cell->data_stat = JDB_CELL_DATA_UPTODATE;

	return ret;

}

int jdb_load_cell(struct jdb_handle *h, wchar_t * table_name,
		uint32_t col, uint32_t row, uchar** data,
		uint32_t* datalen, uchar* data_type, int* unsign){

	struct jdb_cell* cell;
	struct jdb_table* table;
	int ret;
	
	if((ret = _jdb_table_handle(h, table_name, &table)) < 0) return ret;

	if (!(cell = _jdb_find_cell_by_pos(h, table, row, col)))
		return -JE_NOTFOUND;


	if(cell->data != JDB_CELL_DATA_NOTLOADED) return -JE_EXISTS;

	ret = _jdb_load_cell_data(h, table, cell);

	if (ret < 0) {

		return ret;

	}

	cell->data_stat = JDB_CELL_DATA_UPTODATE;

	return 0;
		
}

void _jdb_free_cell(struct jdb_cell* cell){
	if(cell->celldef->datalen) free(cell->data);
}

int jdb_rm_cell(struct jdb_handle* h, wchar_t * table_name,
		uint32_t col, uint32_t row){

	struct jdb_cell *cell, *prev;
	jdb_bent_t bent;
	struct jdb_table* table;
	int ret;
	jdb_bid_t next_dptr_bid;
	jdb_bent_t next_dptr_bent;
	struct jdb_cell_data_ptr_blk_entry* dptr;
	
	if((ret = _jdb_table_handle(h, table_name, &table)) < 0) return ret;	

	prev = table->cell_list.first;
	for (cell = table->cell_list.first; cell; cell = cell->next) {		
			
		if (cell->celldef->row == row && cell->celldef->col == col){
			if(cell == table->cell_list.first){
				table->cell_list.first = table->cell_list.first->next;
			} else if(cell == table->cell_list.last){
				table->cell_list.last = prev;
			} else {
				prev->next = cell->next;
			}
		}
	
		prev = cell;			

	}

	if(cell){

		//remove celldef
		next_dptr_bid = cell->celldef->bid_entry;
		next_dptr_bent = cell->celldef->bent;
		_jdb_rm_celldef(h, table, cell->celldef->row, cell->celldef->col);
		//free data
		if(cell->celldef->datalen) free(cell->data);
		//remove data segments
		_jdb_rm_data_seg(h, table, cell->celldef->bid_entry, cell->celldef->bent, 1);
		for(dptr = cell->dptr; dptr; dptr = dptr->next){
			_jdb_rm_data_seg(h, table, dptr->bid, dptr->bent, dptr->nent);
		}
		//remove dptrs
		_jdb_rm_dptr_chain(h, table, cell, next_dptr_bid, next_dptr_bent);
		//cell->dptr = NULL;
		
		_jdb_free_cell(cell);
		return 0;
	}
	
	return -JE_NOTFOUND;
	
	
}		
		

int jdb_update_cell(struct jdb_handle *h, wchar_t * table_name,
		    uint32_t col, uint32_t row,
		    uchar * data, uint32_t datalen){
		    
	//you cannot change data_type by this call!
	int ret;
	struct jdb_table* table;
	
	if((ret = _jdb_table_handle(h, table_name, &table)) < 0) return ret;

	return -JE_IMPLEMENT;
}		    					

void _jdb_free_cell_list(struct jdb_table* table){
	return;
}


//set row or col to JDB_ID_INVAL for all matches
int jdb_list_cells(	struct jdb_handle* h, wchar_t* table_name, uint32_t col,
			uint32_t row, uint32_t** li_col, uint32_t** li_row,
			uint32_t* n){
	struct jdb_table* table;
	struct jdb_cell* cell;
	int ret;
	
	if((ret = _jdb_table_handle(h, table_name, &table)) < 0) return ret;

	//use list_buck

	*li_col = NULL;
	*li_row = NULL;
	*n = 0UL;
	
	for(cell = table->cell_list.first; cell; cell = cell->next){
		if(row == JDB_ID_INVAL && col == JDB_ID_INVAL){			
		} else if(row == JDB_ID_INVAL && col != JDB_ID_INVAL){
			if(col != cell->celldef->col) continue;
		} else if(row != JDB_ID_INVAL && col == JDB_ID_INVAL){
			if(row != cell->celldef->row) continue;
		} else {
			if(	row != cell->celldef->row &&
				col != cell->celldef->col) continue;
		}

		if(!((*n)%h->hdr.list_buck)){
			*li_col = (uint32_t*)realloc(*li_col, h->hdr.list_buck*sizeof(uint32_t));
			if(!(*li_col)){
				//free the allocated
				return -JE_MALOC;
			}
			*li_row = (uint32_t*)realloc(*li_row, h->hdr.list_buck*sizeof(uint32_t));
			if(!(*li_row)){
				//free the allocated
				return -JE_MALOC;
			}			
		}

		(*li_col)[*n] = cell->celldef->col;
		(*li_row)[*n] = cell->celldef->row;
		(*n)++;
		
	}

	return 0;
	
}
