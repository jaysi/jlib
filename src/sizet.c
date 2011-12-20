#include "jac.h"
#include "jnet.h"

/*
#define JAC_BSIZE	1024

typedef unsigned char uchar;
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef unsigned long jacid_t;

#define JB_HDR_PAD (431+512)
struct JACDBHeader{
	uint16_t magic;//first three entries are constant in all j dbs!
	uchar type;
	uchar ver;
	uchar flags;
	uchar crc_type:4, crypt_type:4;
	uint32_t crc32;//header crc
	uchar last_stat;
	jacid_t last_block;
	jacid_t nblocks;//number of blocks
	uchar pwhash[32]; /*sha256
	
	jacid_t max_gid;
	jacid_t max_uid;
	jacid_t max_oid;
	
	uint16_t change_id;
	
	//configuration
	uint32_t max_utiks;
	uint32_t def_utiks;
	uchar wr_retries;
	uchar rd_retries;
	uint32_t max_logins;
	uchar nblocks_cache;//number of blocks cached in memmory before flush
	uchar inx_update;//update index after adding INX_ENTRIES... this is counter
	
	uchar pad[JB_HDR_PAD];
}__attribute__((packed));

typedef struct JACDBHeader jac_hdr;

#define JB_MAP_ENTRIES	198
#define JB_MAP_BITMAP	25
#define JB_MAP_PAD	2
#define JBF_MODIF	0x01

struct JACMapBlock{
	uchar type;
	uint32_t crc32;
	uchar nful; //n of full entries
	uchar flags;
	uchar bitmap[JB_MAP_BITMAP];
	uint32_t bno[JB_MAP_ENTRIES];//block number
	uchar btype[JB_MAP_ENTRIES];//block type
	uchar pad[JB_MAP_PAD];
}__attribute__((packed));

typedef struct JACMapBlock jac_map_block;

// Lists

typedef struct JACMapListEntry{
	jac_map_block block;
	uint32_t block_num;
	struct JACMapListEntry* next;
}jac_map_list_entry;

typedef struct JACMapList{
	jac_map_list_entry* first;
	jac_map_list_entry* last;
}jac_map_list;

#define MAX_GNAME		31
#define MAX_UNAME		43
#define MAX_ONAME		73

struct JACGroupRecord{
	uchar flags;
	char name[MAX_GNAME];
	jacid_t gid;
}__attribute__((packed));

typedef struct JACGroupRecord jac_grec;

#define JAC_REC_DISABLE		_8b0
#define JAC_REC_UT8		_8b1//second check, on if ut8; if both off use ascii
#define JAC_REC_UT16		_8b2//first check, on if ut16

#define UP_FLAG		(0x01<<0)
#define UP_NAME		(0x01<<1)
#define UP_PASS		(0x01<<2)
#define UP_ID		(0x01<<3)
#define UP_TIK		(0x01<<4)
#define UP_DATE		(0x01<<5)
#define UP_TIME		(0x01<<6)
#define UP_NLOG		(0x01<<7)
struct JACUserRecord{
	uchar flags;
	char name[MAX_UNAME];
	uchar pwhash[32]; /*sha256
	jacid_t uid;
	uint32_t tickets;
	uchar last_date[3];
	uchar last_time[3];
	uint16_t nlogins;
}__attribute__((packed));

typedef struct JACUserRecord jac_urec;

struct JACUserDepency{
	jacid_t uid;
	jacid_t gid;
}__attribute__((packed));

typedef struct JACUserDepency jac_udep;//each entry keeps 3 group depencies

#define JPERM_RD	_8b0
#define JPERM_WR	_8b1
#define JPERM_EX	_8b2
#define JOBJ_T_OWNER	0x00
#define JOBJ_T_GROUP	0x01
#define JOBJ_T_USER	0x02

struct JACObjectRecord{
	uchar flags;
	char name[MAX_ONAME];
	jacid_t objid;
	uchar last_date[3];
	uchar last_time[3];
}__attribute__((packed));

typedef struct JACObjectRecord jac_orec;

struct JACObjectDepencyRecord{
	jacid_t objid;
	uchar type;
	uchar perm;
	jacid_t gid;
	jacid_t uid;
}__attribute__((packed));

typedef struct JACObjectDepencyRecord jac_odep;

struct JACIndexRecord{
	uint32_t hash;
	jacid_t id;
	jacid_t nblock;
	uchar i;//index inside the block
}__attribute__((packed));

typedef struct JACIndexRecord jac_inx;

#define JBT_EMPTY	0x00
#define JBT_GRP_DEF	0x01
#define JBT_USR_DEF	0x02
#define JBT_OBJ_DEF	0x03
#define JBT_USR_DEP	0x04
#define JBT_OBJ_DEP	0x05
#define JBT_GRP_INX	0x06
#define JBT_USR_INX	0x07
#define JBT_OBJ_INX	0x08
#define JBT_MAP		0x09
//full blocks
#define JBT_GRP_DEF_F	0x0a
#define JBT_USR_DEF_F	0x0b
#define JBT_OBJ_DEF_F	0x0c
#define JBT_USR_DEP_F	0x0d
#define JBT_OBJ_DEP_F	0x0e
#define JBT_GRP_INX_F	0x0f
#define JBT_USR_INX_F	0x10
#define JBT_OBJ_INX_F	0x20
#define JBT_MAP_F	0x30

//defined but not used actually
struct JACBlockHeader{
	uchar type;
	uint32_t crc32;
	uchar nful; //n of full entries
	uchar flags;
}__attribute__((packed));

typedef struct JACBlockHeader jac_bhdr;

struct JACRawBlock{
	uchar type;
	uint32_t crc32;
	uchar nful; //n of full entries
	uchar flags;
	char* data;
}__attribute__((packed));

typedef struct JACRawBlock jac_raw_block;

#define GDEF_ENTRIES	28
#define GDEF_BITMAP	4
#define GDEF_PAD	5

struct JACGroupDefineBlock{
	uchar type;
	uint32_t crc32;
	uchar nful; //n of full entries
	uchar flags;
	uchar bitmap[GDEF_BITMAP];
	jac_grec rec[GDEF_ENTRIES];
	uchar pad[GDEF_PAD];
}__attribute__((packed));

typedef struct JACGroupDefineBlock jac_gdef_block;

#define UDEF_ENTRIES	11
#define UDEF_BITMAP	2
#define UDEF_PAD	3

struct JACUserDefineBlock{
	uchar type;
	uint32_t crc32;
	uchar nful; //n of full entries
	uchar flags;
	uchar bitmap[UDEF_BITMAP];
	jac_urec rec[UDEF_ENTRIES];
	uchar pad[UDEF_PAD];
}__attribute__((packed));

typedef struct JACUserDefineBlock jac_udef_block;

#define ODEF_ENTRIES	12
#define ODEF_BITMAP	2
#define ODEF_PAD	7

struct JACObjectDefineBlock{
	uchar type;
	uint32_t crc32;
	uchar nful; //n of full entries
	uchar flags;
	uchar bitmap[ODEF_BITMAP];
	jac_orec rec[ODEF_ENTRIES];
	uchar pad[ODEF_PAD];
}__attribute__((packed));

typedef struct JACObjectDefineBlock jac_odef_block;

#define UDEP_ENTRIES	125
#define UDEP_BITMAP	16
#define UDEP_PAD	1

struct JACUserDepencyBlock{
	uchar type;
	uint32_t crc32;
	uchar nful; //n of full entries
	uchar flags;
	uchar bitmap[UDEP_BITMAP];
	jac_udep rec[UDEP_ENTRIES];
	uchar pad[UDEP_PAD];
}__attribute__((packed));

typedef struct JACUserDepencyBlock jac_udep_block;

#define ODEP_ENTRIES	72
#define ODEP_BITMAP	9
//#define ODEP_PAD	8

struct JACObjectDepencyBlock{
	uchar type;
	uint32_t crc32;
	uchar nful; //n of full entries
	uchar flags;
	uchar bitmap[ODEP_BITMAP];
	jac_odep rec[ODEP_ENTRIES];
//	uchar pad[ODEP_PAD];
}__attribute__((packed));

typedef struct JACObjectDepencyBlock jac_odep_block;

#define INX_ENTRIES	76
#define INX_BITMAP	10
#define INX_PAD		11

struct JACIndexBlock{
	uchar type;
	uint32_t crc32;
	uchar nful; //n of full entries
	uchar flags;
	uint32_t max;
	uint32_t min;
	uchar bitmap[INX_BITMAP];
	jac_inx inx[INX_ENTRIES];
	uchar pad[INX_PAD];
}__attribute__((packed));

typedef struct JACIndexBlock jac_inx_block;
*/

