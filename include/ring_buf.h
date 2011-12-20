#ifndef _RING_BUF_H
#define _RING_BUF_H

#include <stdlib.h>

//from wikipedia

struct ring_buffer {
	char *address;

	unsigned long count_bytes;
	unsigned long write_offset_bytes;
	unsigned long read_offset_bytes;
};

#define ring_buffer_create(buffer, count_bytes)	\
  buffer->count_bytes = count_bytes;			\
  buffer->write_offset_bytes = 0;			\
  buffer->read_offset_bytes = 0;			\
  buffer->address = (char*)malloc(buffer->count_bytes)

#define ring_buffer_free(buffer) \
	free(buffer->address)

#define ring_buffer_write_address(buffer) \
	(buffer->address + buffer->write_offset_bytes)

#define ring_buffer_write_advance(buffer, count_bytes) \
	(buffer->write_offset_bytes += count_bytes)

#define ring_buffer_read_address(buffer) \
  	(buffer->address + buffer->read_offset_bytes)

#define ring_buffer_read_advance(buffer, bytes)	\
  buffer->read_offset_bytes += bytes;		\
  if (buffer->read_offset_bytes >= buffer->count_bytes)	\
    {							\
      buffer->read_offset_bytes -= buffer->count_bytes;	\
      buffer->write_offset_bytes -= buffer->count_bytes;\
    }

#define ring_buffer_count_bytes(buffer)	\
	(buffer->write_offset_bytes - buffer->read_offset_bytes)

#define ring_buffer_count_free_bytes(buffer)	\
	(buffer->count_bytes - ring_buffer_count_bytes(buffer))

void
ring_buffer_write(struct ring_buffer *buffer, char *data, unsigned long *size);

void
ring_buffer_read(struct ring_buffer *buffer, char *data, unsigned long *size);

#endif
