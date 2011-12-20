#ifndef _JSTRUCT_H
#define _JSTRUCT_H

#include <sys/types.h>

struct jstack_entry{
	size_t datasize;
	void* data;
	struct jstack_entry* next;
	struct jstack_entry* prev;
};

struct jstack{
	size_t max;
	size_t cnt;
	size_t fixed_data_size;
	void* data_pool;
	struct jstack_entry* entry_pool;
	struct jstack_entry* top;
};

#ifdef __cplusplus
extern "C" {
#endif

int jstack_init(struct jstack* stack, size_t fixed_data_size, size_t max);
int jstack_push(struct jstack* stack, void* data, size_t datasize, int do_alloc);
void* jstack_pop(struct jstack* stack, void** data, void* datasize, int do_free);
void jstack_destroy(struct jstack* stack, int free_data);

#ifdef __cplusplus
}
#endif


#else

#endif
