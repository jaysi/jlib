#include "jdb.h"
#include <stdarg.h>

int jdb_init_filter(	struct jdb_filter* filter, size_t n, uint32_t col_start,
			uint32_t col_end, uint32_t row_start, uint32_t row_end,
			uint32_t sort_cmp_col, void* sort_cmp_fn){
	
	filter->entry = (struct jdb_filter_entry*)malloc(n*sizeof(struct jdb_filter_entry));	
	if(!filter->entry) return -JE_MALOC;
	
	for(filter->cnt = 0; filter->cnt < n; filter->cnt++){
		filter->entry[filter->cnt].type = 0x00;
	}

	filter->nfilters = n;
	filter->cnt = 0UL;

	filter->col_start = col_start;
	filter->col_end = col_end;
	filter->row_start = row_start;
	filter->row_end = row_end;
	filter->sort_col = sort_cmp_col;
	filter->sort_fn = sort_cmp_fn;

	return 0;
}

int jdb_add_filter(	struct jdb_filter* filter, uchar type, uchar not, uint32_t col,
			uchar logic, uchar match, jdb_data_t data_type, uint32_t str_len,
			uchar* str){

	if(filter->cnt >= filter->nfilters) return -JE_LIMIT;
	
	filter->entry[filter->cnt].type = type;
	filter->entry[filter->cnt].col = col;	
	filter->entry[filter->cnt].data_type = data_type;
	filter->entry[filter->cnt].str_len = str_len;
	filter->entry[filter->cnt].str = str;	

	filter->cnt++;

	return 0;
		
}

/*
	returns:
		1 -> TRUE
		0 -> FALSE
		a negative value from jer.h -> error
*/

static inline int _jdb_check_match(uchar* data, uint32_t datalen, uchar* str, uint32_t slen, uchar type){
	uint32_t cnt;
	int not;
	int size;
	int ret = -JE_TYPE;

	switch(type & JDB_FILTER_T_MASK){
		case JDB_FILTER_T_IGN:
			ret = 1;
			break;
		case JDB_FILTER_T_LT:
			if(type & JDB_FILTER_F_SIZE){
				if(datalen < slen) ret = 1;
				else ret = 0;
			} else {
				if(datalen != slen) ret = -JE_SIZE;
				else{
					if(memcmp(data, str, datalen) < 0)
						ret = 1;
					else ret = 0;
				}				
			}
			break;
		case JDB_FILTER_T_GT:
			if(type & JDB_FILTER_F_SIZE){
				if(datalen > slen) ret = 1;
				else ret = 0;
			} else {
				if(datalen != slen) ret = -JE_SIZE;
				else{
					if(memcmp(data, str, datalen) > 0)
						ret = 1;
					else ret = 0;
				}				
			}		
			break;
		case JDB_FILTER_T_EQ:
			if(type & JDB_FILTER_F_SIZE){
				if(datalen == slen) ret = 1;
				else ret = 0;
			} else {
				if(datalen != slen) ret = -JE_SIZE;
				else{
					if(!memcmp(data, str, datalen))
						ret = 1;
					else ret = 0;
				}				
			}		
			break;
		case JDB_FILTER_T_NE:
			if(type & JDB_FILTER_F_SIZE){
				if(datalen != slen) ret = 1;
				else ret = 0;
			} else {
				if(datalen != slen) ret = -JE_SIZE;
				else{
					if(memcmp(data, str, datalen) != 0)
						ret = 1;
					else ret = 0;
				}				
			}
			break;
		case JDB_FILTER_T_LE:
			if(type & JDB_FILTER_F_SIZE){
				if(datalen <= slen) ret = 1;
				else ret = 0;
			} else {
				if(datalen != slen) ret = -JE_SIZE;
				else{
					if(memcmp(data, str, datalen) <= 0)
						ret = 1;
					else ret = 0;
				}				
			}
			break;
		case JDB_FILTER_T_GE:
			if(type & JDB_FILTER_F_SIZE){
				if(datalen >= slen) ret = 1;
				else ret = 0;
			} else {
				if(datalen != slen) ret = -JE_SIZE;
				else{
					if(memcmp(data, str, datalen) >= 0)
						ret = 1;
					else ret = 0;
				}				
			}
			break;
		case JDB_FILTER_T_WILD:
			ret = -JE_IMPLEMENT;
			break;
		case JDB_FILTER_T_PART:
			if(slen > datalen) ret = -JE_TOOLONG;
			if(memmem(data, datalen, str, slen)) ret = 1;
			else ret = 0;
			break;
		default:
			break;
	}
/*
	if(type & JDB_FILTER_L_NOT){
		if(!ret) ret = 1;
		else if(ret == 1) ret = 0;
	}
*/
	return ret;
}

