#include "jss.h"
#include "j_if.h"
#include "jer.h"
#include "jtable.h"

#include <stdlib.h>
#include <locale.h>

#define VER	"0.0.1"

void help()
{
	wprintf(L"\nSupported commands:\n\n");
	wprintf(L"\tINIT\tInitialize jss handle\n");
	wprintf(L"\tOPEN\tOpens jss database\n");
	wprintf(L"\tCLOSE\tCloses jss\n");
	wprintf(L"\tSYNC\tSynchronizing database with disk\n");
	wprintf(L"\tSTAT\tStatus of current jss handle\n");
	wprintf(L"\tADD\tAdd entry\n");
	wprintf(L"\tLIST\tList entries by name or type\n");
	wprintf(L"\tRM\tRemove entry\n");
	wprintf(L"\tLISTALL\tList all entries\n");
	wprintf(L"\tFIND\tDumps jss entry\n");
	wprintf(L"\tTABLE\tTable subsystem\n");
	wprintf(L"\tEXIT\tExits jss interface\n\n");
}

void flag_jss(uchar flags)
{
	if (flags & JSS_REC_DONTLOAD)
		wprintf(L"DONT_LOAD ");
	if (flags & JSS_REC_FILE)
		wprintf(L"FILE ");
	if (flags & JSS_REC_DONTTOUCH)
		wprintf(L"DONT_TOUCH");
}

void dump_jss_hdr(struct jss_rec_hdr *hdr)
{

	wprintf(L"flags: ");
	flag_jss(hdr->flags);
	wprintf(L"\ncrc32: 0x%x\n", hdr->crc32);
	wprintf(L"offset: %u\n", hdr->offset);
	wprintf(L"datalen (real): %u\n", hdr->datalen);
	wprintf(L"id: %u\n", hdr->id);
	wprintf(L"type: 0x%x\n", hdr->type);
	wprintf(L"namelen (real): %u\n", hdr->namelen);
	wprintf(L"--------------------\n");
}

void open_jss(struct jss_handle *h)
{
	wchar_t filename[JSS_MAXFILENAME];
	wchar_t key[JSS_MAXKEY];
	int ret;
	uchar flags = 0;
	wchar_t yn[2];

	wprintf(L"filename: ");
	wscanf(L"%S", filename);
	wprintf(L"key: ");
	wscanf(L"%S", key);
	wprintf(L"set read only? ");
	wscanf(L"%S", yn);
	if (yn[0] == L'y')
		flags = JSS_RDONLY;

	ret = jss_open(h, filename, key, flags);
	if (ret < 0) {
		jif_perr(ret);
		return;
	}
	wprintf(L"[ OK ]\n");

}

void add_jss(struct jss_handle *h)
{
	wchar_t name[JSS_MAXFILENAME];
	uchar *data;
	uint32_t datalen;
	uint32_t namelen;
	uint32_t id;
	int iid;
	uint16_t type;
	uchar flags;
	wchar_t fstr[10];
	int ret;

	wprintf(L"data name (type \'file\' to read from file): ");
	wscanf(L"%S", name);
	wprintf(L"Id: ");
	wscanf(L"%i", &iid);
	id = (uint32_t) iid;
	wprintf(L"Type: ");
	wscanf(L"%u", &type);
	wprintf(L"flags (d = DONT_LOAD, i = IGNORE): ");
	wscanf(L"%S", fstr);
	flags = 0;
	for (ret = 0; ret < wcslen(fstr); ret++) {
		if (fstr[ret] == L'i')
			break;
		if (fstr[ret] == L'd')
			flags = JSS_REC_DONTLOAD;
		else
			wprintf(L"Could not understand flag %C at %i\n",
				fstr[ret], ret);
	}

	if (!wcscmp(name, L"file")) {
		wprintf(L"filename: ");
		wscanf(L"%S", name);
		wprintf(L"adding file # %u...", id);
		ret = jss_add_file(h, id, type, flags, name);
	} else {
		wprintf(L"data len for # %u: ", id);
		wscanf(L"%u", &datalen);
		data = (uchar *) malloc(datalen);
		wprintf(L"data: ");
		wscanf(L"%s", data);
		wprintf(L"adding entry...");
		ret = jss_add(h, id, type, flags, name, datalen, data);
		free(data);
	}

	if (ret < 0) {
		wprintf(L"[FAIL]\n");
		jif_perr(ret);
	} else {
		wprintf(L"[DONE]\n");
	}
}

