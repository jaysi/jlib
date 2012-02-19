#include "jdb_intern.h"

//journal open flags
#define JDB_JRNL_TRUNC	(0x0001<<0)

struct jdb_wr_fifo_entry{
	jdb_bid_t nblocks;
	jdb_bid_t* bid_list;
	uint32_t bufsize;
	uchar* hdrbuf;
	uchar* buf;
	struct jdb_wr_fifo_entry* next;
};

struct jdb_wr_fifo{
	uint32_t cnt;
	struct jdb_wr_fifo_entry* first, *last;
};

struct jdb_jrnl_fifo_entry{
	/*
	uchar cmd;
	uint32_t nargs;
	uint32_t* arg_size;
	uchar* arg;
	*/
	size_t bufsize;
	uchar* buf;
	struct jdb_jrnl_fifo_entry* next;
};

struct jdb_jrnl_fifo{
	uint32_t cnt;
	struct jdb_jrnl_fifo_entry* first, *last;
};


/*
	JOURNALLing
		begin_record> REC_SIZE:OP_ID:EDIT_COMMAND:NARGS:ARG_SIZE_LIST:ARG_LIST
		end_record> REC_SIZE:OP_ID:END_COMMAND
		
		*every begin_record MUST have a matching end_record to ensure
		the operation has been finished successfully
			
*/

struct jdb_jrnl_rec_hdr{
	uint32_t recsize;
	uint64_t chid;//header's change id [i.e. sync between DB & JRNL]
	uchar cmd;
	int ret;
}__attribute__((packed));

struct jdb_jrnl_rec{
	struct jdb_jrnl_rec_hdr hdr;
	uchar nargs;
	uchar* buf;
	uint32_t* argsize;
	uchar* arg;
	struct jdb_jrnl_rec* next;
}__attribute__((packed));

//open
#define JDB_CMD_OPEN			0x00

//table
#define JDB_CMD_CREATE_TABLE		0x01
#define JDB_CMD_RM_TABLE		0x02

//type
#define JDB_CMD_ADD_TYPEDEF		0x03
#define JDB_CMD_RM_TYPEDEF		0x04
#define JDB_CMD_ASSIGN_COLTYPE		0x05

//cell
#define JDB_CMD_CREATE_CELL		0x06
#define JDB_CMD_RM_CELL			0x07
#define JDB_CMD_UPDATE_CELL		0x08

//row/col ops
#define JDB_CMD_RM_COL			0x09
#define JDB_CMD_INSERT_COL		0x0a
#define JDB_CMD_RM_ROW			0x0b
#define JDB_CMD_INSERT_ROW		0x0c
#define JDB_CMD_SWAP_COL		0x0d
#define JDB_CMD_SWAP_ROW		0x0e

//endings
#define JDB_CMD_CLOSE			0x0f	//closing file
#define JDB_CMD_END			0x10	//operation ends

//journalling special
#define JDB_CMD_EMPTY			0x11
#define JDB_CMD_FAIL			0x80	//shows failure of the command

//exit
#define JDB_CMD_EXIT			0x12

