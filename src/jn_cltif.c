#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <locale.h>

#include "jnet.h"
#include "jnet_internals.h"
#include "jer.h"
#include "debug.h"
#include "j_if.h"

#define _wdeb1(f, a...)		//jncif_mod_list_handle free crash

#define VER	"1.0.0"

#define MAX_HOSTNAME	200
#define MAX_PORT	10

#define CIF_SUPP_COMS	"\n\tSCHAT\n"
#define CIF_PROMPT_LEN	20

jn_mod_entry *mlist;
jn_comod_entry *clist;
wchar_t prompt_str[CIF_PROMPT_LEN];

void set_prompt(wchar_t * str)
{
	wcscpy(prompt_str, str);
}

void prompt()
{
	wprintf(L"\n%S", prompt_str);
}

void jncif_handle_err(jn_cb_entry * cb_entry)
{
	char jerr[200];

	jer_lookup_str(cb_entry->jerr, jerr);
	wprintf(L"Error: %i, %s", cb_entry->jerr, jerr);
	if (cb_entry->pkt) {

		_jn_transa_rm(((jn_conn *) cb_entry->conn)->trans_a,
			      cb_entry->pkt->hdr.trans_id);

		wprintf(L" TID = %u", cb_entry->pkt->hdr.trans_id);
		if (cb_entry->pkt->hdr.flags & JNF_PLAIN) {
			wprintf(L", plain data: %s", cb_entry->pkt->data);
		}
		wprintf(L"\n");
	} else {
		wprintf(L"\n");
	}

	_jn_free_cb(cb_entry);

	//free(cb_entry);
	prompt();
}

void jncif_handle_echo_req(jn_cb_entry * cb_entry)
{
	wchar_t rcv_msg[256];
	int ret = 0;

	_jn_transa_rm(((jn_conn *) cb_entry->conn)->trans_a,
		      cb_entry->pkt->hdr.trans_id);

	wprintf(L"Echo: ");
	if (cb_entry->pkt->hdr.data_len < 2) {
		wprintf(L"Bad data length\n");
		ret = -1;
	}

	if (!ret)
		if (jcstow(cb_entry->pkt->data, rcv_msg, 256) < 0) {
			wprintf(L"Bad charset\n");
			ret = -2;
		}

	if (!ret) {
		wprintf(L"%S\n", rcv_msg);

	}

	if (!ret || ret == -2) {
		_jn_free_cb(cb_entry);
	}

	prompt();
}

int jncif_chat(jn_conn * conn)
{
	wchar_t snd_msg[256], com_name[JN_MAX_COM_NAME];
	jn_comod_entry *com;
	jn_pkt pkt;
	jtransid_t transid;
	int ret;
	char jerr[100];
	//jn_cb_entry* q;

	wprintf(L"COMOD name: ");
	wscanf(L"%S", com_name);

	for (com = clist; com; com = com->next) {
		if (!wcscmp(com->copyname, com_name))
			break;
	}

	if (!com) {
		wprintf(L"COMOD not found, try mod > list.\n");
		return -JE_NOTFOUND;
	}

	wprintf(L"message: ");
	wscanf(L"%S", snd_msg);

	pkt.hdr.type = JNT_COMREQ;
	pkt.hdr.data_len = wtojcs_len(snd_msg, 256) + 1;
	pkt.hdr.comod_id = com->com_id;
	pkt.hdr.trans_id = _jn_transa_get(conn->trans_a);
	transid = pkt.hdr.trans_id;
	pkt.data = (uchar *) malloc(pkt.hdr.data_len);
	wtojcs(snd_msg, pkt.data, pkt.hdr.data_len);

	//wprintf(L"requested jcs %s, wchar was %S\n", pkt.data, snd_msg);

	ret = jn_send(conn, &pkt);
	/*
	   q = jn_assm_cb( conn, JN_CB_TID, pkt.hdr.trans_id, 0,
	   (void*)&jncif_handle_echo_req, NULL,
	   NULL, NULL);

	   ret = jn_reg_cb(conn, &pkt, q);
	 */
	free(pkt.data);

	if (ret < 0) {
		//      free(q);
		wprintf(L"\t[FAIL]\n");
		jer_lookup_str(ret, jerr);
		wprintf(L"\t\tError: %i, %s\n", ret, jerr);
		return ret;
	} else {
		wprintf(L"Submitted request\n");
	}

	return 0;

}

