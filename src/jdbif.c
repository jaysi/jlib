#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <locale.h>

#include "jdb.h"
#include "jdb_intern.h"
#include "debug.h"
#include "j_if.h"
#include "jconst.h"

#define VER	"0.0.1"
#define PROMPT_LEN	20

wchar_t prompt_str[PROMPT_LEN];

void jdbif_set_prompt(wchar_t * str)
{
	wcscpy(prompt_str, str);
}

void jdbif_prompt()
{
	wprintf(L"\n%ls", prompt_str);
}

void jdbif_help()
{
	wprintf(L"\nSupported commands:\n\n");
	wprintf(L"\tSTAT\tCurrent status\n");
	wprintf(L"\tOPEN\tOpen/Create database using default settings\n");
	wprintf(L"\tOPEN2\tOpen/Create database, provides detailed settings\n");	
	wprintf(L"\tTABLE\tTable sub-system\n");
	wprintf(L"\tDEBUG\tDebug sub-system\n");
	wprintf(L"\tSYNC\tSynchronizing database\n");
	wprintf(L"\tCLOSE\tClose database\n");
	wprintf(L"\tEXIT\tExit interface\n");
}

int jdbif_stat(struct jdb_handle *h)
{

	int i;
	uint16_t mask;

	if (h->magic != JDB_MAGIC) {
		wprintf(L"Handle is not initialized.\n");
		return 0;
	}

	if (h->fd == -1) {
		wprintf(L"No open files.\n");
	} else {
		wprintf(L"File < %ls > is open.\n", h->conf.filename);
		wprintf(L"File type: 0x%02x\n", h->hdr.type);
		wprintf(L"File version: 0x%08x\n", h->hdr.ver);
		wprintf(L"blocksize is < %u > bytes.\n", h->hdr.blocksize);
		wprintf(L"crc type is 0x%02x\n", h->hdr.crc_type);
		wprintf(L"encryption type is 0x%02x\n", h->hdr.crypt_type);
		wprintf(L"write retries is %u\n", h->hdr.wr_retries);
		wprintf(L"read retries is %u\n", h->hdr.rd_retries);
		wprintf(L"maximum number of blocks is %u\n", h->hdr.max_blocks);
		wprintf(L"number of entries inside a map block is %u\n",
			h->hdr.map_bent);
		wprintf(L"number of entries inside a typedef block is %u\n",
			h->hdr.typedef_bent);
		wprintf
		    (L"number of entries inside a column/row-typedef block is %u\n",
		     h->hdr.typedef_bent);
		wprintf(L"number of entries inside a celldef block is %u\n",
			h->hdr.celldef_bent);
		wprintf(L"number of entries inside a index1 block is %u\n",
			h->hdr.index1_bent);
		wprintf(L"number of entries inside a map buck is %u\n",
			h->hdr.map_list_buck);
		wprintf(L"number of blocks is %u\n", h->hdr.nblocks);
		wprintf(L"number of nmaps is %u\n", h->hdr.nmaps);
		wprintf(L"number of ntables is %u\n", h->hdr.ntables);
		wprintf(L"number of changes are %u\n", h->hdr.chid);

		wprintf(L"db flags are (0x%04x): ", h->hdr.flags);
		mask = 0x8000;
		for (i = 0; i < sizeof(h->hdr.flags) * 8; i++) {
			if (mask & h->hdr.flags)
				wprintf(L"1");
			else
				wprintf(L"0");
			mask = mask >> 1;
		}
		wprintf(L"\n");

	}

	return 0;

}

int jdbif_open(struct jdb_handle *h)
{
	int ret;
	wchar_t filename[J_MAX_PATHNAME8];
	wchar_t pass[J_MAX_PASSWORD8];
	uint16_t flags;
	wchar_t flag_str[16];

	wprintf(L"filename: ");
	wscanf(L"%ls", filename);
	wprintf(L"pass: ");
	wscanf(L"%ls", pass);

	wprintf(L"openning %ls ...", filename);
	ret = jdb_open(h, filename, pass, JDB_DEF_FLAGS);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
	} else
		wprintf(L"\t[DONE]\n");

	return ret;
}

