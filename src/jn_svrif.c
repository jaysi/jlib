#include "jnet.h"
#include "jer.h"

#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <locale.h>

#include "j_if.h"

#define VER	"1.0.0"
#define NOP	L"nop"

void jnif_conn_help()
{
	wprintf(L"\nSupported commands:\n\n");
	wprintf(L"\tLIST\tLists users\n");
	wprintf(L"\tUCLIST\tLists user COMODs\n");
	wprintf(L"\tKICK\tKicks user from server\n");
	wprintf(L"\tBAN\tBans user\n");
	wprintf(L"\tUNBAN\tUnBans user\n");
	wprintf(L"\tEXIT\tExit to main menu\n");
}

void jnif_conn_list(jn_h * h)
{
	//jn_conn* entry;
	jac_login_list *lent;
	int cnt;
	char s[INET6_ADDRSTRLEN];
	jn_poll_buck *pollb;

	wprintf
	    (L"Connection List (Username -> address -> uid -> login_time){\n");
	if (!h->nusers) {
		wprintf(L"\tEmpty\n");
		goto end;
	}
	_lock_mx(&h->poll_list->mx);
	for (pollb = h->poll_list->first; pollb; pollb = pollb->next) {
		_lock_mx(&pollb->conn_a_mx);
		for (cnt = 0; cnt < h->conf.nfds_buck; cnt++) {
			if (pollb->conn_a[cnt].uid != JACID_INVAL) {
				inet_ntop(pollb->conn_a[cnt].addr.ss_family,
					  get_in_addr((struct sockaddr *)
						      &pollb->conn_a[cnt].addr),
					  s, sizeof(s));

				wprintf(L"\t%S -> %s -> %u -> ",
					pollb->conn_a[cnt].username, s,
					pollb->conn_a[cnt].uid);
				for (lent = h->jac.login_list; lent;
				     lent = lent->next) {
					if (lent->uid == pollb->conn_a[cnt].uid) {
						wprintf(L"%i-%i-%i %i:%i:%i\n",
							(int)lent->date[0] +
							BASE_YEAR,
							(int)lent->date[1],
							(int)lent->date[2],
							(int)lent->time[0],
							(int)lent->time[1],
							(int)lent->time[2]);
						break;
					} else {
						wprintf
						    (L"WARNING: user not found in login list\n");
					}
				}
			}
			if (!lent)
				wprintf(L"\n");
			cnt++;
			if (cnt == 20) {
				cnt = 0;
				wprintf(L"\nPress any key to continue...\n");
				getchar();
				getchar();
			}
		}
		_unlock_mx(&pollb->conn_a_mx);
	}
	_unlock_mx(&h->poll_list->mx);
 end:
	wprintf(L"}Connection List;\n");
}

jn_conn *jnif_find_conn(jn_h * h, wchar_t * username)
{
	jn_poll_buck *buck;
	jn_conn *conn = NULL;
	uint16_t i;
	_lock_mx(&h->poll_list->mx);
	for (buck = h->poll_list->first; buck; buck = buck->next) {
		_lock_mx(&buck->conn_a_mx);
		for (i = 0; i < h->conf.nfds_buck; i++) {
			if (!wcscmp(username, buck->conn_a[i].username)) {
				conn = &buck->conn_a[i];
				_unlock_mx(&buck->conn_a_mx);
				goto end;
			}
		}
		_unlock_mx(&buck->conn_a_mx);
	}
 end:
	_unlock_mx(&h->poll_list->mx);
	return conn;
}

void jnif_ucom_list(jn_h * h)
{
	wchar_t uname[MAX_UNAME];
	jn_conn *conn;
	jn_ucom_entry *uc_entry;
	wprintf(L"username: ");
	wscanf(L"%S", uname);
	wprintf(L"Searching ...");
	conn = jnif_find_conn(h, uname);
	if (!conn) {
		wprintf(L"\t[FAIL]\n");
		return;
	}
	wprintf(L"\t[ OK ]\n");
	wprintf(L"User COMOD List (comod_name -> com_id -> objid){\n");
	//_lock_mx(&conn->mx);
	if (conn->ucom_list->cnt == 0) {
		wprintf(L"\tEmpty\n");
		goto end;
	}
	for (uc_entry = conn->ucom_list->first; uc_entry;
	     uc_entry = uc_entry->next) {
		wprintf(L"\t%S -> %u -> %u\n", uc_entry->copyname,
			uc_entry->mod_id, uc_entry->objid);
	}
 end:
	//_unlock_mx(&conn->mx);
	wprintf(L"}User COMOD List;\n");
}