void jncif_print_chat(jn_cb_entry * cb)
{
	char msg[1024];
	wchar_t wmsg[1024];
	if (cb->jerr) {
		jer_lookup_str(cb->jerr, msg);
		wprintf(L"[SCHATMODULE], Error: %i, %s\n", cb->jerr, msg);
	} else {
		if (cb->pkt->hdr.data_len) {
			jcstow(cb->pkt->data, wmsg, 1024);
			wprintf(L"[SCHAT] %S\n", wmsg);
		}
	}

	_jn_free_cb(cb);

	prompt();
}

int jncif_chat_recv(jn_conn * conn)
{
	jn_comod_entry *com;
	wchar_t com_name[JN_MAX_COM_NAME];
	jn_cb_entry *q;
	int ret;
	char jerr[200];

	wprintf(L"COMOD name: ");
	wscanf(L"%S", com_name);

	for (com = clist; com; com = com->next) {
		if (!wcscmp(com->copyname, com_name))
			break;
	}

	if (!com) {
		wprintf(L"COMOD not found, try mod > list.\n");
		return -JE_NOTFOUND;
	}

	q = jn_assm_cb(conn, JN_CB_COMID | JN_CB_KEEP, 0, com->com_id,
		       (void *)&jncif_print_chat, NULL, NULL, NULL);

	ret = jn_reg_cb(conn, NULL, q);

	if (ret < 0) {
		free(q);
		wprintf(L"\t[FAIL]\n");
		jer_lookup_str(ret, jerr);
		wprintf(L"\t\tError: %i, %s\n", ret, jerr);
		return ret;
	} else {
		wprintf(L"Submitted request\n");
	}

	return 0;

}

int jncif_echo_req(jn_conn * conn)
{
	wchar_t snd_msg[256], com_name[JN_MAX_COM_NAME];
	jn_comod_entry *com;
	jn_pkt pkt;
	jtransid_t transid;
	int ret;
	char jerr[100];
	jn_cb_entry *q;

	wprintf(L"COMOD name: ");
	wscanf(L"%S", com_name);

	for (com = clist; com; com = com->next) {
		if (!wcscmp(com->copyname, com_name))
			break;
	}

	if (!com) {
		wprintf(L"COMOD not found, try mod > list.\n");
		return -JE_NOTFOUND;
	}

	wprintf(L"message: ");
	wscanf(L"%S", snd_msg);

	pkt.hdr.type = JNT_COMREQ;
	pkt.hdr.data_len = wtojcs_len(snd_msg, 256) + 1;
	pkt.hdr.comod_id = com->com_id;
	pkt.hdr.trans_id = _jn_transa_get(conn->trans_a);
	transid = pkt.hdr.trans_id;
	pkt.data = (uchar *) malloc(pkt.hdr.data_len);
	wtojcs(snd_msg, pkt.data, pkt.hdr.data_len);

	//wprintf(L"requested jcs %s, wchar was %S\n", pkt.data, snd_msg);

	q = jn_assm_cb(conn, JN_CB_TID, pkt.hdr.trans_id, 0,
		       (void *)&jncif_handle_echo_req, NULL, NULL, NULL);

	ret = jn_reg_cb(conn, &pkt, q);

	free(pkt.data);

	if (ret < 0) {
		free(q);
		wprintf(L"\t[FAIL]\n");
		jer_lookup_str(ret, jerr);
		wprintf(L"\t\tError: %i, %s\n", ret, jerr);
		return ret;
	} else {
		wprintf(L"Submitted request\n");
	}

	return 0;

}

void jncif_ex_help()
{
	wprintf(L"\tECHO\techo request\n");
	wprintf(L"\tCHAT\tchat request\n");
	wprintf(L"\tCHATRECV\tsets comod to recieve the chat messages\n");
	wprintf(L"\tEXIT\texit subsystem\n");
}

