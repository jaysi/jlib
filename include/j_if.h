#ifndef _J_IF_H
#define _J_IF_H

#include <wchar.h>

#define MAX_CMD	20

#define INIT	0
#define LIST	1
#define CANCEL	2
#define OPEN	3
#define CLOSE	4
#define GROUP	5
#define USER	6
#define STAT	7
#define EXIT	8
#define UMEM	9
#define LOGIN	10
#define EDIT	11
#define USERADD	12
#define RMUSER	13
#define USERFIND	14
#define GROUPADD	15
#define RMGROUP	16
#define GROUPFIND	17
#define HELP	18
#define DEBUG	19
#define HDUMP	20
#define UDUMP	21
#define GDUMP	22
#define UEDIT	23
#define GEDIT	24
#define LOGOUT	25
#define KEEP	26
#define HEDIT	27
#define OBJECT	28
#define OUMEM	29
#define OBJADD	30
#define RMOBJ	31
#define OBJFIND	32
#define SHUTDOWN 33
#define OPENSET 34
#define SIZE	35
#define LLOGIN	36
#define FILE	37
#define OWNER	38
#define ODUMP	39
#define OEDIT	40
#define CONF	41
#define GADDZ	42
#define GHDUMP	43
#define UADDZ	44
#define UHDUMP	45
#define MAPDUMP	46
#define BLOCKWALK	47
#define DUMPGINX	48
#define DCACHE	49
#define DUMPUINX	50
#define GMEM	51
#define UDADD	52
#define RMUD	53
#define RMUDALL	54
#define GOBJMEM 55
#define UOBJMEM 56
#define ODADD	57
#define ODEP	58
#define OADDZ	59
#define SYNC	60
#define RMGDALL 61
#define RMUODALL 62
#define RMGODALL 63
#define RMODALL 64
#define ODEPFIND 65
#define LOGOUTALL 66
#define OUSE	67
#define OFREE	68
#define OLIST	69
#define BCOUNT	70
#define IFUL	71
#define PASS	72
#define DUPDEP	73
#define DUPODEP 74
#define RMOD	75
#define UDADDZ	76
#define ODADDZ	77
#define RUN	78
#define MOD	79
#define KICK	80
#define BAN	81
#define CONN	82
#define START	83
#define UNBAN	84
#define CONNECT	85
#define MLIST	86
#define MYLIST	87
#define UCLIST	88
#define JOIN	89
#define EX	90
#define ECHO	91
#define CHAT	92
#define CHATRECV 93
#define LEAVE	94
#define PAUSE	95
#define RESUME	96
#define REG	97
#define UNREG	98
#define REGDIR	99
#define RELOAD	100
#define CMD_NOP	101
#define OEX	102
#define OEXALL	103
#define OEXADD	104
#define RMOEX	105
#define CHOEX	106
#define DOEXINXALL	107
#define DOEXALL	108
#define DXDEFALL	109
#define ADD	110
#define RM	111
#define FIND	112
#define LISTALL	113
#define TREE	114
#define TREEADD	115
#define RMTREE	116
#define TREEFIND	117
#define LEAFADD	118
#define RMLEAF	119
#define TREEVIEW	120
#define TREEROOT	121
#define VIEW	122
#define RMCOL	123
#define RMROW	124
#define CELL	125
#define TABLE	126
#define EXPORT	127
#define IMPORT	128
#define SEARCH	129
#define OPEN2	130
#define BDUMP	131
#define LLIST	132
#define REN	133
#define TYPE	134
#define SYNCALL	135
#define TADDZ	136
#define TYPADDZ	137
#define TCADDZ	138

#define _ZERO	1000
#define _ONE	1001
#define _TWO	1002
#define _THREE	1003
#define _FOUR	1004
#define _FIVE	1005
#define _SIX	1006
#define _SEVEN	1007
#define _EIGHT	1008
#define _NINE	1009

