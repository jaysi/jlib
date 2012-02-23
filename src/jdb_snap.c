#include <stdarg.h>
#include "jdb.h"

int _jdb_changelog_table_snapshot(	struct jdb_handle* h, wchar_t* name){	
	return -JE_IMPLEMENT;
}

int _jdb_changelog_typedef_snapshot(	struct jdb_handle* h, wchar_t* tname,
					jdb_data_t type_id){
	
	return -JE_IMPLEMENT;
}

int _jdb_changelog_coltype_snapshot(	struct jdb_handle* h, wchar_t* tname,
					uint32_t col){
	return -JE_IMPLEMENT;
}

int _jdb_changelog_cell_snapshot(	struct jdb_handle* h, wchar_t* tname,
					uint32_t col, uint32_t row){
	return -JE_IMPLEMENT;
}