void jncif_ex(jn_conn * conn)
{
	wchar_t cmd[MAX_CMD];
	wprintf
	    (L"\nModule Example Subsystem, type \'HELP\' to get a list of commands.\n");
 prompt1:
	set_prompt(L"Ex >");
	prompt();
	wscanf(L"%S", cmd);
	switch (lookup_cmd(cmd)) {
	case CHAT:
		jncif_chat(conn);
		break;
	case CHATRECV:
		jncif_chat_recv(conn);
		break;
	case ECHO:
		jncif_echo_req(conn);
		break;
	case HELP:
		jncif_ex_help();
		break;
	case EXIT:
		return;
	default:
		wprintf(L"\nCommand not supported, try using \'HELP\'\n");
		break;
	}
	goto prompt1;

}

void jncif_com_open_handle(jn_cb_entry * cb_entry)
{
	char jerr[200];
	struct _jn_open_com *s = (struct _jn_open_com *)cb_entry->if_context;

	if (cb_entry->jerr) {

		wprintf(L"COMOD Open request failed, ");
		wprintf(L"ObjID: %u, Copyname: %S\n", s->objid, s->copyname);
		jer_lookup_str(cb_entry->jerr, jerr);
		wprintf(L", Error: %i, %s\n", cb_entry->jerr, jerr);
		if (s->copyname)
			free(s->copyname);
	} else {
		wprintf(L"COMOD Opened successfully, ");
		wprintf(L"Copyname: %S, ObjID: %u, ComID: %u\n", s->copyname,
			s->objid, s->modid);
	}

	free(s);
	_jn_free_cb(cb_entry);

	prompt();

}

int jncif_com_open(jn_conn * conn)
{
	char jerr[200];
	int ret;
	wchar_t modname[JN_MAX_COM_NAME], copyname[JN_MAX_COM_NAME];
	jn_mod_entry *entry;
	struct _jn_open_com *s;

	if (!mlist) {
		wprintf(L"Empty module list, try mod > mlist first.\n");
		return -JE_EMPTY;
	}

	wprintf(L"Module name: ");
	wscanf(L"%S", &modname);

	for (entry = mlist; entry; entry = entry->next) {
		if (!wcscmp(entry->pathname, modname))
			break;
	}

	if (!entry) {
		wprintf
		    (L"Module name < %S > not found, try updating your list via mod > mlist.\n",
		     modname);
		return -JE_NOTFOUND;
	} else {
		s = (struct _jn_open_com *)malloc(sizeof(struct _jn_open_com));
		if (!s) {
			wprintf(L"No Memmory\n");
			return -JE_MALOC;
		}
		s->objid = entry->objid;
	}

	wprintf(L"COMOD name: ");
	wscanf(L"%S", copyname);

	s->copyname = (wchar_t *) malloc(WBYTES(copyname));
	if (!s->copyname) {
		wprintf(L"No Memmory\n");
		free(s);
		return -JE_MALOC;
	}
	wcscpy(s->copyname, copyname);

	s->flags = 0x00;

	wprintf(L"Registering Comod Open request...");
	ret = jn_open_com(conn, (void *)&jncif_com_open_handle, (void *)s, s);
	if (ret < 0) {
		free(s->copyname);
		free(s);
		wprintf(L"\t[FAIL]\n");
		jer_lookup_str(ret, jerr);
		wprintf(L"\t\tError: %i, %s\n", ret, jerr);
		return ret;
	} else {
		wprintf(L"\t[ OK ]\n");
	}

	return 0;

}

