#include "jdb.h"

void* _jdb_write_thread(void* arg){
	struct jdb_handle* h = (struct jdb_handle*)arg;
	int oldcancelstate;
	struct jdb_wr_fifo_entry* entry;
	//jdb_bid_t
	
	//EXIT: if !bufsize
	
	while(1){
		_wait_sem(&h->wrsem);
		_lock_mx(&h->wrmx);
		
		entry = h->wr_fifo.first;
		h->wr_fifo.first = h->wr_fifo.first->next;
		
		_unlock_mx(&h->wrmx);
		
		if(!entry->bufsize) break;
		
		_lock_mx(&h->rdmx);
		
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldcancelstate);
		
		//write here		
		
		if(entry->hdrbuf){
			_jdb_write_hdr(h);
		}
		
		i = 0;		
		for(pos = 0; pos < entry->bufsize; pos+=h->hdr.blocksize){
			_jdb_write_block(h, entry->buf + pos, entry->bid_list[i]);
			assert(i <= entry->nblocks);
			i++;
		}				
		
		pthread_setcancelstate(oldcancelstate, NULL);	
		_unlock_mx(&h->rdmx);
		
		if(entry->hdrbuf) free(entry->hdrbuf);
		free(entry->bid_list);
		free(entry->buf);
		free(entry);		

		
	}
	
	return NULL;	
}

int _jdb_init_wr_thread(struct jdb_handle* h){

	pthread_attr_t tattr;

#ifndef NDEBUG
	jmxattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif

#ifndef NDEBUG
	_init_mx(&h->wrmx, &attr);
	_init_mx(&h->rdmx, &attr);
#else
	_init_mx(&h->wrmx, NULL);
	_init_mx(&h->rdmx, NULL);
#endif

	_init_sem(&h->wrsem);
	
	pthread_attr_init(&tattr);
	pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);

	h->wr_fifo.first = NULL;

	if (_create_thread(&(h->wrthid), &tattr, _jdb_write_thread, (void *)h)
	    < 0) {
		ret = -JE_THREAD;
		goto end;
	}

	pthread_attr_destroy(&tattr);

	return ret;
		
}

int _jdb_request_wr_thread_exit(struct jdb_handle* h){
	struct jdb_wr_fifo_entry* entry;

	entry = (struct jdb_wr_fifo_entry*)malloc(sizeof(struct jdb_wr_fifo_entry));
	if(!entry) return -JE_MALOC;	
	
	entry->bufsize = 0UL;
	entry->next = NULL;

	_lock_mx(&h->wrmx);
	
	if(!h->wr_fifo.first){
		h->wr_fifo.first = entry;
		h->wr_fifo.last = entry;
		h->wr_fifo.cnt = 1UL;
	} else {
		h->wr_fifo.last->next = entry;
		h->wr_fifo.last = entry;
		h->wr_fifo.cnt++;
	}
	
	_sem_post(&h->wrsem);
	_unlock_mx(&h->wrmx);
	
	return 0;

		
}

