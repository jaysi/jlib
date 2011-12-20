#include "jnet.h"
#include "jcs.h"
/*
int jn_block_chk(jn_h * h, wchar_t * name, struct sockaddr *sa)
{
	jn_block_list *entry;
	_lock_mx(&h->block_list->mx);
	for (entry = h->block_list->first; entry; entry = entry->next) {
		if (name) {
			if (!wcscmp(name, entry->name)) {
				_unlock_mx(&h->block_list->mx);
				return 0;
			}
		} else {
			if (entry->sa.sa_family == sa->sa_family) {
				if ((struct sockaddr_in *)&entry->sa.
				    sa_family == AF_INET) {
					if (entry->sa.sin_addr ==
					    (struct sockaddr_in *)sa->
					    sin_addr) {
						_unlock_mx(&h->block_list->mx);
						return 0;
					}
				} else {
				if (}
				    }
				    }
				    }
				    _unlock_mx(&h->block_list->mx);
				    return -JE_NOTFOUND;}

				    void jn_block_add(jn_h * h, wchar_t * name,
						      struct sockaddr *sa) {
				    jn_block_list_entry * entry;
				    if (!jn_block_chk(h, name, sa)) return;
				    entry =
				    (jn_block_list_entry *)
				    malloc(sizeof(jn_block_list_entry));
				    if (!entry) return; entry->next = NULL;
				    if (name) {
				    entry->name =
				    (wchar_t *) malloc(WBYTES(name));
				    if (!entry->name) {
				    free(entry); return;}
				    wcscpy(entry->name, name); entry->ip = 0L;}
				    else {
				    entry->ip = ip; entry->name = NULL;}
				    _lock_mx(&h->block_list->mx);
				    if (!h->block_list->first) {
				    h->block_list->first = entry;
				    h->block_list->last = entry;}
				    else {
				    h->block_list->last->next = entry;
				    h->block_list->last =
				    h->block_list->last->next;}
				    _unlock_mx(&h->block_list->mx);}

				    void jn_block_rm(jn_h * h, wchar_t * name,
						     struct sockaddr *sa) {
				    jn_block_list_entry * prev, *delete;
				    _lock_mx(&h->block_list->mx); if (name) {
				    if (!wcscmp(name, h->block_list->first->name)) {	//first entry
				    delete = h->block_list->first;
				    h->block_list->first =
				    h->block_list->first->next; free(delete);
				    _unlock_mx(&h->block_list->mx); return;}

				    for (delete = h->block_list->first; delete;
					 delete = delete->next) {
				    if (!wcscmp(delete->name, name)) {
				    prev->next = delete->next;
				    free(delete->name); free(delete);}
				    prev = delete;}
				    }
				    else {
				    if (ip == h->block_list->first->ip) {	//first entry
				    delete = h->block_list->first;
				    h->block_list->first =
				    h->block_list->first->next; free(delete);
				    _unlock_mx(&h->block_list->mx); return;}

				    for (delete = h->block_list->first; delete;
					 delete = delete->next) {
				    if (delete->ip == ip) {
				    prev->next = delete->next; free(delete);}
				    prev = delete;}
				    }
				    _unlock_mx(&h->block_list->mx);}

				    void jn_block_empty(jn_h * h) {
				    jn_block_list_entry * entry;
				    _lock_mx(&h->block_list->mx);
				    entry = h->block_list->first;
				    while (entry) {
				    h->block_list->first =
				    h->block_list->first->next;
				    if (entry->name) free(entry->name);
				    free(entry); entry = h->block_list->first;}
				    _unlock_mx(&h->block_list->mx);}
*/