int jncif_com_join(jn_conn * conn)
{
	char jerr[200];
	int ret;
	wchar_t copyname[JN_MAX_COM_NAME];
	jn_comod_entry *entry;
	struct _jn_open_com *s;

	if (!clist) {
		wprintf(L"Empty COMOD list, try mod > list first.\n");
		return -JE_EMPTY;
	}

	wprintf(L"COMOD name: ");
	wscanf(L"%S", copyname);

	for (entry = clist; entry; entry = entry->next) {
		if (!wcscmp(entry->copyname, copyname))
			break;
	}

	if (!entry) {
		wprintf
		    (L"COMOD not found, try updating COMOD list via mod > list.\n");
		return -JE_NOTFOUND;
	} else {
		s = (struct _jn_open_com *)malloc(sizeof(struct _jn_open_com));
		if (!s) {
			wprintf(L"No Memmory\n");
			return -JE_MALOC;
		}
		s->copyname = (wchar_t *) malloc(WBYTES(copyname));
		wcscpy(s->copyname, copyname);
		s->modid = entry->com_id;
		s->objid = entry->objid;
	}

	wprintf(L"Joining COMOD...");
	ret = jn_join_com(conn, (void *)&jncif_com_open_handle, (void *)s, s);
	if (ret < 0) {
		free(s->copyname);
		free(s);
		wprintf(L"\t[FAIL]\n");
		jer_lookup_str(ret, jerr);
		wprintf(L"\t\tError: %i, %s\n", ret, jerr);
		return ret;
	} else {
		wprintf(L"\t[ OK ]\n");
	}

	return 0;
}

void jncif_com_proto1_handle(jn_cb_entry * cb_entry)
{
	char jerr[200];

	if (cb_entry->jerr) {

		wprintf(L"COMOD %S request failed",
			(wchar_t *) cb_entry->if_context);
		jer_lookup_str(cb_entry->jerr, jerr);
		wprintf(L", Error: %i, %s\n", cb_entry->jerr, jerr);
	} else {
		wprintf(L"COMOD %S request done.",
			(wchar_t *) cb_entry->if_context);
	}

	free(cb_entry->if_context);

	_jn_free_cb(cb_entry);

	prompt();
}

int jncif_com_leave(jn_conn * conn)
{
	char jerr[200];
	int ret;
	wchar_t copyname[JN_MAX_COM_NAME];
	jn_comod_entry *entry;
	wchar_t *if_ctx;

	if (!clist) {
		wprintf(L"Empty COMOD list, try mod > list first.\n");
		return -JE_EMPTY;
	}

	wprintf(L"COMOD name: ");
	wscanf(L"%S", copyname);

	for (entry = clist; entry; entry = entry->next) {
		if (!wcscmp(entry->copyname, copyname))
			break;
	}

	if (!entry) {
		wprintf
		    (L"COMOD not found, try updating COMOD list via mod > list.\n");
		return -JE_NOTFOUND;
	}

	wprintf(L"Leaving COMOD...");
	if_ctx = (wchar_t *) malloc(WBYTES(L"Leave"));
	wcscpy(if_ctx, L"Leave");
	ret =
	    jn_leave_com(conn, (void *)&jncif_com_proto1_handle, (void *)if_ctx,
			 entry->com_id);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jer_lookup_str(ret, jerr);
		wprintf(L"\t\tError: %i, %s\n", ret, jerr);
		return ret;
	} else {
		wprintf(L"\t[ OK ]\n");
	}

	return 0;
}

int jncif_com_pause(jn_conn * conn)
{
	char jerr[200];
	int ret;
	wchar_t copyname[JN_MAX_COM_NAME];
	jn_comod_entry *entry;
	wchar_t *if_ctx;

	if (!clist) {
		wprintf(L"Empty COMOD list, try mod > list first.\n");
		return -JE_EMPTY;
	}

	wprintf(L"COMOD name: ");
	wscanf(L"%S", copyname);

	for (entry = clist; entry; entry = entry->next) {
		if (!wcscmp(entry->copyname, copyname))
			break;
	}

	if (!entry) {
		wprintf
		    (L"COMOD not found, try updating COMOD list via mod > list.\n");
		return -JE_NOTFOUND;
	}

	wprintf(L"Pausing COMOD...");
	if_ctx = (wchar_t *) malloc(WBYTES(L"Pause"));
	wcscpy(if_ctx, L"Pause");
	ret =
	    jn_pause_com(conn, (void *)&jncif_com_proto1_handle, (void *)if_ctx,
			 entry->com_id);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jer_lookup_str(ret, jerr);
		wprintf(L"\t\tError: %i, %s\n", ret, jerr);
		return ret;
	} else {
		wprintf(L"\t[ OK ]\n");
	}

	return 0;
}

