#include "jdb.h"

#define SN_ENT_BUCK	10

int _jdb_create_snapshot(struct jdb_handle *h)
{
	struct jdb_snapshot_blk *blk;

	_wdeb_type(L"called");

	blk = (struct jdb_snapshot_blk*)malloc(sizeof(struct jdb_snapshot_blk));
	if (!blk)
		return -JE_MALOC;
	blk->entry = NULL;
	/*
	blk->entry = (struct jdb_snapshot_blk_entry *)
	    malloc(h->hdr.blocksize - sizeof(struct jdb_snapshot_blk_hdr));

	if (!blk->entry) {
		free(blk);
		return -JE_MALOC;
	}
	*/
	
	blk->datapool = (uchar*) malloc(h->hdr.blocksize);
	if (!blk->datapool) {
		free(blk);
		return -JE_MALOC;
	}
	
	blk->bid =
	    _jdb_get_empty_map_entry(h, JDB_BTYPE_SNAPSHOT, 0,
				     JDB_ID_INVAL, 0);

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

	memset(&blk->hdr, 0, sizeof(struct jdb_snapshot_blk_hdr));

	blk->hdr.type = JDB_BTYPE_SNAPSHOT;
	blk->nsupernodes = 0;
	blk->total = 0;
	blk->next = NULL;

	if (!h->snapshot_list.first) {
		h->snapshot_list.first = blk;
		h->snapshot_list.last = blk;
		h->snapshot_list.cnt = 1UL;
	} else {
		h->snapshot_list.last->next = blk;
		h->snapshot_list.last = blk;
		h->snapshot_list.cnt++;
	}
	return 0;
}

static inline int _jdb_create_diff_list(	uchar* oldblock,
						uchar* newblock,
						jdb_bsize_t bsize,
						struct jdb_snapshot_node**
								nodelist,
						uint16_t* cnt,
						uint16_t* totaldatasize
					){	
	uint16_t i;
	uint16_t n = 0;
	uint16_t asize;
	//uint16_t totaldatasize;	
	struct jdb_snapshot_node *node;
		
	asize = SN_ENT_BUCK;
	node = (struct jdb_snapshot_node*)malloc(
			SN_ENT_BUCK*sizeof(struct jdb_snapshot_node));
	if(!node) { return -JE_MALOC; }
	
	*totaldatasize = 0;
	n = 0;		
	for(i = 0; i < bsize; i++){
		if(oldblock[i] != newblock[i]){
			if(i){
				if(oldblock[i-1] != newblock[i-1]){
					//continue
					node[n-1].hdr.datalen++;
					(*totaldatasize)++;
				} else {//GAP
					n++;
					if(asize <= n){
						asize += SN_ENT_BUCK;
						node = (struct jdb_snapshot_node*)realloc(
							node,
							asize*sizeof(struct jdb_snapshot_node));
						if(!node) return -JE_MALOC;
					}
					node[n-1].hdr.blk_off = i;
					node[n-1].hdr.datalen = 1;
				}
			} else { //init stat
				n = 1;
				node[n-1].hdr.blk_off = 0;
				node[n-1].hdr.datalen = 1;
				*totaldatasize = 1;
			}
		}		
	}
	
	//handle special cases
	
	if(!n){	
		//means blocks are same!
		return -JE_NOTFOUND;
	}
	
	/*
	*totalsupernodesize = 	sizeof(struct jdb_snapshot_supernode_hdr) +
				(n*sizeof(struct jdb_snapshot_node_hdr)) + 
				totaldatasize;
	
	if(*totalsupernodesize > bsize - sizeof(struct jdb_snapshot_blk_hdr){
		//means that supernode is too big to fit in one block
	}
	*/
		
	*nodelist = node;
	*cnt = n;
		
	return 0;
}

void _jdb_compact_diff_list(	
				jdb_bsize_t bsize,
				struct jdb_snapshot_node* node,
				uint16_t n, uint16_t* c, uint32_t* totaldatasize
			){
	/*
	  rule:
		if the gap between two following entries is smaller than
		the size of hdr, join two segments
	*/
	
	uint16_t i;
		
	*c = n;
	*totaldatasize = 0;
	
	for(i = 1; i < n; i++){		
		if(	(node[i-1].hdr.blk_off != JDB_BENT_INVAL) &&
		
			((node[i].hdr.blk_off - node[i-1].hdr.blk_off) <
			 sizeof(struct jdb_snapshot_node_hdr)) &&
			 
			((node[i].hdr.blk_off - node[i-1].hdr.blk_off +
			 node[i].hdr.datalen) < 
			 bsize -
			 sizeof(struct jdb_snapshot_blk_hdr) -
			 sizeof(struct jdb_snapshot_blk_node_hdr))

		  ){
			node[i-1].hdr.datalen = node[i].hdr.blk_off -
						 node[i-1].hdr.blk_off +
						 node[i].hdr.datalen;

			node[i].hdr.blk_off = JDB_BENT_INVAL;
			(*c)--;
			*totaldatasize += node[i-1].hdr.datalen;
		} else if(node[i-1].hdr.blk_off != JDB_BENT_INVAL){
			*totaldatasize += node[i].hdr.datalen;
		}				
	}	
}

