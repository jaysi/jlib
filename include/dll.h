#ifndef _DLL_H_
#define _DLL_H_

#if BUILDING_DLL
# define DLLIMPORT __declspec (dllexport)
#else /* Not BUILDING_DLL */
# define DLLIMPORT __declspec (dllimport)
#endif /* Not BUILDING_DLL */


DLLIMPORT void HelloWorld (void);

/*
	basic
*/
DLLIMPORT	uint32_t jdb_ver();
DLLIMPORT	int jdb_fstat(wchar_t * filename, wchar_t * key, uint16_t mode);
DLLIMPORT	int jdb_init(struct jdb_handle *h);
DLLIMPORT	int jdb_open(struct jdb_handle *h, wchar_t * filename, wchar_t * key,
		     uint16_t flags);
DLLIMPORT	int jdb_open2(struct jdb_handle *h, int use_default_configuration);
DLLIMPORT	int jdb_sync(struct jdb_handle *h);
DLLIMPORT	int jdb_close(struct jdb_handle *h);

/*
	table create/remove/open/close
*/
DLLIMPORT	int jdb_create_table(struct jdb_handle *h, wchar_t * name,
			     uint32_t nrows, uint32_t ncols, uchar flags,
			     uint16_t indexes);
DLLIMPORT	int jdb_rm_table(struct jdb_handle *h, wchar_t * name);
DLLIMPORT	int jdb_open_table(struct jdb_handle *h, wchar_t * name, uchar flags);
DLLIMPORT	int jdb_close_table(struct jdb_handle *h, wchar_t * name);
DLLIMPORT	int jdb_list_tables(struct jdb_handle *h, jcslist_entry ** list,
				jdb_bid_t * n);
DLLIMPORT	int jdb_sync_table(struct jdb_handle *h, wchar_t * name);
DLLIMPORT	int jdb_sync(struct jdb_handle* h);
DLLIMPORT	int jdb_find_table(struct jdb_handle *h, wchar_t * name);

/*
	table type
*/
DLLIMPORT	int jdb_add_typedef(struct jdb_handle *h,
			    wchar_t* table_name, uchar type_id,
			    uchar base, uint32_t len, uchar flags);
DLLIMPORT	int jdb_rm_typedef(struct jdb_handle *h,
			   wchar_t* table_name, uchar type_id);
	uchar jdb_find_col_type(struct jdb_handle *h,
				wchar_t* table_name, uint32_t col);
DLLIMPORT	int jdb_assign_col_type(struct jdb_handle *h,
				wchar_t* table_name, uchar type_id,
				uint32_t col);
DLLIMPORT	int jdb_rm_col_type(struct jdb_handle *h,
			    wchar_t* table_name, uint32_t col);

/*
	table cell
*/
DLLIMPORT	int jdb_create_cell(struct jdb_handle *h, wchar_t * table,
			    uint32_t col, uint32_t row,
			    uchar * data, uint32_t datalen,
			    uchar data_type, int unsign);
DLLIMPORT	int jdb_load_cell(struct jdb_handle *h, wchar_t * table,
			uint32_t col, uint32_t row, uchar** data,
			uint32_t* datalen, uchar* data_type, int* unsign);
DLLIMPORT	int jdb_rm_cell(struct jdb_handle* h, wchar_t * table,
			uint32_t col, uint32_t row);
DLLIMPORT	int jdb_update_cell(struct jdb_handle *h, wchar_t * table,
			    uint32_t col, uint32_t row,
			    uchar * data, uint32_t datalen);//you cannot change
			    					//data_type
			    					//by this call!
DLLIMPORT	int jdb_find_cell(struct jdb_handle* h, wchar_t * table,
			uchar* data, uint32_t datalen, uchar data_type,
			int unsign, uint16_t flags, uint32_t* col, uint32_t* row,
			uint32_t* n);
DLLIMPORT	int jdb_list_cells(struct jdb_handle* h, wchar_t* table_name,
			uint32_t col, uint32_t row,
			uint32_t** li_col, uint32_t** li_row, uint32_t* n);
/*
	list functions
*/	
DLLIMPORT	int jdb_list_rows(struct jdb_handle* h, wchar_t* table_name,
			struct jdb_filter* filter, uint32_t nfilter,
			uint32_t** row_id,
			uint32_t* n);

/*
	table col-row acts
*/
	
DLLIMPORT	int jdb_rm_col(struct jdb_handle* h, wchar_t* table, uint32_t col,
			int shift);
DLLIMPORT	int jdb_insert_col(struct jdb_handle* h, wchar_t* table, uint32_t col,
			int shift);
DLLIMPORT	int jdb_rm_row(struct jdb_handle* h, wchar_t* table, uint32_t row,
			int shift);
DLLIMPORT	int jdb_insert_row(struct jdb_handle* h, wchar_t* table, uint32_t row,
			int shift);
DLLIMPORT	int jdb_swap_col(struct jdb_handle* h, wchar_t* table, uint32_t col1,
			uint32_t col2);
DLLIMPORT	int jdb_swap_row(struct jdb_handle* h, wchar_t* table, uint32_t row1,
			uint32_t row2);
/*
	filters
*/
	int jdb_init_filter(	struct jdb_filter* filter, size_t n,
				uint32_t col_start,
				uint32_t col_end, uint32_t row_start,
				uint32_t row_end,
				uint32_t sort_col, void* sort_fn);
	int jdb_add_filter(	struct jdb_filter* filter, uchar type,
				uchar not, uint32_t col,
				uchar logic, uchar match, jdb_data_t data_type,
				uint32_t str_len, uchar* str);
	int jdb_list(	struct jdb_handle* h, struct jdb_table* table,
 	    		struct jdb_filter* filter, struct jdb_rowset* rowset);


#endif /* _DLL_H_ */