int jncif_com_resume(jn_conn * conn)
{
	char jerr[200];
	int ret;
	wchar_t copyname[JN_MAX_COM_NAME];
	jn_comod_entry *entry;
	wchar_t *if_ctx;

	if (!clist) {
		wprintf(L"Empty COMOD list, try mod > list first.\n");
		return -JE_EMPTY;
	}

	wprintf(L"COMOD name: ");
	wscanf(L"%S", copyname);

	for (entry = clist; entry; entry = entry->next) {
		if (!wcscmp(entry->copyname, copyname))
			break;
	}

	if (!entry) {
		wprintf
		    (L"COMOD not found, try updating COMOD list via mod > list.\n");
		return -JE_NOTFOUND;
	}

	wprintf(L"Resuming COMOD...");
	if_ctx = (wchar_t *) malloc(WBYTES(L"Resume"));
	wcscpy(if_ctx, L"Resume");
	ret =
	    jn_resume_com(conn, (void *)&jncif_com_proto1_handle,
			  (void *)if_ctx, entry->com_id);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jer_lookup_str(ret, jerr);
		wprintf(L"\t\tError: %i, %s\n", ret, jerr);
		return ret;
	} else {
		wprintf(L"\t[ OK ]\n");
	}

	return 0;
}

void jncif_ucom_list(jn_conn * conn)
{
	jn_ucom_entry *entry;
	wprintf(L"User Comod List (copyname comod_id objid){\n");
	if (!conn->ucom_list->first) {
		wprintf(L"\tEmpty\n");
		goto end;
		return;
	}

	for (entry = conn->ucom_list->first; entry; entry = entry->next) {
		wprintf(L"\t%S \t %u \t %u\n", entry->copyname, entry->mod_id,
			entry->objid);
	}
 end:
	wprintf(L"\n}User Comod List;\n");
}

void jncif_com_list_handle(jn_cb_entry * cb_entry)
{
	int ret;
	jn_comod_entry *delete;
	char jerr[200];

	_jn_transa_rm(((jn_conn *) cb_entry->conn)->trans_a,
		      cb_entry->pkt->hdr.trans_id);

	if (cb_entry->jerr) {
		wprintf(L"COMOD List request failed, ");
		jer_lookup_str(cb_entry->jerr, jerr);
		wprintf(L"Error: %i, %s\n", cb_entry->jerr, jerr);
		if (cb_entry->buf)
			free(cb_entry->buf);
		free(cb_entry);
		goto end;
	} else {

		wprintf(L"Got Comod List Request, Parsing...");
		ret =
		    jn_parse_comlist((uchar *) cb_entry->buf, cb_entry->nbytes,
				     &clist);
		free(cb_entry->buf);
		free(cb_entry);
		if (ret < 0) {
			wprintf(L"\t[FAIL]\n");
			jer_lookup_str(ret, jerr);
			wprintf(L"\t\tError: %i, %s\n", ret, jerr);
			goto end;
		} else {
			wprintf(L"\t[ OK ]\n");
		}

		wprintf(L"Server Comod list( CopyName, ComodId, ObjectId ){\n");
		delete = clist;
		while (delete) {
			wprintf(L"\t%S, %u, %u\n", delete->copyname,
				delete->com_id, delete->objid);
			delete = delete->next;
		}
		wprintf(L"}Server Comod list;\n");

		goto end;
	}

	free(cb_entry);

 end:
	prompt();
}

int jncif_list(jn_conn * conn, void *callback, uchar type)
{
	int ret;
	char jerr[200];

	wprintf(L"Getting list...");
	ret = jn_recv_list(conn, callback, NULL, type);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jer_lookup_str(ret, jerr);
		wprintf(L"\t\tError: %i, %s\n", ret, jerr);
		return ret;
	} else {
		wprintf(L"\t[ OK ]\n");
	}

	return 0;
}