static inline void _jdb_free_snapshot_blks(struct jdb_snapshot_blk* first){
	
	struct jdb_snapshot_blk* del, *next;
	
	del = first;
	next = del;
	
	while(next){
		next = del->next;
		if(del->node) free(del->node);
		free(del);
		del = next;
	}
	
}

int _jdb_create_supernode_hdr(	
				struct jdb_snapshot_node* node,
				uint16_t block_empty_space,
				uint16_t node_i,
				uint16_t n,
				struct jdb_snapshot_supernode_hdr* shdr,
				uint16_t* last_i
				){
	
	*last_i = node_i;
	
	if(
		block_empty_space <
		sizeof(struct jdb_snapshot_supernode_hdr) +
		sizeof(struct jdb_snapshot_node_hdr) +
		node[node_i].hdr.datalen
	){
		return JE_NEXT_BLOCK;
	}
	
	shdr->total = 0;
	shdr->nodes = 0;
	
	block_empty_space -= sizeof(struct jdb_snapshot_supernode_hdr);
	
	while(
		block_empty_space > 
		sizeof(struct jdb_snapshot_node_hdr) +
		node[*last_i].hdr.datalen
	){
		shdr->total += 	sizeof(struct jdb_snapshot_node_hdr) + 
				node[*last_i].hdr.datalen;
		shdr->nodes++;
		
		block_empty_space -= 	sizeof(struct jdb_snapshot_node_hdr) +
					node[*last_i].hdr.datalen;		
		
		if(*last == n-1) break;			
		(*last_i)++;
	}
	
	return 0;
}

int _jdb_assign_snapshot_nodes(	
				struct jdb_handle* h,
				uchar* oldblock,
				struct jdb_snapshot_node* node,
				jdb_bent_t n
				){
	struct jdb_snapshot_blk* blk;
	int ret;
	jdb_bent_t i;
	struct jdb_snapshot_supernode* supernode;
	
	if(!n) return 0;
	
	if(!h->snapshot_list.first){
		if((ret = _jdb_create_snapshot(h)) < 0) return ret;
	}
	
	blk = h->snapshot_list.last;
	
	i = 0;
	while(i < n){
		while((h->hdr.blocksize - blk->total) >
		      (sizeof(struct jdb_snapshot_node_hdr) +
		       node[i].hdr.datalen)
		){
			if(blk == h->snapshot_list.last && !i){
				//create supernode here
			}
			
			pack(blk->datapool + blk->total, "hh",
			     node[i].hdr.blk_off,
			     node[i].hdr.datalen);
			blk->total += sizeof(struct jdb_snapshot_node_hdr);
			memcpy( blk->datapool + blk->total,
				oldblock + node[i].hdr.blk_off,
				node[i].hdr.datalen);
			blk->total += node[i].hdr.datalen;
			i++;
		}
		
		if((ret = _jdb_create_snapshot(h)) < 0){
			//create supernode...
			return ret;
		}
		blk = h->snapshot_list.last;
	}

	return 0;
}

int _jdb_create_snapshot(struct jdb_handle *h, uchar* block, jdb_bid_t bid,
		     	uchar flags){
	uchar* oldblock;
	int ret;
	struct jdb_snapshot_blk_entry* ent;
	uint16_t n, c;
	uint32_t total;

	oldblock = _jdb_get_blockbuf(h);
	if(!oldblock) return -JE_MALOC;

	if((ret = _jdb_read_block(h, oldblock, bid)) < 0){
		_jdb_release_blockbuf(oldblock);
		return ret;
	}

	if((ret = _jdb_create_diff_list(oldblock, block, h->hdr.blocksize,
					    &ent, &n)) < 0){
		_jdb_release_blockbuf(oldblock);
		return ret;
	}

	_jdb_compact_diff_list(h->hdr.blocksize, ent, n, &c, &total);

	_jdb_assign_snapshot_entries(h, oldblock, ent, n);

	free(ent);

	return 0;	
}

int _jdb_rewind_snapshot_block(	uchar* block, jdb_bsize_t bsize,
				struct jdb_snapshot_blk* first){
	
}

int jdb_rewind_snapshot(struct jdb_handle* h){
	if(!(h->hdr.flags & JDB_O_SNAP)){
		
	}
}

int jdb_forward_snapshot(struct jdb_handle* h){
	if(!(h->hdr.flags & JDB_O_SNAP)){
		
	}	
}
