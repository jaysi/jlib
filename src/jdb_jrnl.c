#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "jdb.h"

static inline wchar_t *_jdb_jrnl_filename(struct jdb_handle *h,
					   wchar_t * filename)
{
	wchar_t * jfile;	
 
	jfile = (wchar_t *) malloc(WBYTES(filename) + WBYTES(JDB_DEF_JRNL_EXT));
	
	if (!jfile)
		return NULL;
	
 
	wcscpy(jfile, filename);	
	wcscat(jfile, JDB_DEF_JRNL_EXT);
	
	return jfile;
}


 
int _jdb_jrnl_open(struct jdb_handle *h, wchar_t * filename, uint16_t flags)
{
 
	wchar_t * jfilename;
 
	jfilename = _jdb_jrnl_filename(h, filename);
	
	if (!jfilename)
		return -JE_NOOPEN;

	return -JE_IMPLEMENT;

}


 
int _jdb_jrnl_close(struct jdb_handle *h)
{
	return -JE_IMPLEMENT;
}


 
int _jdb_jrnl_reg_open(struct jdb_handle *h, wchar_t * filename,
			  uint16_t flags)
{
	return -JE_IMPLEMENT;
}


 
int _jdb_jrnl_reg_close(struct jdb_handle *h)
{
	return -JE_IMPLEMENT;

}


 
int _jdb_jrnl_reg_create_table(struct jdb_handle *h, wchar_t * name,
				  uint32_t nrows, uint32_t ncols, uchar flags)
{
	return -JE_IMPLEMENT;

}


 
int _jdb_jrnl_reg_rm_table(struct jdb_handle *h, wchar_t * name)
{
	return -JE_IMPLEMENT;

}


 
int _jdb_jrnl_reg_add_typedef(struct jdb_handle *h, struct jdb_table *table,
				 uchar type_id, uchar base, uint32_t len,
				 uchar flags)
{
	return -JE_IMPLEMENT;

}


 
int _jdb_jrnl_reg_rm_typedef(struct jdb_handle *h, struct jdb_table *table,
				uchar type_id)
{
	return -JE_IMPLEMENT;
}


 
int _jdb_jrnl_reg_assign_col_type(struct jdb_handle *h,
				     struct jdb_table *table, uchar type_id,
				     uint32_t col)
{
	return -JE_IMPLEMENT;
}


 
int _jdb_jrnl_reg_create_cell(struct jdb_handle *h, wchar_t * table,			 
				uint32_t col, uint32_t row, 
				uchar * data,
				uint32_t datalen, 
				uchar data_type, int unsign)
{ 
	return -JE_IMPLEMENT;
}


 
 
