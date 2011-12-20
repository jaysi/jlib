#include "jstruct.h"
#include "jer.h"
/*
	max -> 0 => not limited stack
	fixed_data_size -> 0 => variable data size
*/
int jstack_init(struct jstack* stack, size_t fixed_data_size, size_t max){
	stack->cnt = 0UL;
	stack->top = NULL;
	if(max){
		stack->entry_pool = (struct jstack_entry*)malloc(
					sizeof(struct jstack_entry)*max);
		if(!stack->entry_pool) return -JE_MALOC;
		if(fixed_data_size){
			stack->data_pool = malloc(fixed_data_size*max);
			if(!stack->data_pool){
				free(stack->entry_pool);
				return -JE_MALOC;
			}
			for(stack->cnt = 0UL; stack->cnt < max; stack->cnt++){
				stack->
			}
		} else stack->data_pool = NULL;
	}
	
	stack->max = max;
	stack->fixed_data_size = fixed_data_size;

	return 0;
	
}

int jstack_push(struct jstack* stack, void* data, size_t datasize, int do_alloc){

	size_t i;

	if(stack->max){
		return -JE_IMPLEMENT;
		if(stack->cnt == stack->max) return -JE_LIMIT;
		if(stack->fixed_data_size)
			
		
	}
	
	entry = (struct jstack_entry*)malloc(sizeof(struct jstack_entry));
	if(!entry) return -JE_MALOC;
	if(!stack->top){
		entry->prev = NULL;
		entry->next = NULL;
		stack->top = entry;
		stack->cnt = 1UL;
	} else {
		entry->prev = NULL;
		entry->next = stack->top;
		stack->top->prev = entry;
		stack->top = entry;
		stack->cnt++;
	}

	return 0;
}

int jstack_pop(struct jstack* stack, void** data, void* datasize, int do_free){
	struct jstack_entry* entry;
	if(!stack->cnt) return -JE_EMPTY;
	entry = stack->top;
	stack->top = stack->top->next;
	stack->cnt--;
	*data = entry->data;
	*datasize = entry->datasize;
	if(do_free && !stack->fixed_data_size && entry->datasize)
		free(entry->data);
	if(!stack->max) free(entry);
	return 0;
}

void jstack_destroy(struct jstack* stack, int free_data){
	struct jstack_entry* entry;
	entry = stack->top;
	while(entry){
		if(free_data && entry->datasize && entry->data) free(entry->data);
		free(entry);
		stack->top = stack->top->next;
		entry = stack->top;
	}	
}
