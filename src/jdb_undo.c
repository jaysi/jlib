#include "jdb.h"

static inline void _jdb_add_to_changelog_list(struct jdb_changelog_list* list,
					struct jdb_changelog_entry* entry){
	if(!list->first){
		list->first = entry;
		list->last = entry;
		list->cnd = 1UL;
	} else {
		list->last->next = entry;
		list->last = list->last->next;
		list->cnt++;
	}
}

static inline void _jdb_add_to_undo_list(struct jdb_handle* h,
					struct jdb_changelog_entry* entry){
	_jdb_add_to_changelog_list(&h->undo_list, entry);
}

static inline void _jdb_add_to_redo_list(struct jdb_handle* h,
					struct jdb_changelog_entry* entry){
	_jdb_add_to_changelog_list(&h->redo_list, entry);
}

int _jdb_changelog_save_table_undo_rec(	struct jdb_handle* h, wchar_t* name){
	return -JE_IMPLEMENT;
}

int _jdb_changelog_save_rm_typedef_undo_rec(	struct jdb_handle* h,
						wchar_t* tname,
						jdb_data_t type_id,
						struct jdb_changelog_entry** changelog_ent){

	struct jdb_table* table;
	struct jdb_typedef_blk *blk;
	struct jdb_typedef_blk_entry *entry, *saved;
	struct jdb_changelog_entry* chent;
	uchar *buf;
	size_t bufsize;	
		
	if((ret = _jdb_table_handle(h, tname, &table)) < 0){		
		return ret;
	}	
	
	if ((ret = _jdb_find_typedef(h, table, type_id, &blk, &entry)) < 0){
		return ret;
	}
	
	chent = (struct jdb_changelog_entry*)malloc(sizeof(struct jdb_changelog_entry));
	if(!chent) return -JE_MALOC;
	chent->next = NULL;
	
	if((ret = _jdb_changelog_assm_rec(&buf, &bufsize, h, 
	
	_jdb_add_to_undo_list(h, chent);
	
	*changelog_ent = chent;	
	
	return 0;
}

int _jdb_changelog_save_coltype_undo_rec(	struct jdb_handle* h,
						wchar_t* tname,
						uint32_t col,
						struct jdb_changelog_entry** changelog_ent){
	jdb_data_t coltype;
	struct jdb_changelog_entry* chent;
	
	coltype = jdb_find_col_type(h, tname, col);
	if(coltype == JDB_TYPE_EMPTY) return -JE_NOTFOUND;

	chent = (struct jdb_changelog_entry*)malloc(sizeof(struct jdb_changelog_entry));
	if(!chent) return -JE_MALOC;
	chent->next = NULL	
	chent->rec.buf = (uchar*)malloc(sizeof(uint32_t)+ sizeof(jdb_data_t) +WBYTES(tname));
	if(!chent->rec.buf) { free(chent); return -JE_MALOC; }	
	
	memcpy((void*)(chent->rec.buf), (void*)&col, sizeof(uint32_t));
	memcpy(	(void*)(chent->rec.buf + sizeof(uint32_t)), (void*)&coltype,
		sizeof(jdb_data_t));
	memcpy((void*)(chent->rec.buf + sizeof(uint32_t) + sizeof(jdb_data_t)),
		(void*)&coltype, sizeof(jdb_data_t));
		
	*changelog_ent = chent;
	
	return 0;
}

int _jdb_changelog_save_cell_undo_rec(	struct jdb_handle* h, wchar_t* tname,
					uint32_t col, uint32_t row){
	return -JE_IMPLEMENT;
}

int _jdb_undo_handle_create_table(	struct jdb_handle* h,
					struct jdb_changelog_rec* rec					
					){
	struct jdb_changelog_entry* entry;
	entry = (struct jdb_changelog_entry*)malloc(
					sizeof(struct jdb_changelog_entry));
	if(!entry){
		return -JE_MALOC;
	}
	entry->next = NULL;

	return -JE_IMPLEMENT;
}

int _jdb_undo_reg(struct jdb_handle* h, struct jdb_changelog_rec* rec){
	
	struct jdb_changelog_rec* prev_rec;
	int ret;	
	
	//store the previous record;
	switch(rec->hdr.cmd & JDB_CMD_MASK){
		case JDB_CMD_CREATE_TABLE:
			if((ret = _jdb_undo_handle_create_table(h, rec)) < 0)
				return ret;
			
			break;
		case JDB_CMD_RM_TABLE:
			_jdb_undo_handle_rm_table(h, rec);
			break;
		case JDB_CMD_ADD_TYPEDEF:
			break;
		case JDB_CMD_RM_TYPEDEF:
			break;
		case JDB_CMD_ADD_COLTYPE:
			break;
		case JDB_CMD_RM_COLTYPE:
			break;
		case JDB_CMD_ADD_CELL:
			break;
		case JDB_CMD_RM_CELL:
			break;
		case JDB_CMD_UPDATE_CELL:
			break;
		default://if command not supported, release everything
			_jdb_free_changelog_rec(rec);
			free(rec);
			break;
	}
	
	return 0;
}

int _jdb_undo_to_redo(struct jdb_handle* h){
	struct jdb_changelog_entry* prev;
	
	if(!h->undo_list.first) return 0;
	
	_jdb_add_to_redo_list(h, h->undo_list.last);
	
	if(h->undo_list.first == h->undo_list.last){
		h->undo_list.first = NULL;		
	} else {
		prev = h->undo_list.first;
		while(prev->next)
			prev = prev->next;
		
		h->undo_list.last = prev;
		h->undo_list.cnt--;
	}
	
	return 0;
	
}

int _jdb_redo_to_undo(struct jdb_handle* h){
	struct jdb_changelog_entry* prev;
	
	if(!h->redo_list.first) return 0;
	
	_jdb_add_to_undo_list(h, h->redo_list.last);
	
	if(h->redo_list.first == h->redo_list.last){
		h->redo_list.first = NULL;		
	} else {
		prev = h->redo_list.first;
		while(prev->next)
			prev = prev->next;
		
		h->redo_list.last = prev;
		h->redo_list.cnt--;
	}
	
	return 0;
	
}

int _jdb_undo_rm_last(struct jdb_handle* h){
	struct jdb_changelog_entry* prev, *del;
	
	if(!h->undo_list.first) return 0;
	
	del = h->undo_list.last;
	
	if(h->undo_list.first == h->undo_list.last){
		h->undo_list.first = NULL;		
	} else {
		prev = h->undo_list.first;
		while(prev->next)
			prev = prev->next;
		
		h->undo_list.last = prev;
		h->undo_list.cnt--;
	}
	
	_jdb_free_changelog_rec(del->rec);
	_jdb_free_changelog_rec(del->prev_rec);
	free(del->rec);
	free(del->prev_rec);
	
	return 0;
	
}

int jdb_undo(struct jdb_handle* h){

}

int jdb_redo(struct jdb_handle* h){

}