void rm_jss(struct jss_handle *h)
{
	uint32_t id;
	int ret;

	wprintf(L"Id: ");
	wscanf(L"%u", &id);

	wprintf(L"removing entry...");
	ret = jss_rm(h, id);
	if (ret < 0) {
		wprintf(L"[FAIL]\n");
		jif_perr(ret);
	} else {
		wprintf(L"[DONE]\n");
	}
}

void list_jss(struct jss_handle *h)
{
	struct jss_rec_entry *entry;
	struct jss_rec_list list;
	int ret;
	wchar_t name[JSS_MAXNAME];
	uint16_t type;

	wprintf(L"name (type \'null\' to filter by type): ");
	wscanf(L"%S", name);
	if (!wcscmp(name, L"null")) {
		wprintf(L"Type: ");
		wscanf(L"%u", &type);
		ret = jss_list(h, NULL, type, &list);
	} else {
		ret = jss_list(h, name, 0, &list);
	}

	if (ret < 0) {
		jif_perr(ret);
		return;
	}

	for (entry = list.first; entry; entry = entry->next) {
		dump_jss_hdr(&entry->hdr);
	}

	jss_free_list(&list);
}

void listall_jss(struct jss_handle *h)
{
	struct jss_rec_entry *entry;

	for (entry = h->rec_list->first; entry; entry = entry->next) {
		dump_jss_hdr(&entry->hdr);
	}

}

void find_jss(struct jss_handle *h)
{
	wchar_t *name;
	uchar *data;
	uint32_t datalen;
	uint32_t namelen;
	uint32_t id;
	uint16_t type;
	uchar flags;
	wchar_t fstr[10];
	int ret;

	wprintf(L"Id: ");
	wscanf(L"%u", &id);

	ret = jss_get_rec(h, id, &type, &flags, &name, &datalen, &data);
	if (ret < 0) {
		jif_perr(ret);
	} else {
		//dump_jss_hdr(&rec->hdr);
		wprintf(L"\nNAME: %S\nDATA: %s\n", name, data);
		free(name);
		//wprintf(L"DUMP DATA NOT IMPLEMENTED\n");
	}
}

void stat_jss(struct jss_handle *h)
{

	if ((h->magic == JSS_MAGIC) && (h->flags & JSS_INIT)) {
		wprintf(L"Handle initialized.\n");
		if (h->flags & JSS_OPEN) {
			wprintf(L"File open.\n");
		} else {
			wprintf(L"No open file.\n");
			return;
		}
	} else {
		wprintf(L"Handle is not initialized.\n");
		return;
	}

	if (h->hdr.flags & JSS_BE) {
		wprintf(L"Big Endian.\n");
	} else {
		wprintf(L"Little Endian.\n");
	}

	if (h->hdr.flags & JSS_COMP) {
		wprintf(L"Compressed.\n");
	} else {
		wprintf(L"Not Compressed.\n");
	}

	if (h->hdr.flags & JSS_CRYPT) {
		wprintf(L"Encrypted.\n");
	} else {
		wprintf(L"Not Encrypted.\n");
	}


	wprintf(L"number of records: %u\n", h->hdr.nrec);

}