int jdbif_open2(struct jdb_handle *h)
{
	int ret;
	wchar_t filename[J_MAX_PATHNAME8];
	wchar_t pass[J_MAX_PASSWORD8];
	uint16_t flags;
	wchar_t flag_str[16];

	wprintf(L"filename: ");
	wscanf(L"%ls", filename);
	wprintf(L"pass: ");
	wscanf(L"%ls", pass);

	if (!jdb_fstat(filename, pass, JDB_FSTAT_EXIST))
		goto do_open;

	wprintf(L"flags are: \n");
	wprintf(L"\tBIT0: force journalling\n");
	wprintf
	    (L"\tBIT1: do not use journalling, if this flag nor the flag above has not been set, journalling will be used if available\n");
	wprintf(L"\tBIT2: read-only\n");
	wprintf(L"\tBIT3: enable packing\n");
	wprintf(L"\tBIT4: enable aio( NOT IMPLEMENTED NOW! )\n");
	wprintf(L"\tBIT5: enable threaded-write\n");

 get_bit_str:
	flags = 0;
	wmemset(flag_str, L'\0', 16);
	wprintf(L"flags bit string: ");
	wscanf(L"%ls", flag_str);

	for (ret = 0; ret < wcslen(flag_str); ret++)
		if (flag_str[ret] == L'1')
			flags |= 0x0001 << ret;
		else if (flag_str[ret] == L'0') {

		} else {
			wprintf
			    (L"could not understand position %i in bit string, please use 0 or 1\n",
			     ret);
			goto get_bit_str;
		}

	h->conf.flags = flags;

	wprintf(L"blocksize (%u): ", JDB_DEF_BLOCK_SIZE);
	wscanf(L"%u", &h->conf.blocksize);

	wprintf(L"map buck entries (%u): ", JDB_DEF_MAPLIST_BUCK);
	wscanf(L"%u", &h->conf.map_list_buck);

	wprintf(L"crc type (0x%x): ", JDB_DEF_CRC);
	wscanf(L"%x", &h->conf.crc_type);

	wprintf(L"crypto type (0x%x): ", JDB_DEF_CRYPT);
	wscanf(L"%x", &h->conf.crypt_type);

	h->conf.filename = (wchar_t *) malloc(WBYTES(filename));
	wcscpy(h->conf.filename, filename);
	h->conf.key = (wchar_t *) malloc(WBYTES(pass));
	wcscpy(h->conf.key, pass);

 do_open:
	wprintf(L"openning %ls ...", filename);
	ret = jdb_open2(h, 0);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jif_perr(ret);
	} else
		wprintf(L"\t[DONE]\n");

	return ret;
}

int main(int argc, char *argv[])
{
	wchar_t cmd[MAX_CMD];
	int ret;
	char jerr[MAX_ERR_STR];
	wchar_t dbname[J_MAX_PATHNAME8];
	wchar_t pass[J_MAX_PASSWORD8];
	struct jdb_handle h;

	setlocale(LC_ALL, "");

	wprintf
	    (L"\njdb interface, version %s, type \'HELP\' to get a list of commands.\n",
	     VER);

	if (argc > 2) {
		/*
		   jcstow((uchar*)argv[1], username, MAX_UNAME);
		   jcstow((uchar*)argv[2], pass, J_MAX_PASSWORD8);
		   wprintf(L"connecting localhost:49999 as %ls...", username);
		   ret = jn_connect(&conn, "localhost", "49999", username, pass);
		 */
		if (ret < 0) {
			wprintf(L"\t[FAIL]\n");
			jif_perr(ret);
		} else {
			wprintf(L"\t[ OK ]\n");

			/*submit error handler

			   cb = jn_assm_cb(&conn, JN_CB_KEEP | JN_CB_ERR, 0, 0,
			   (void*)&jncif_handle_err, NULL,
			   NULL, NULL);

			   jn_reg_cb(&conn, NULL, cb); */
		}

	}

 prompt1:
	jdbif_set_prompt(L"Main >");
	jdbif_prompt();
	wscanf(L"%ls", cmd);

	switch (lookup_cmd(cmd)) {
	case STAT:
		jdbif_stat(&h);
		break;
	case OPEN:
		jdbif_open(&h);
		break;
	case OPEN2:
		jdbif_open2(&h);
		break;
	case TABLE:
		jdbif_table(&h);
		break;
	case DEBUG:
		jdbif_debug(&h);
		break;
	case SYNC:
		jdb_sync(&h);
		break;
	case CLOSE:
		jdb_close(&h);
		break;
	case HELP:
		jdbif_help();
		break;
	case EXIT:
		jdb_close(&h);
		return 0;
	default:
		wprintf(L"\nCommand not supported, try using \'HELP\'\n");
		break;
	}
	goto prompt1;
	return 0;
}