int main()
{

	printf("sizeof(jac_hdr) = %i\n", sizeof(jac_hdr));
	printf("sizeof(jac_map_block) = %i (needs %i bytes)\n",
	       sizeof(jac_map_block), JAC_BSIZE - sizeof(jac_map_block));
	printf("sizeof(jac_group_rec) = %i \n", sizeof(jac_grec));
	printf("sizeof(jac_group_definition_block) = %i (needs %i bytes)\n",
	       sizeof(jac_gdef_block), JAC_BSIZE - sizeof(jac_gdef_block));
	printf("sizeof(jac_user_rec) = %i \n", sizeof(jac_urec));
	printf("sizeof(jac_user_definition_block) = %i (needs %i bytes)\n",
	       sizeof(jac_udef_block), JAC_BSIZE - sizeof(jac_udef_block));
	printf("sizeof(jac_obj_rec) = %i \n", sizeof(jac_orec));
	printf("sizeof(jac_obj_definition_block) = %i (needs %i bytes)\n",
	       sizeof(jac_odef_block), JAC_BSIZE - sizeof(jac_odef_block));
	printf("sizeof(jac_user_depency_block) = %i (needs %i bytes)\n",
	       sizeof(jac_udep_block), JAC_BSIZE - sizeof(jac_udep_block));
	printf("sizeof(jac_obj_depency_block) = %i (needs %i bytes)\n",
	       sizeof(jac_odep_block), JAC_BSIZE - sizeof(jac_odep_block));
	printf("sizeof(jac_inx) = %i \n", sizeof(jac_inx));
	printf("sizeof(jac_inx_block) = %i (needs %i bytes)\n",
	       sizeof(jac_inx_block), JAC_BSIZE - sizeof(jac_inx_block));
	printf("sizeof(jn_hdr) = %i (needs %i bytes)\n", sizeof(jn_hdr),
	       AES_BSIZE - sizeof(jn_hdr));
	printf("sizeof(jac_oex_block) = %i (needs %i bytes)\n",
	       sizeof(jac_oex_block), JAC_BSIZE - sizeof(jac_oex_block));
	printf("sizeof(jac_oex_inx) = %i\n", sizeof(jac_oex_inx));
	printf("sizeof(jac_oex_inx_block) = %i (needs %i bytes)\n",
	       sizeof(jac_oex_inx_block),
	       JAC_BSIZE - sizeof(jac_oex_inx_block));
	printf("sizeof(jac_tdef_block) = %i (needs %i bytes)\n",
	       sizeof(jac_tdef_block), JAC_BSIZE - sizeof(jac_tdef_block));
	printf("sizeof(jac_xdef_block) = %i (needs %i bytes)\n",
	       sizeof(jac_xdef_block), JAC_BSIZE - sizeof(jac_xdef_block));

	return 0;
}
