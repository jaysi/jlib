#ifndef _JPARSE_H
#define _JPARSE_H

/*			*** delimiters - string ***			*/

#define JP_DELIM_S		"+-*^/%=;(),><"
#define JP_DELIM_SPACE_S	" +-*^/%=;(),><"

/*			*** signs ***			*/

//
#define JP_DOT_S	"."

//math-basic
#define JP_PLUS_S	"+"
#define JP_MINUS_S	"-"
#define JP_MULT_S	"*"
#define JP_DIV_S	"/"
#define JP_MOD_S	"%"
#define JP_POW_S	"^"
#define JP_PAR_OPEN_S	"("
#define JP_PAR_CLOSE_S	")"

//math-logic
#define JP_LT_S		"<"
#define JP_GT_S		">"
#define JP_EQ_S		"="

//math-func
#define JP_SIN_S	"sin"
#define JP_COS_S	"cos"
#define JP_TAN_S	"tan"

//logic
#define JP_AND_S	"&"
#define JP_OR_S		"|"

//white
#define JP_SPACE_S	" "
#define JP_TAB_S	"\t"

//other delimiters
#define JP_COL_S	","
#define JP_SEMICOL_S	";"
#define JP_CR_C		"\r"
#define JP_LF_C		"\n"

/*			*** delimiters - char ***			*/

/*			*** signs ***			*/

//
#define JP_DOT_C	'.'

//math-basic
#define JP_PLUS_C	'+'
#define JP_MINUS_C	'-'
#define JP_MULT_C	'*'
#define JP_DIV_C	'/'
#define JP_MOD_C	'%'
#define JP_POW_C	'^'
#define JP_PAR_OPEN_C	'('
#define JP_PAR_CLOSE_C	')'

//math-logic
#define JP_LT_C		'<'
#define JP_GT_C		'>'
#define JP_EQ_C		'='

//math-func
#define JP_SIN_C	'sin'
#define JP_COS_C	'cos'
#define JP_TAN_C	'tan'

//logic
#define JP_AND_C	'&'
#define JP_OR_C		'|'

//white
#define JP_SPACE_S	' '
#define JP_TAB_S	'\t'

//other delimiters
#define JP_COL_C	','
#define JP_SEMICOL_C	';'
#define JP_NUL_C	'\0'
#define JP_CR_C		'\r'
#define JP_LF_C		'\n'


/*			*** token types ***			*/
#define JP_TOK_ERR	0
#define JP_TOK_VAR	1
#define JP_TOK_NUM	2
#define JP_TOK_CMD	3
#define JP_TOK_STR	4
#define JP_TOK_DELIM	5
#define JP_TOK_QUOTE	6
#define JP_TOK_END	7
#define JP_TOK_EOL_1	8	//end of line 1, CR
#define JP_TOK_EOL_2	9	//end of line 2, CR/LF

/*

struct jp_commands {
		   char jp_commands[20] ;
		   char jp_tok ;
		} jp_table[] = {
		"print", JP_CMD_PRINT, "input", JP_CMD_INPUT, "if",JP_CMD_IF,
		"then", JP_CMD_THEN, "goto", JP_CMD_GOTO, "for", JP_CMD_FOR ,
		"next", JP_CMD_NEXT , "to", JP_CMD_TO , "gosub", JP_CMD_GOSUB ,
		"return", JP_CMD_ETURN , "end", JP_CMD_END , "", JP_CMD_END } ;

*/