int jncif_com_list(jn_conn * conn)
{

	jn_comod_entry *delete;

	while (clist) {
		delete = clist;
		clist = clist->next;
		free(delete->copyname);
		free(delete);
	}

	return jncif_list(conn, (void *)&jncif_com_list_handle, JNT_COMLIST);

}

void jncif_mod_list_handle(jn_cb_entry * cb_entry)
{

	jn_mod_entry *delete;
	char jerr[200];
	int ret;

	_wdeb1(L"called");

	_jn_transa_rm(((jn_conn *) cb_entry->conn)->trans_a,
		      cb_entry->pkt->hdr.trans_id);

	if (cb_entry->jerr) {
		wprintf(L"Module List request failed, ");
		jer_lookup_str(cb_entry->jerr, jerr);
		wprintf(L"Error: %i, %s\n", cb_entry->jerr, jerr);
		_wdeb1(L"free cb_entry->buf");
		if (cb_entry->buf)
			free(cb_entry->buf);
		_wdeb1(L"free s");
		free(cb_entry);
		goto end;
	}

	wprintf(L"Parsing list...");
	ret =
	    jn_parse_modlist((uchar *) cb_entry->buf, cb_entry->nbytes, &mlist);
	_wdeb1(L"free cb_entry->buf");
	free(cb_entry->buf);
	_wdeb1(L"free(s)");
	free(cb_entry);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jer_lookup_str(ret, jerr);
		wprintf(L"\t\tError: %i, %s\n", ret, jerr);
		goto end;
	} else {
		wprintf(L"\t[ OK ]\n");
	}

	wprintf(L"Server Module list ( ModuleName, ObjectId ){\n");
	if (!mlist) {
		wprintf(L"\tEmpty\n");
		goto end;
	} else {
		delete = mlist;
		while (delete) {
			wprintf(L"\t%S, %u\n", delete->pathname, delete->objid);
			delete = delete->next;
		}

	}

 end:
	wprintf(L"}Server Modlue list;\n");

	prompt();
}

int jncif_mod_list(jn_conn * conn)
{
	jn_mod_entry *delete;

	while (mlist) {
		delete = mlist;
		mlist = mlist->next;
		free(delete->pathname);
		free(delete);
	}

	return jncif_list(conn, (void *)&jncif_mod_list_handle, JNT_MODLIST);

}

void jncif_mod_help()
{
	wprintf(L"\nSupported commands (COMOD = Copy of module):\n\n");
	wprintf(L"\tMLIST\tLists Modules at server\n");
	wprintf(L"\tLIST\tLists COMODs at server\n");
	wprintf(L"\tOPEN\tOpen a new COMOD\n");
	wprintf(L"\tCLOSE\tClose a COMOD, needs ownership of that comod\n");
	wprintf(L"\tJOIN\tJoin a COMOD\n");
	wprintf(L"\tLEAVE\tLeave a COMOD\n");
	wprintf(L"\tPAUSE\tPause COMOD\n");
	wprintf(L"\tRESUME\tResume COMOD\n");
	wprintf(L"\tMYLIST\tLists your open COMODs\n");
	wprintf(L"\tEX\tExample modules\n");
	wprintf(L"\tEXIT\tExit to main menu\n");
}

void jncif_mod(jn_conn * conn)
{
	wchar_t cmd[MAX_CMD];
	wprintf
	    (L"\nModule Management Subsystem, type \'HELP\' to get a list of commands.\n");

 prompt1:
	set_prompt(L"Mod >");
	prompt();
	wscanf(L"%S", cmd);
	switch (lookup_cmd(cmd)) {
	case MLIST:
		jncif_mod_list(conn);
		break;
	case LIST:
		jncif_com_list(conn);
		break;
	case HELP:
		jncif_mod_help();
		break;
	case OPEN:
		jncif_com_open(conn);
		break;
	case LEAVE:
		jncif_com_leave(conn);
		break;
	case PAUSE:
		jncif_com_pause(conn);
		break;
	case RESUME:
		jncif_com_resume(conn);
		break;
	case MYLIST:
		jncif_ucom_list(conn);
		break;
	case JOIN:
		jncif_com_join(conn);
		break;
	case EX:
		jncif_ex(conn);
		break;
	case EXIT:
		return;
	default:
		wprintf(L"\nCommand not supported, try using \'HELP\'\n");
		break;
	}
	goto prompt1;

}