void jnif_conn(jn_h * h)
{
	wchar_t cmd[MAX_CMD];
	wprintf
	    (L"\nConnection Management Subsystem, type \'HELP\' to get a list of commands.\n");
 prompt:
	wprintf(L"\nConn >");
	wscanf(L"%S", cmd);
	switch (lookup_cmd(cmd)) {
	case LIST:
		jnif_conn_list(h);
		break;
	case UCLIST:
		jnif_ucom_list(h);
		break;
	case HELP:
		jnif_conn_help();
		break;
	case EXIT:
		return;
	default:
		wprintf(L"\nCommand not supported, try using \'HELP\'\n");
		break;
	}
	goto prompt;

}

void jnif_mod_list(jn_h * h)
{
	jn_mod_entry *entry;
	wprintf(L"Module List (name -> objid){\n");
	_lock_mx(&h->mod_list->mx);
	if (!h->mod_list->cnt) {
		wprintf(L"\tEmpty\n");
		goto end;
	}
	for (entry = h->mod_list->first; entry; entry = entry->next) {
		wprintf(L"\t%S -> %u\n", entry->pathname, entry->objid);
	}

 end:
	_unlock_mx(&h->mod_list->mx);
	wprintf(L"}Module List;\n");
}

void jnif_com_list(jn_h * h)
{
	jn_comod_entry *entry;
	wprintf(L"Comod List (copyname -> objid -> com_id -> owner_id){\n");
	_lock_mx(&h->com_list->mx);
	if (!h->com_list->cnt) {
		wprintf(L"\tEmpty\n");
		goto end;
	}
	for (entry = h->com_list->first; entry; entry = entry->next) {
		wprintf(L"\t%S -> %u -> %u -> %u", entry->copyname,
			entry->objid, entry->com_id, entry->owner_id);
		if (entry->flags & JN_COM_PAUSED) {
			wprintf(L"\tPAUSED\n");
		} else {
			wprintf(L"\n");
		}
	}
 end:
	_unlock_mx(&h->com_list->mx);
	wprintf(L"}Comod List;\n");
}

void jnif_mod_unreg(jn_h * h)
{
	wchar_t modname[MAX_ONAME];
	int ret;

	wprintf(L"module name: ");
	wscanf(L"%S", modname);
	wprintf(L"Unregistering module...");
	ret = jac_rm_object(&h->jac, modname);
	if (ret < 0) {
		wprintf(L"[FAILED]\n");
	} else {
		wprintf(L"[DONE]\n");
	}

}

void jnif_mod_reg(jn_h * h)
{
	wchar_t modname[MAX_ONAME];
	jobjid_t objid;
	int ret;

	wprintf(L"module name: ");
	wscanf(L"%S", modname);
	wprintf(L"Registering module...");
	ret = jn_mod_register(h, modname, &objid);
	if (ret < 0) {
		wprintf(L"[FAILED]\n");
	} else {
		wprintf(L"[DONE]\n");
		wprintf(L"ObjectID: %u\n", objid);
	}
}

void jnif_mod_regdir(jn_h * h)
{
	int ret;
	wprintf(L"Registering directory %S...", h->conf.mod_path);
	ret = jn_mod_register_dir(h);
	if (ret < 0) {
		wprintf(L"[FAILED]\n");
	} else {
		wprintf(L"[DONE]\n");
		wprintf(L"Registered %i entries.\n", ret);
	}
}

void jnif_pause_com(jn_h * h)
{
	wchar_t comname[JN_MAX_COM_NAME];
	jn_comod_entry *com;
	wprintf(L"comod name: ");
	wscanf(L"%S", comname);
	_lock_mx(&h->com_list->mx);
	for (com = h->com_list->first; com; com = com->next) {
		if (!wcscmp(com->copyname, comname)) {
			com->flags |= JN_COM_PAUSED;
			break;
		}
	}
	_unlock_mx(&h->com_list->mx);
	if (!com) {
		wprintf(L"[FAILED]\n");
	} else {
		wprintf(L"[DONE]\n");
	}
}

