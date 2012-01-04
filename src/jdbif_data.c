void jdbif_dump_data_block_hdr(struct jdb_handle* h, struct jdb_data_blk_hdr hdr){
	wprintf
	    (L"\tType: 0x%02x\tFlags: 0x%02x\tDataType: 0x%02x\tCRC32: 0x%04x\n",
	     hdr.type, hdr.flags, hdr.dtype, hdr.crc32);			
}

void jdbif_dump_data_blk_info(struct jdb_data_blk* blk){
	wprintf(L"\tEntrySize: %u\tBaseSize: %u\tBitmapSize: %u\tNoOfEntries: %u\n\tDataType: 0x%02x\tBaseType: 0x%02x\tMaxEntries: %u\n",
		blk->entsize, blk->base_len, blk->bmapsize, blk->nent, blk->data_type, blk->base_type, blk->maxent);
}

void jdbif_dump_data_blk_bmap(struct jdb_data_blk* blk){
	jif_dump_bmap_w(blk->bitmap, blk->bmapsize);
}

int jdbif_dump_data_block(struct jdb_handle* h, jdb_bid_t bid){
	struct jdb_data_blk blk;
	int ret;
	struct jdb_map_blk_entry* m_ent;
	
	blk.bid = bid;
	
	m_ent = _jdb_get_map_entry_ptr(h, bid);
	if(!m_ent) return -JE_NULLPTR;

	ret = _jdb_read_data_blk(h, table, &blk, m_ent->dtype);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
		return;

	} else
		wprintf(L"\t[DONE]\n");		
	return 0;
}