int table_prepare(struct jss_handle *h, struct jt_table *t)
{
	uint32_t row;
	uint32_t col;
	int iid;
	uint32_t id;
	int ret;
	wchar_t str[3];
	wchar_t name[100];
	wchar_t *tname;
	uchar *buf;
	uint32_t bufsize;
	uint16_t type;
	uchar flags;

	wprintf(L"1. load / create table.\n");
	wprintf(L"2. edit / import table.\n");
	wscanf(L"%S", str);
	if (str[0] == L'2') {
		goto edit;
	}

	wprintf(L"load table ( y / n ): ");
	wscanf(L"%S", str);
	if (str[0] == L'y') {
		wprintf(L"Id: ");
		wscanf(L"%i", &iid);
		id = (uint32_t) iid;
		//ret = jss_get_rec(h, id, &type, &flags, &name, &datalen, &data);
		wprintf(L"Loading record...");
		ret = jss_get_rec(h, id, &type, &flags, &tname, &bufsize, &buf);
		if (type != JSS_JTABLE) {
			wprintf(L"[FAIL]\n");
			wprintf
			    (L"ERROR: Type mismatch, record '%S' is not a j-tree\n",
			     tname);
			free(tname);
			return;
		}
		wprintf(L"[DONE]\n");
		wprintf(L"Loaded '%S'\n", tname);
		wprintf(L"converting to table...");
		ret = jt_buf_to_table(t, buf, bufsize);

		if (ret) {
			wprintf(L"[FAIL]\n");
			jif_perr(ret);
			return;
		}

		wprintf(L"[DONE]\n");
		t->name = tname;

		table_jss(t);

		return;

	}

	wprintf(L"Name: ");
	wscanf(L"%S", name);

	wprintf(L"rows: ");
	wscanf(L"%u", &row);

	wprintf(L"columns: ");
	wscanf(L"%u", &col);

	wprintf(L"id: ");
	wscanf(L"%i", &iid);
	id = (uint32_t) iid;

	wprintf(L"creating table...");

	ret = jt_create_table(t, row, col, id);
	if (ret < 0) {
		wprintf(L"[FAIL]\n");
		jif_perr(ret);
		return ret;
	} else {
		wprintf(L"[DONE]\n");
	}

	t->name = (wchar_t *) malloc((wcslen(name) + 1) * sizeof(wchar_t));
	wcscpy(t->name, name);

 edit:

	table_jss(t);

	wprintf(L"write tree ( y / n ): ");
	wscanf(L"%S", str);
	if (str[0] == L'y') {
		wprintf(L"converting table...");
		ret = jt_table_to_buf(&buf, &bufsize, t);

		if (ret) {
			wprintf(L"[FAIL]\n");
			jif_perr(ret);
			return;
		}

		wprintf(L"[DONE]\n");
		wprintf(L"adding to jss...");
		//ret = jss_add(h, id, type, flags, name, datalen, data);
		ret = jss_add(h, id, JSS_JTABLE, 0, name, bufsize, buf);
		if (ret) {
			wprintf(L"[FAIL]\n");
			jif_perr(ret);
			free(buf);
			return;
		}
		free(buf);
		wprintf(L"[DONE]\n");
	}

}

int main()
{
	struct jss_handle h;
	struct jt_table t;
	wchar_t cmd[20];
	int ret;
	char jerr[MAX_ERR_STR];
	setlocale(LC_ALL, "");
	if (jss_init(&h) < 0) {
		wprintf(L"\nInit failed.\n");
		return -1;
	}
	wprintf
	    (L"\njss interface version %s, type \'HELP\' to get a list of commands.\n",
	     VER);

 prompt:
	wprintf(L"\nMain >");
	wscanf(L"%S", cmd);

	switch (lookup_cmd(cmd)) {
	case INIT:
		if ((ret = jss_init(&h)) < 0)
			jif_perr(ret);
		break;
	case OPEN:
		open_jss(&h);
		break;
	case SYNC:
		wprintf(L"synchronising...");
		ret = jss_sync(&h);
		if (ret < 0) {
			wprintf(L"[FAIL]\n");
			jif_perr(ret);
		} else
			wprintf(L"[DONE]\n");
		break;
	case ADD:
		add_jss(&h);
		break;
	case RM:
		rm_jss(&h);
		break;
	case LIST:
		list_jss(&h);
		break;
	case FIND:
		find_jss(&h);
		break;
	case CLOSE:
		jss_close(&h);
		break;
	case HELP:
		help();
		break;
	case LISTALL:
		listall_jss(&h);
		break;
	case STAT:
		stat_jss(&h);
		break;
	case TABLE:
		table_prepare(&h, &t);
		break;
	case EXIT:
		jss_close(&h);
		return 0;
	default:
		wprintf(L"\nCommand not supported, try using \'HELP\'\n");
		break;
	}
	goto prompt;
	return 0;
}