void jnif_resume_com(jn_h * h)
{
	wchar_t comname[JN_MAX_COM_NAME];
	jn_comod_entry *com;
	wprintf(L"comod name: ");
	wscanf(L"%S", comname);
	_lock_mx(&h->com_list->mx);
	for (com = h->com_list->first; com; com = com->next) {
		if (!wcscmp(com->copyname, comname)) {
			com->flags &= ~JN_COM_PAUSED;
			break;
		}
	}
	_unlock_mx(&h->com_list->mx);
	if (!com) {
		wprintf(L"[FAILED]\n");
	} else {
		wprintf(L"[DONE]\n");
	}
}

void jnif_reload_com(jn_h * h)
{
	int ret;
	wchar_t comname[JN_MAX_COM_NAME];
	jn_comod_entry *com;
	wprintf(L"comod name: ");
	wscanf(L"%S", comname);
	_lock_mx(&h->com_list->mx);
	for (com = h->com_list->first; com; com = com->next) {
		if (!wcscmp(com->copyname, comname)) {
			ret = jn_com_reload(h, com, NULL);
			break;
		}
	}
	_unlock_mx(&h->com_list->mx);
	if (!com || ret) {
		wprintf(L"[FAILED]\n");
	} else {
		wprintf(L"[DONE]\n");
	}
}

void jnif_close_com(jn_h * h)
{
	int ret;
	wchar_t comname[JN_MAX_COM_NAME];
	jn_comod_entry *com;
	jn_pkt pkt;
	jmodid_t modid = JACID_INVAL;

	wprintf(L"comod name: ");
	wscanf(L"%S", comname);

	_lock_mx(&h->com_list->mx);
	for (com = h->com_list->first; com; com = com->next) {
		if (!wcscmp(com->copyname, comname)) {
			modid = com->com_id;
			break;
		}
	}
	_unlock_mx(&h->com_list->mx);

	pkt.hdr.comod_id = modid;
	ret = _jn_close_com(h, NULL, &pkt);

	//ret = jn_close_com(h, NULL, NULL, modid);

	if (!com || ret) {
		wprintf(L"[FAILED]\n");
	} else {
		wprintf(L"[DONE]\n");
	}
}

void jnif_mod_help()
{
	wprintf(L"\nSupported commands:\n\n");
	wprintf(L"\tMLIST\tLists modules\n");
	wprintf(L"\tLIST\tLists COMODs\n");
	wprintf(L"\tPAUSE\tPause COMOD\n");
	wprintf(L"\tRESUME\tResume COMOD\n");
	wprintf(L"\tRELOAD\tReloads module, all COMODs will be affected\n");
	wprintf(L"\tCLOSE\tClose COMOD\n");
	wprintf(L"\tREG\tRegister a module\n");
	wprintf(L"\tREGDIR\tRegister a directory of modules\n");
	wprintf(L"\tUNREG\tUnregister a module\n");
	wprintf(L"\tEXIT\tExit to main menu\n");
}

void jnif_mod(jn_h * h)
{
	wchar_t cmd[MAX_CMD];
	wprintf
	    (L"\nModule Management Subsystem, type \'HELP\' to get a list of commands.\n");
 prompt:
	wprintf(L"\nMod >");
	wscanf(L"%S", cmd);
	switch (lookup_cmd(cmd)) {
	case MLIST:
		jnif_mod_list(h);
		break;
	case LIST:
		jnif_com_list(h);
		break;
	case REG:
		jnif_mod_reg(h);
		break;
	case UNREG:
		jnif_mod_unreg(h);
		break;
	case REGDIR:
		if (!jn_mod_register_dir(h)) {
			wprintf(L"[DONE]\n");
		} else {
			wprintf(L"[FAILED]\n");
		}
		break;
	case PAUSE:
		jnif_pause_com(h);
		break;
	case RESUME:
		jnif_resume_com(h);
		break;
	case RELOAD:
		jnif_reload_com(h);
		break;
	case CLOSE:
		jnif_close_com(h);
		break;
	case HELP:
		jnif_mod_help();
		break;
	case EXIT:
		return;
	default:
		wprintf(L"\nCommand not supported, try using \'HELP\'\n");
		break;
	}
	goto prompt;

}

void jnif_print_conf_flags(jn_h * h)
{
	if (h->flags & JN_IF_BIND)
		wprintf(L"BIND ");
	if (h->flags & JN_IF_LISTEN)
		wprintf(L"LISTEN ");
	if (h->flags & JN_IF_SOCKOPT)
		wprintf(L"SOCKET_OPTIONS ");
	if (h->flags & JN_IF_LOGIN)
		wprintf(L"LOGIN ");
	if (h->flags & JN_IF_CRYPT)
		wprintf(L"ENCRYPT ");
	if (h->flags & JN_IF_CRC)
		wprintf(L"CRC ");
	if (h->flags & JN_IF_GUEST)
		wprintf(L"GUEST ");
	if (h->flags & JN_IF_FREE_GRP)
		wprintf(L"FREE_GROUP ");
}