//returns 0 -> FALSE, 1 -> TRUE, -jeno -> ERROR
int _jdb_filter_cell(struct jdb_cell* cell, struct jdb_filter* filter){

	if(filter->col_start != JDB_ID_INVAL){
		if(	cell->celldef->col < filter->col_start ||
			cell->celldef->col > filter->col_end){

			return -JE_LIMIT;
		}
	}

	if(filter->row_start != JDB_ID_INVAL){
		if(	cell->celldef->row < filter->row_start ||
			cell->celldef->row > filter->row_end){			
			return -JE_LIMIT;
		}
	}

	//compare with filter string

	if(filter->entry[filter->cnt].col != JDB_ID_INVAL){
		if(filter->entry[filter->cnt].col != cell->celldef->col) return -JE_INV;
	}

	if(filter->entry[filter->cnt].data_type != JDB_TYPE_EMPTY){
		if(	filter->entry[filter->cnt].data_type !=
			cell->celldef->data_type) return -JE_TYPE;
	}

	return _jdb_check_match(cell->data, cell->celldef->datalen,
			filter->entry[filter->cnt].str,
			filter->entry[filter->cnt].str_len,
			filter->entry[filter->cnt].type);
	
}

static inline int _jdb_is_rc(struct jdb_rowset* rowset, uint32_t row, uint32_t col){
	uint32_t i;
	
	for(i = 0; i < rowset->n; i++)
		if(rowset->row[i] == row && rowset->col[i] == col) return 0;
	
	return -JE_NOTFOUND;
}

static inline int _jdb_check_filter_logic(struct jdb_rowset* rowset, uint32_t row, uint32_t col, uchar type){
	if(type & JDB_FILTER_L_NOT){
		return -JE_INV;	
	} else if(type & JDB_FILTER_L_AND){
		if(!_jdb_is_rc(rowset, row, col)) return 0;
		else return -JE_INV;
	} else if(type & JDB_FILTER_L_OR){
		return 0;
		
	} else if(type & JDB_FILTER_L_XOR){
		if(!_jdb_is_rc(rowset, row, col)) return -JE_INV;
		else return 0;		
	} else return -JE_TYPE;
}

int _jdb_filter_boundary(struct jdb_handle* h, struct jdb_table* table,
			struct jdb_filter* filter, struct jdb_rowset* rowset){
	struct jdb_cell* cell;

	rowset->n = 0;
	rowset->row = NULL;
	rowset->col = NULL;	
	
	for(filter->cnt = 0UL; filter->cnt < filter->nfilters; filter->cnt++){
		for(cell = table->cell_list.first; cell; cell = cell->next){
			if(!_jdb_filter_cell(cell, filter)){
				
				if(filter->cnt){
					if(_jdb_check_filter_logic(rowset,
							cell->celldef->row,
							cell->celldef->col,
							filter->entry[filter->cnt].type) < 0)
								continue;
				}
				//add to rowset
				if(!(rowset->n % h->hdr.list_buck)){
					rowset->row =(uint32_t*)realloc(
							rowset->row,
							h->hdr.list_buck*sizeof(uint32_t));
					if(!rowset->row){
						if(rowset->col) free(rowset->col);
						return -JE_MALOC;
					}
					rowset->col =(uint32_t*)realloc(
							rowset->col,
							h->hdr.list_buck*sizeof(uint32_t));
					if(!rowset->col){
						if(rowset->row) free(rowset->row);
						return -JE_MALOC;
					}
				}
				rowset->row[rowset->n] = cell->celldef->row;
				rowset->col[rowset->n] = cell->celldef->col;
				rowset->n++;
				
				
			}
		}
	}

	return 0;
}

static inline void _jdb_filter_rm_duplicate_rows(struct jdb_rowset* rowset){
	uint32_t i, j;
	for(i = 0; i < rowset->n; i++){
		if(rowset->row[i] != JDB_ID_INVAL){
			for(j = 0; j < rowset->n; j++){
				if(rowset->row[i] == rowset->row[j])
					rowset->row[i] = JDB_ID_INVAL;				
			}			
		}
	}
}

int jdb_list(struct jdb_handle* h, struct jdb_table* table, struct jdb_filter* filter){
		/* TODO (#1#): write jdb_filter() code */
		
		
}



