#include <string.h>
#include "jparse.h"

int isfloatdigit(char c){
	if((c >= '0' && c <= '9') || c == '.') return 1;
	return 0;
}

char jp_lookup_cmd(char *s)
 {
       register int i , j ;
       char *p ;
       p = s ;
       while(*p) {
	  *p = tolower(*p) ;
	  p ++ ;
       }
       for( i= 0 ; *jp_cmd_table[i].jp_commands ; i++)
	  if(!strcmp(jp_cmd_table[i].jp_commands , s))
	      return jp_cmd_table[i].jp_tok ;
       return 0 ;
  }
//************
int isdelim(char c)
  {
    if(strchr(JP_DELIM_SPACE_S ,c) || c == JP_TAB_C ||
	  c==JP_CR_C || c==0)
	  return 1 ;
     return 0 ;
  }
//**************
int iswhite(char c)
  {
       if(c == JP_SPACE_C || c == JP_TAB_C)
	  return 1 ;
       else
	  return 0 ;
  }
  
/*
	-the buf must be null terminated,
	-max_tok not checked
*/
char jp_get_token(char* buf, size_t* next_i, char* tok){
	char tok_type = JP_TOK_ERR;
	char* buf_main;
	
	if(*buf == JP_NUL_C)  {
		*tok = 0 ;
		return JP_TOK_END;
	}
	
	buf_main = buf;
	
	while(iswhite(*buf))
		++ buf ;
	
	if(*buf == JP_CR_C)  {
		if(*(buf+1) == JP_LF_C){
			tok_type = JP_TOK_EOL_2;
			++ buf ;
		} else tok_type = JP_TOK_EOL_1;
		++ buf ;
		*token = JP_CR_C ;
		if(tok_type == JP_TOK_EOL_2){
			tok[1] = JP_LF_C ;
			tok[2] = 0 ;
		} else {
		tok[1] = 0;
	}
	*next_i = buf_main - buf;
	return(tok_type) ;
	
	if(strchr(JP_DELIM_S , *buf))  {
		*tok=*buf ;
		buf++;
		tok++;
		*next_i = buf_main - buf;
		return JP_TOK_DELIM;
	}
	
	if(*buf == JP_QUOTE_C) {
		buf ++ ;
		while(*buf != JP_QUOTE_C && *buf != JP_CR_C)
			*tok ++ = *buf ++ ;
		if(*buf == JP_CR_C){
			*next_i = buf_main - buf;
			return  JP_TOK_ERR;
		}
		buf ++ ;
		*tok = 0 ;
		*next_i = buf_main - buf;
		return JP_TOK_QUOTE;
	}

	if(isdigit(*buf)) {
		while(!isdelim(*buf)){
			if(!isfloatdigit(*buf)){
				*next_i = buf_main - buf;
				return JP_TOK_ERR;
			}
			*tok ++= *buf ++ ;			
		}
		*tok=JP_NUL_C ;
		*next_i = buf_main - buf;
		return JP_TOK_NUM ;
	}

	if(isalpha(*buf)) {
		while(!isdelim(*buf))
		*tok ++= *buf ++ ;
		token_type = JP_TOK_STR ;
	}
	*tok=JP_NUL_C ;
	if(tok_type == JP_TOK_STR) {
		//tok = jp_lookup_cmd(tok) ;
		//if(!tok)
			tok_type = JP_TOK_VAR ;
		//else
		//	tok_type = JP_TOK_CMD ;
	}
	*next_i = buf_main - buf;
	 return tok_type ;	
}

char* jp_c_to_s(char c){
	
}

