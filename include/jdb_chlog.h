#include "jdb_intern.h"

struct jdb_changelog_hdr{
	uint32_t recsize;
	uint64_t chid;//header's change id [i.e. sync between DB & JRNL]
	uchar cmd;
	int ret;
	uchar nargs;
}__attribute__((packed));

struct jdb_changelog_rec{
	struct jdb_changelog_hdr hdr;
	uchar* buf;
	uint32_t* argsize;
	uchar* arg;
};

struct jdb_changelog_entry{
	struct jdb_changelog_rec rec;
	struct jdb_changelog_entry* next;
};

struct jdb_changelog_list{
	uint32_t cnt;
	struct jdb_changelog_entry* first;
	struct jdb_changelog_entry* last;
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
//open
#define JDB_CMD_OPEN			0x00

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
struct jdb_changelog_rm_table{
	wchar_t* name;
};

//type
#define JDB_CMD_ADD_TYPEDEF		0x03
struct jdb_changelog_add_typedef{
	wchar_t* tname;
	jdb_data_t* type_id;
	jdb_data_t* base;
	uint32_t* len;
	uchar* flags;
};
#define JDB_CMD_RM_TYPEDEF		0x04
struct jdb_changelog_rm_typedef{
	wchar_t* tname;
	jdb_data_t* type_id;
};
#define JDB_CMD_ASSIGN_COLTYPE		0x05
struct jdb_changelog_assign_coltype{
	wchar_t* tname;
	jdb_data_t* type_id;
	uint32_t* col;
};
#define JDB_CMD_RM_COLTYPE		0x06
struct jdb_changelog_rm_coltype{
	wchar_t* tname;	
	uint32_t* col;
};

//cell
#define JDB_CMD_CREATE_CELL		0x07
struct jdb_changelog_create_cell{
	wchar_t* tname;
	uint32_t* col;
	uint32_t* row;
	uchar* data;
	uint32_t* datalen;
	uchar* data_type;
};
#define JDB_CMD_RM_CELL			0x08
struct jdb_changelog_rm_cell{
	wchar_t* tname;
	uint32_t* col;
	uint32_t* row;
};
#define JDB_CMD_UPDATE_CELL		0x09
struct jdb_changelog_update_cell{
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

//endings
#define JDB_CMD_CLOSE			0x10	//closing file
#define JDB_CMD_END			0x11	//operation ends

//journalling special
#define JDB_CMD_EMPTY			0x12
#define JDB_CMD_FAIL			0x80	//shows failure of the command

//exit
#define JDB_CMD_EXIT			0x13

