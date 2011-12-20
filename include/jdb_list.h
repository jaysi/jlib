#ifndef _JDB_LIST_H
#define _JDB_LIST_H

/*
  allowed filter types are:
  	1. '0' as first char -> ignore the filter string
  	2. '<' -> d < f
  	3. '>' -> d > f
  	4. '=' -> d = f
  	5. '!=' or '<>' or '><' -> d != f
  	6. '<=' or '=<' -> d <= f
  	7. '>=' or '=>' -> d >= f
  	8. '*' -> use wild cards for string compares

  use JDB_FILTER_TYPE() MACRO to convert string to the filter_type valuef
*/

//filter type, bits 0-3
#define JDB_FILTER_T_IGN	0x00
#define JDB_FILTER_T_LT		0x01
#define JDB_FILTER_T_GT		0x02
#define JDB_FILTER_T_EQ		0x03
#define JDB_FILTER_T_NE		0x04
#define JDB_FILTER_T_LE		0x05
#define JDB_FILTER_T_GE		0x06
#define JDB_FILTER_T_WILD	0x07
#define JDB_FILTER_T_PART	0x08
#define JDB_FILTER_T_MASK	0x0f

//filter logic, bits 6 and 7, NOTE: the filter entries are interpreted in
//				order of appearance,  
#define JDB_FILTER_L_NOT	0x00//bit7:unset, bit6:unset
#define JDB_FILTER_L_AND	0x80//bit7:set, bit6:unset
#define JDB_FILTER_L_OR		0x40//bit7:unset, bit6:set
#define JDB_FILTER_L_XOR	0xC0//bit7:set, bit6:set
#define JDB_FILTER_L_MASK	JDB_FILTER_L_XOR

//a flag, compare size?, bit 4
#define JDB_FILTER_F_SIZE	0x10//compare sizes, if not set, compare values

#define JDB_FILTER_TYPE(str)	if(!strcmp(str, "0")) return JDB_FILTER_T_IGN;\
				if(!strcmp(str, "<")) return JDB_FILTER_T_LT;\
				if(!strcmp(str, ">")) return JDB_FILTER_T_GT;\
				if(!strcmp(str, "=")) return JDB_FILTER_T_EQ;\
				if(!strcmp(str, "!=") || \
				   !strcmp(str, "<>") || \
				   !strcmp(str, "><")) return JDB_FILTER_T_NE;\
				if(!strcmp(str, "<=") || \
				   !strcmp(str, "=<")) return JDB_FILTER_T_LE;\
				if(!strcmp(str, ">=") || \
				   !strcmp(str, "=>")) return JDB_FILTER_T_GE;\
				if(!strcmp(str, "*")) return JDB_FILTER_T_WILD;

struct jdb_filter_entry{
	uchar type;	/*      BIT0-3:	TYPE
				        BIT4:	CMP_SIZE FLAG
                        BIT6-7:	LOGIC
			            */
	uint32_t col;/*	column to filter, set to INVALID to check the
			whole boundary*/

	jdb_data_t data_type;/*ignore if set as EMPTY, i.e. compare with all fileds in range*/

	uint32_t str_len;
	uchar* str;

	struct jdb_rowset* result;
};

struct jdb_rowset{
	uint32_t n;
	uint32_t* row;
	uint32_t* col;//can be match column or JDB_ID_INVAL
};

struct jdb_filter{
	size_t nfilters;
	size_t cnt;

	/* filter's effective area */	
	uint32_t col_start;	//use INVAL to include all columns
	uint32_t col_end;	
	uint32_t row_start;	// SAA
	uint32_t row_end;
	
	/*
	  compare function, returns the winner pointer, for sorting purpose,
	  to disable sorting, set this to NULL
	The comparison function must return an integer less than, equal to,  or
        greater  than  zero  if  the first argument is considered to be respec‐
        tively less than, equal to, or greater than the second.  If two members
        compare as equal, their order in the sorted array is undefined.	  
	*/
	uint32_t sort_col; //base column for sorting
	int (*sort_fn)(void* s1, void* s2, size_t len);	

	struct jdb_filter_entry* entry;
	
};

#else

#endif
