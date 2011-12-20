#include "jparse.h"
#include "jstruct.h"

#define MAX_TOK	20

struct jstack postfix_stack;
struct jstack calc_stack;

int infix_to_postfix(char* str, char* postfix_str){
	char tok_type, prev_tok_type = ;
	char tok[MAX_TOK];
	size_t next_i = 0UL;
	*postfix_str = 0;
	jstack_init(&postfix_stack, 0);
	while(1){
		tok_type = jp_get_tok(str, &next_i, tok);
		switch(tok_type){
			case JP_TOK_END:
				return 0;
			break;
			case JP_TOK_ERR:
				printf("token error on position %u\n", next_i);
				return -JE_TOK;
			break;
			case JP_TOK_NUM:
				if(prev_tok_type == JP_TOK_NUM){
					printf("syntax error at pos %u\n", next_i);
					return -JE_SYNTAX;
				}
				strcat(postfix_str, tok);
				strcat(postfix_str, JP_COL_S);
			break;
			case JP_TOK_DELIM:
				switch(tok[0]){
					case '+':
					case '-':
					case '*':
					case '/':
						jstack_push(
					case '(':
						
					case ')':
					break;
					default:
					printf("could not understand %c at pos %u\n", tok[0], next_i);
					return -JE_IMPLEMENT;
				}
			break;
			default:
			break;
		}
		prev_tok_type = tok_type;
	}
	
}

int do_calc(char* str, double* result){
	int ret;
	size_t next_i;
	ret = infix_to_postfix(str, postfix_str);
	if(ret < 0) return ret;
}

int main(int argc, char* argv[]){

	char* str;
	double result;

	if(argv[1]){
		str = argv[1];
	} else {
		
	}

	ret = do_calc(str, &result);
	if(ret < 0){
		
	} else {
		
	}


	return 0;
}