void jncif_connect(jn_conn * conn)
{
	wchar_t username[MAX_UNAME];
	wchar_t pass[MAX_PASS];
	char hostname[MAX_HOSTNAME];
	char port[MAX_PORT];
	int ret;
	char jerr[200];
	jn_cb_entry *cb;

	wprintf(L"hostname: ");
	wscanf(L"%s", hostname);
	wprintf(L"port: ");
	wscanf(L"%s", port);

	wprintf(L"username: ");
	wscanf(L"%S", username);
	wprintf(L"password: ");
	wscanf(L"%S", pass);

	wprintf(L"Connecting...");
	//conn->flags = JN_CF_PS;
	ret = jn_connect(conn, hostname, port, username, pass);
	if (ret < 0) {
		wprintf(L"\t[FAIL]\n");
		jer_lookup_str(ret, jerr);
		wprintf(L"\t\tError: %i, %s\n", ret, jerr);
		perror("");
		return;
	} else {
		wprintf(L"\t[ OK ]\n");

		//submit error handler

		cb = jn_assm_cb(conn, JN_CB_KEEP | JN_CB_ERR, 0, 0,
				(void *)&jncif_handle_err, NULL, NULL, NULL);

		jn_reg_cb(conn, NULL, cb);
	}
}

void jncif_stat(jn_conn * conn)
{

}

void help()
{
	wprintf(L"\nSupported commands:\n\n");
	wprintf(L"\tCONNECT\tConnects to the server\n");
	wprintf(L"\tSTAT\tCurrent status\n");
	wprintf(L"\tMOD\tModule subsystem\n");
	wprintf(L"\tEXIT\tExits client\n");
}

int main(int argc, char *argv[])
{
	wchar_t cmd[MAX_CMD];
	int ret;
	char jerr[MAX_ERR_STR];
	jn_conn conn;
	jn_cb_entry *cb;
	wchar_t username[MAX_UNAME];
	wchar_t pass[MAX_PASS];

	setlocale(LC_ALL, "");

	wprintf
	    (L"\njnet client interface version %s, type \'HELP\' to get a list of commands.\n",
	     VER);

	mlist = NULL;
	clist = NULL;

	if (argc > 2) {
		jcstow((uchar *) argv[1], username, MAX_UNAME);
		jcstow((uchar *) argv[2], pass, MAX_PASS);
		wprintf(L"connecting localhost:49999 as %S...", username);
		ret = jn_connect(&conn, "localhost", "49999", username, pass);
		if (ret < 0) {
			wprintf(L"\t[FAIL]\n");
			jer_lookup_str(ret, jerr);
			wprintf(L"\t\tError: %i, %s\n", ret, jerr);
			perror("");
		} else {
			wprintf(L"\t[ OK ]\n");

			//submit error handler

			cb = jn_assm_cb(&conn, JN_CB_KEEP | JN_CB_ERR, 0, 0,
					(void *)&jncif_handle_err, NULL,
					NULL, NULL);

			jn_reg_cb(&conn, NULL, cb);
		}

	}

 prompt1:
	set_prompt(L"Main >");
	prompt();
	wscanf(L"%S", cmd);

	switch (lookup_cmd(cmd)) {
	case CONNECT:
		jncif_connect(&conn);
		break;
	case MOD:
		jncif_mod(&conn);
		break;
	case STAT:
		jncif_stat(&conn);
		break;
	case HELP:
		help();
		break;
	case EXIT:
		return 0;
	default:
		wprintf(L"\nCommand not supported, try using \'HELP\'\n");
		break;
	}
	goto prompt1;
	return 0;
}
