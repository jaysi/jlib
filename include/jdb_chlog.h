#include "jdb_intern.h"

struct jdb_jrnl_hdr{
	uint32_t argbufsize;
	uint64_t chid;//header's change id [i.e. sync between DB & JRNL]
	uchar cmd;
	int ret;
	uchar nargs;
}__attribute__((packed));

struct jdb_jrnl_rec{
	struct jdb_jrnl_hdr hdr;
	void* argbuf;	
	uint32_t* argsize;
	void* arg;
};

struct jdb_jrnl_entry{
	struct jdb_jrnl_rec* rec;
	struct jdb_jrnl_rec* prev_rec;
	struct jdb_jrnl_entry* next;
};

struct jdb_jrnl_list{
	uint32_t cnt;
	struct jdb_jrnl_entry* first;
	struct jdb_jrnl_entry* last;
};

struct jdb_snapshot_blk_hdr{
	uchar type;
	uchar flags;
	
	uint32_t crc32;		//crc of the block including header
}__attribute__((packed));

struct jdb_snapshot_blk_node_hdr{
	uint16_t blk_off;
	uint16_t datalen;
}__attribute__((packed));

struct jdb_snapshot_blk_node{
	struct jdb_snapshot_blk_node_hdr hdr;	
	uchar* data;
};

struct jdb_snapshot_supernode_hdr{
	jdb_bid_t bid;
	uint64_t chid;
	uchar flags;
	uint16_t nodes;
	uint16_t total;//total node_hdr + data
}__attribute__((packed));

struct jdb_snapshot_supernode{
	struct jdb_snapshot_supernode_hdr hdr;
	struct jdb_snapshot_blk_node* node;
	struct jdb_snapshot_supernode* next;
};

struct jdb_snapshot_blk{
	//uint32_t write; always write
	jdb_bid_t bid;	
	jdb_bent_t total;
	jdb_bent_t nsupernodes;
	struct jdb_snapshot_supernode* supernode;
	struct jdb_snapshot_blk* next;	
};

struct jdb_snapshot_list {
	jdb_bid_t cnt;
	struct jdb_snapshot_blk *first;
	struct jdb_snapshot_blk *last;
};

//journal open flags
#define JDB_JRNL_TRUNC	(0x0001<<0)

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

/*
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
*/

//table
#define JDB_CMD_CREATE_TABLE		0x01
struct jdb_jrnl_create_table{
	wchar_t* name;
	uint32_t* nrows;
	uint32_t* ncols;
	uint32_t* flags;
	uint32_t* indexes;
};
#define JDB_CMD_RM_TABLE		0x02
struct jdb_jrnl_rm_table{
	wchar_t* name;
};

//type
#define JDB_CMD_ADD_TYPEDEF		0x03
struct jdb_jrnl_add_typedef{
	wchar_t* tname;
	jdb_data_t* type_id;
	jdb_data_t* base;
	uint32_t* len;
	uchar* flags;
};
#define JDB_CMD_RM_TYPEDEF		0x04
struct jdb_jrnl_rm_typedef{
	wchar_t* tname;
	jdb_data_t* type_id;
};
#define JDB_CMD_ASSIGN_COLTYPE		0x05
struct jdb_jrnl_assign_coltype{
	wchar_t* tname;
	jdb_data_t* type_id;
	uint32_t* col;
};
#define JDB_CMD_RM_COLTYPE		0x06
struct jdb_jrnl_rm_coltype{
	wchar_t* tname;	
	uint32_t* col;
};

//cell
#define JDB_CMD_CREATE_CELL		0x07
struct jdb_jrnl_create_cell{
	wchar_t* tname;
	uint32_t* col;
	uint32_t* row;
	uchar* data;
	uint32_t* datalen;
	uchar* data_type;
};
#define JDB_CMD_RM_CELL			0x08
struct jdb_jrnl_rm_cell{
	wchar_t* tname;
	uint32_t* col;
	uint32_t* row;
};
#define JDB_CMD_UPDATE_CELL		0x09
struct jdb_jrnl_update_cell{
	wchar_t* tname;
	uint32_t* col;
	uint32_t* row;
	uchar* data;
	uint32_t* datalen;
};

//row/col ops
#define JDB_CMD_RM_COL			0x0a
#define JDB_CMD_INSERT_COL		0x0b
#define JDB_CMD_RM_ROW			0x0c
#define JDB_CMD_INSERT_ROW		0x0d
#define JDB_CMD_SWAP_COL		0x0e
#define JDB_CMD_SWAP_ROW		0x0f

//special
#define JDB_CMD_END			0x80	//operation ends
#define JDB_CMD_UNDO			0x40	//useless for now!
#define JDB_CMD_EMPTY			0x00

#define JDB_CMD_MASK			0x3f