void jnif_stat(jn_h * h)
{
	wprintf(L"Server Status:\n\n");
	wprintf(L"Configuration:\n");
	wprintf(L"\tflags: ");
	jnif_print_conf_flags(h);
	wprintf(L"\n");
	//wprintf(L"\taddress: %s\n", h->conf.addr);
	wprintf(L"\tport: %s\n", h->conf.port);
	wprintf(L"\tmodule path: %S\n", h->conf.mod_path);
	wprintf(L"\tmaximum users: %u\n", h->conf.max_users);
	wprintf(L"\tmaximum modules: %u\n", h->conf.max_mod);
	wprintf(L"\tmaxumum comods: %u\n", h->conf.max_com);
	wprintf(L"\tmaximum user comods: %u\n", h->conf.max_ucom);
	wprintf(L"Current Status:\n");
	wprintf(L"\tpoll buckets: %u\n", h->poll_list->cnt);
	wprintf(L"\tmodule entries: %u\n", h->mod_list->cnt);
	wprintf(L"\tcomod entries: %u\n", h->com_list->cnt);
	wprintf(L"\tconnection entries: %u\n", h->nusers);
}

void help()
{
	wprintf(L"\nSupported commands:\n\n");
	wprintf(L"\tINIT\tInitialize jnet handle\n");
	wprintf(L"\tSTART\tStarts jnet server\n");
	wprintf(L"\tSTAT\tServer stat\n");
	wprintf(L"\tCONN\tConnection management subsystem\n");
	wprintf(L"\tMOD\tModule management subsystem\n");
	wprintf(L"\tSHUTDOWN\tShuts down the server\n");
	wprintf(L"\tEXIT\tExits jnet interface\n\n");
}

int main()
{
	wchar_t cmd[MAX_CMD];
	jn_h h;
	int ret;
	char jerr[MAX_ERR_STR];

	setlocale(LC_ALL, "");

	if (jn_init_svr(&h) < 0) {
		wprintf(L"\nInit failed.\n");
		return -1;
	}
	wprintf
	    (L"\njnet server interface version %s, type \'HELP\' to get a list of commands.\n",
	     VER);

 prompt:
	wprintf(L"\nMain >");
	wscanf(L"%S", cmd);

	switch (lookup_cmd(cmd)) {
	case INIT:
		wprintf(L"Initializing server handle...");
		ret = jn_init_svr(&h);
		if (ret < 0) {
			wprintf(L"\t[FAIL]\n");
			jer_lookup_str(ret, jerr);
			wprintf(L"\t\tError: %i, %s\n", ret, jerr);
		} else {
			wprintf(L"\t[ OK ]\n");
		}
		break;
	case START:
		wprintf(L"Creating server thread on port \'%s\' ...",
			h.conf.port);
		ret = jn_start_svr(&h);
		/*
		   if((ret=_create_thread(&thid, NULL, (void*)&jn_start_svr, (void*)&h)) == 0){
		   ret=_join_thread(thid, (void*)&ret);
		   }
		 */
		if (ret < 0) {
			wprintf(L"\t[FAIL]\n");
			//      _cancel_thread(thid);
			jer_lookup_str(ret, jerr);
			wprintf(L"\t\tError: %i, %s\n", ret, jerr);
		} else {
			wprintf(L"\t[ OK ]\n");
		}
		break;
	case STAT:
		jnif_stat(&h);
		break;
	case CONN:
		jnif_conn(&h);
		break;
	case MOD:
		jnif_mod(&h);
		break;
	case SHUTDOWN:
		wprintf(L"Shutting down...");
		jn_shutdown_svr(&h);
		wprintf(L"\t[ OK ]\n");
		break;
	case EXIT:
		wprintf(L"Shutting down...");
		jn_shutdown_svr(&h);
		return 0;
	case HELP:
		help();
		break;
	case CMD_NOP:
		break;
	default:
		wprintf(L"\nCommand not supported, try using \'HELP\'\n");
		break;
	}
	wcscpy(cmd, NOP);
	goto prompt;
	return 0;
}