int _jdb_request_table_write(struct jdb_handle* h, struct jdb_table* table){
	jdb_bid_t i, nbids;	
	struct jdb_wr_fifo_entry* entry;
	struct jdb_cell_data_blk* data_blk;
	struct jdb_cell_data_ptr_blk* data_ptr_blk;
	struct jdb_celldef_blk* celldef_blk;
	struct jdb_col_typedef* col_typedef_blk;
	struct jdb_typedef_blk* typedef_blk;
	struct jdb_fav_blk* fav_blk;
	struct jdb_map* map;
	
	int ret = 0, ret2;	

	entry = (struct jdb_wr_fifo_entry*)malloc(sizeof(struct jdb_wr_fifo_entry));
	if(!entry) return -JE_MALOC;

	nbids = table->map_chg_list_size + table->nwrblk;

	entry->bid_list = nbids*sizeof(jdb_bid_t);
	if(!entry->bid_list){
		free(entry);
		return -JE_MALOC;
	}

	if(h->flags & JDB_HMODIF){
		entry->hdrbuf = (uchar*)malloc(JDB_HDR_SIZE);
		if(!entry->hdrbuf){
			free(entry);
			free(entry->bid_list);
			return -JE_MALOC;
		}
	} else entry->hdrbuf = NULL;
	
	entry->nblocks = nbids;
	entry->bufsize = (nbids)*h->hdr.blocksize;
	entry->buf = (uchar*)malloc(entry->bufsize);	
	if(!entry->buf){
		if(entry->hdrbuf){
			free(entry->hdrbuf);
		}
		free(entry->bid_list);
		free(entry);
		return -JE_MALOC;
	}
	
	entry->next = NULL;	

	i = 0UL;
	
	for(data_blk = table->data_list.first; data_blk; data_blk = data_blk->next){
		if(data_blk->write){
			_jdb_pack_data(h, data_blk, entry->buf + (i*h->hdr.blocksize));
			entry->bid_list[i] = data_blk->bid;
			i++;
			data_blk->write = 0;
		}
	}

	for(data_ptr_blk = table->data_ptr_list.first; data_ptr_blk; data_ptr_blk = data_ptr_blk->next){
		if(data_ptr_blk->write){
			_jdb_pack_data_ptr(h, data_ptr_blk, entry->buf + (i*h->hdr.blocksize));
			entry->bid_list[i] = data_ptr_blk->bid;
			i++;
			data_ptr_blk->write = 0;
		}
	}	
	
	for(celldef_blk = table->celldef_list.first; celldef_blk; celldef_blk = celldef_blk->next){
		if(celldef_blk->write){
			_jdb_pack_celldef(h, celldef_blk, entry->buf + (i*h->hdr.blocksize));
			entry->bid_list[i] = celldef_blk->bid;
			i++;
			celldef_blk->write = 0;
		}
	}
	
	for(col_typedef_blk = table->col_typedef_list.first; col_typedef_blk; col_typedef_blk = col_typedef_blk->next){
		if(col_typedef_blk->write){
			_jdb_pack_col_typedef(h, col_typedef_blk, entry->buf + (i*h->hdr.blocksize));
			entry->bid_list[i] = col_typedef_blk->bid;
			i++;
			col_typedef_blk->write = 0;
		}
	}

	for(typedef_blk = table->typedef_list.first; typedef_blk; typedef_blk = typedef_blk->next){
		if(typedef_blk->write){
			_jdb_pack_typedef(h, typedef_blk, entry->buf + (i*h->hdr.blocksize));
			entry->bid_list[i] = typedef_blk->bid;
			i++;
			typedef_blk->write = 0;
		}
	}

	for(fav_blk = table->fav_list.first; fav_blk; fav_blk = fav_blk->next){
		if(fav_blk->write){
			_jdb_pack_fav(h, fav_blk, entry->buf + (i*h->hdr.blocksize));
			entry->bid_list[i] = fav_blk->bid;
			i++;
			fav_blk->write = 0;		
		}
	}	
	
	if(table->main.write){
			_jdb_pack_table_def(h, &table->main, entry->buf + (i*h->hdr.blocksize));
			entry->bid_list[i] = table->tdef_main_bid;
			i++;
			table->main.write = 0;
	}
	
	for(j = 0; j < table->map_chg_list_size; j++){
		for(map = table->map_list.first; map; map = map->next){
			if(map->bid == table->map_chg_list[j]){
				_jdb_pack_map(h, map, entry->buf + (i*h->hdr.blocksize));
				entry->bid_list[i] = table->map_chg_list[j];		
				i++;
			}			
		}
	}
	
	_lock_mx(&h->wrmx);
	
	if(!h->wr_fifo.first){
		h->wr_fifo.first = entry;
		h->wr_fifo.last = entry;
		h->wr_fifo.cnt = 1UL;
	} else {
		h->wr_fifo.last->next = entry;
		h->wr_fifo.last = entry;
		h->wr_fifo.cnt++;
	}
	
	_sem_post(&h->wrsem);
	_unlock_mx(&h->wrmx);
	
	return 0;
}


