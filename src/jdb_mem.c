#include "jdb.h"

 uchar * _jdb_get_blockbuf(struct jdb_handle * h, uchar btype)
{
	return (uchar *) malloc(h->hdr.blocksize);

}


 
void _jdb_release_blockbuf(struct jdb_handle *h, uchar * buf)
{
	
	free(buf);

} 