static struct JifCmd {
	int no;
	const wchar_t *str;
} jifcmd[] = {
	{
	_ZERO, L"0"}, {
	_ONE, L"1"}, {
	_TWO, L"2"}, {
	_THREE, L"3"}, {
	_FOUR, L"4"}, {
	_FIVE, L"5"}, {
	_SIX, L"6"}, {
	_SEVEN, L"7"}, {
	_EIGHT, L"8"}, {
	_NINE, L"9"}, {
	TYPADDZ, L"typaddz"}, {
	TADDZ, L"taddz"}, {
	SYNCALL, L"syncall"}, {
	LLIST, L"llist"}, {
	TYPE, L"type"}, {
	OPEN2, L"open2"}, {
	SEARCH, L"search"}, {
	EXPORT, L"export"}, {
	IMPORT, L"import"}, {
	VIEW, L"view"}, {
	RMCOL, L"rmcol"}, {
	RMROW, L"rmrow"}, {
	CELL, L"cell"}, {
	TREEVIEW, L"treeview"}, {
	TREEROOT, L"treeroot"}, {
	LEAFADD, L"leafadd"}, {
	RMLEAF, L"rmleaf"}, {
	TREE, L"tree"}, {
	TREEADD, L"treeadd"}, {
	RMTREE, L"rmtree"}, {
	TREEFIND, L"treefind"}, {
	LISTALL, L"listall"}, {
	FIND, L"find"}, {
	ADD, L"add"}, {
	RM, L"rm"}, {
	DOEXALL, L"doexall"}, {
	DXDEFALL, L"dxdefall"}, {
	DOEXINXALL, L"doexinxall"}, {
	OEX, L"oex"}, {
	OEXALL, L"oexall"}, {
	OEXADD, L"oexadd"}, {
	RMOEX, L"rmoex"}, {
	CHOEX, L"choex"}, {
	CMD_NOP, L"nop"}, {
	RELOAD, L"reload"}, {
	REG, L"reg"}, {
	UNREG, L"unreg"}, {
	REGDIR, L"regdir"}, {
	LEAVE, L"leave"}, {
	PAUSE, L"pause"}, {
	RESUME, L"resume"}, {
	CHAT, L"chat"}, {
	CHATRECV, L"chatrecv"}, {
	EX, L"ex"}, {
	ECHO, L"echo"}, {
	JOIN, L"join"}, {
	UCLIST, L"uclist"}, {
	MYLIST, L"mylist"}, {
	MLIST, L"mlist"}, {
	CONNECT, L"connect"}, {
	UNBAN, L"unban"}, {
	KICK, L"kick"}, {
	BAN, L"ban"}, {
	CONN, L"conn"}, {
	START, L"start"}, {
	MOD, L"mod"}, {
	RUN, L"run"}, {
	CONF, L"conf"}, {
	OWNER, L"owner"}, {
	FILE, L"file"}, {
	LLOGIN, L"llogin"}, {
	SIZE, L"size"}, {
	UEDIT, L"uedit"}, {
	GEDIT, L"gedit"}, {
	OEDIT, L"oedit"}, {
	HEDIT, L"hedit"}, {
	UDUMP, L"uedit"}, {
	GDUMP, L"gedit"}, {
	ODUMP, L"odump"}, {
	HDUMP, L"hdump"}, {
	GHDUMP, L"ghdump"}, {
	UHDUMP, L"uhdump"}, {
	MAPDUMP, L"mapdump"}, {
	KEEP, L"keep"}, {
	INIT, L"init"}, {
	LIST, L"list"}, {
	CANCEL, L"cancel"}, {
	OPEN, L"open"}, {
	CLOSE, L"close"}, {
	GROUP, L"group"}, {
	USER, L"user"}, {
	STAT, L"stat"}, {
	EXIT, L"exit"}, {
	UMEM, L"udep"}, {
	GMEM, L"gdep"}, {
	LOGIN, L"login"}, {
	LOGOUT, L"logout"}, {
	EDIT, L"edit"}, {
	USERADD, L"useradd"}, {
	RMUSER, L"rmuser"}, {
	USERFIND, L"userfind"}, {
	GROUPADD, L"groupadd"}, {
	RMGROUP, L"rmgroup"}, {
	GROUPFIND, L"groupfind"}, {
	HELP, L"help"}, {
	DEBUG, L"debug"}, {
	OBJECT, L"object"}, {
	OUMEM, L"oug"}, {
	OBJADD, L"objadd"}, {
	RMOBJ, L"rmobj"}, {
	OBJFIND, L"objfind"}, {
	SHUTDOWN, L"shutdown"}, {
	OPENSET, L"openset"}, {
	GADDZ, L"gaddz"}, {
	UADDZ, L"uaddz"}, {
	BLOCKWALK, L"blockwalk"}, {
	DUMPGINX, L"dginx"}, {
	DUMPUINX, L"duinx"}, {
	DCACHE, L"dcache"}, {
	UDADD, L"udadd"}, {
	RMUD, L"rmud"}, {
	RMUDALL, L"rmudall"}, {
	GOBJMEM, L"gobjmem"}, {
	UOBJMEM, L"uobjmem"}, {
	ODADD, L"odadd"}, {
	ODEP, L"odep"}, {
	OADDZ, L"oaddz"}, {
	SYNC, L"sync"}, {
	RMGDALL, L"rmgdall"}, {
	RMUODALL, L"rmuodall"}, {
	RMGODALL, L"rmgodall"}, {
	RMODALL, L"rmodall"}, {
	ODEPFIND, L"odepfind"}, {
	LOGOUTALL, L"logoutall"}, {
	OUSE, L"ouse"}, {
	OFREE, L"ofree"}, {
	OLIST, L"olist"}, {
	BCOUNT, L"bcount"}, {
	IFUL, L"iful"}, {
	PASS, L"pass"}, {
	DUPDEP, L"dupdep"}, {
	DUPODEP, L"dupodep"}, {
	RMOD, L"rmod"}, {
	UDADDZ, L"udaddz"}, {
	ODADDZ, L"odaddz"}, {
	BDUMP, L"bdump"}, {
	TABLE, L"table"}, {
	REN, L"ren"}, {
	0, 0}
};

int lookup_cmd(wchar_t * cmd);
void jif_perr(int err);

#endif
