#ifndef _JLIST_H
#define _JLIST_H

/*
	list type 0
	
	struct entry_s{
		struct entry_s* next;
	}entry;
*/

#define jl0_add(list, entry)	if(!list) list = entry; \
				else

/*
	list type 1
	
	struct entry_s{
		...
		struct entry_s* next;
	}entry;
	
	struct entry_list_s{
		uint32_t cnt;
		jmx_t mx;
		entry* first;
		entry* last;
	};
*/

#else

#endif
