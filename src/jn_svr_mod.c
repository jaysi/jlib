#include "jnet.h"
#include "jnet_internals.h"
#include "debug.h"
#include "jer.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>
#include <locale.h>
#include <dlfcn.h>
#include <string.h>

#ifndef _SVID_SOURCE
#define _SVID_SOURCE
#endif
#include <dirent.h>

#define _wdeb1(f, a...)
#define _deb1(f, a...)
#define _deb2(f, a...)
#define _wdeb2 _wdeb

char *_jn_mod_path(jn_h * h, wchar_t * name)
{
	_wdeb2(L"called with name = %S, conf.mod_path = %S", name,
	       h->conf.mod_path);
	wchar_t *pathname;
	char *path8;
	_wdeb2(L"allocating %u bytes", WBYTES(h->conf.mod_path) + WBYTES(name));
	pathname = (wchar_t *) malloc(WBYTES(h->conf.mod_path) + WBYTES(name));
	wcscpy(pathname, h->conf.mod_path);
	wcscat(pathname, name);
	_wdeb2(L"allocating %u bytes", WBYTES(pathname));
	path8 = (char *)malloc(WBYTES(pathname));
	_wdeb2(L"wtojcs()");
	if (wtojcs(pathname, (uchar *) path8, 0) == ((size_t) - 1)) {
		free(pathname);
		free(path8);
		return NULL;
	}
	_wdeb2(L"returning %S", pathname);
	free(pathname);
	return path8;
}

int _jn_com_load(jn_h * h, jobjid_t objid, wchar_t * copyname,
		 jn_comod_entry ** ptr)
{
	struct stat s;
	char *path8;
//      wchar_t* path;
	jn_mod_entry *entry;
	jn_comod_entry *hmod;

	_wdeb2(L"Loading %S, objid = %u", copyname, objid);

	_lock_mx(&h->mod_list->mx);

	_wdeb2(L"mutex locked, searching for object id");

	for (entry = h->mod_list->first; entry; entry = entry->next) {
		if (objid == entry->objid) {
//                      path = (wchar_t*)malloc(WBYTES(entry->pathname));
//                      wcscpy(path, entry->pathname);
			break;
		}
	}
	_unlock_mx(&h->mod_list->mx);

	_wdeb2(L"mutex unlocked, reaching to somewhere");

	if (!entry) {
		return -JE_NOTFOUND;
	}

	_wdeb2(L"_jn_mod_path()");

	if (!(path8 = _jn_mod_path(h, entry->pathname))) {
		return -JE_INV;
	}

	_wdeb2(L"path8 = %s", path8);

	if (stat(path8, &s)) {
		_wdeb(L"stat() fails");
		return -JE_STAT;
	}

	if (!S_ISREG(s.st_mode)) {
//              _wdeb(L"< %S > is not a regular file", path);
		return -JE_NOFILE;
	}

	hmod = (jn_comod_entry *) malloc(sizeof(jn_comod_entry));
	if (!hmod)
		return -JE_MALOC;

	_wdeb2(L"dlopen()");

	hmod->handle = dlopen(path8, RTLD_LAZY);
	if (!hmod->handle) {
		free(path8);
//              free(path);
		return -JE_OPEN;
	}
	free(path8);
/*
	*(void **) (&_magic) = dlsym(hmod->handle, "jnmod_magic");
	if(!_magic){
		dlclose(hmod->handle);
		_wdeb(L"unable to resolve handle_packet()");
		free(path8);
		free(path);
		return -JE_RESOLV;
	}
	if(_magic() != JNMOD_MAGIC){
        	dlclose(hmod->handle);
        	_wdeb(L"bad magic");
        	free(path8);
        	free(path);
		return -JE_BADMOD;
	}
*/

	_wdeb1(L"loading symbols");
	*(void **)(&hmod->jnmod_init) = dlsym(hmod->handle, "jnmod_init");
	if (!hmod->jnmod_init) {
		dlclose(hmod->handle);
		_wdeb(L"unable to resolve init()");
		free(path8);
//              free(path);
		return -JE_RESOLV;
	}

	*(void **)(&hmod->jnmod_cleanup) = dlsym(hmod->handle, "jnmod_cleanup");
	if (!hmod->jnmod_cleanup) {
		dlclose(hmod->handle);
		_wdeb(L"unable to resolve cleanup()");
		free(path8);
//              free(path);
		return -JE_RESOLV;
	}

	*(void **)(&hmod->jnmod_handle_join) =
	    dlsym(hmod->handle, "jnmod_handle_join");
	if (!hmod->jnmod_handle_join) {
		dlclose(hmod->handle);
		_wdeb(L"unable to resolve join()");
		free(path8);
//              free(path);
		return -JE_RESOLV;
	}

	*(void **)(&hmod->jnmod_handle_leave) =
	    dlsym(hmod->handle, "jnmod_handle_leave");
	if (!hmod->jnmod_handle_leave) {
		dlclose(hmod->handle);
		_wdeb(L"unable to resolve leave()");
		free(path8);
//              free(path);
		return -JE_RESOLV;
	}

	*(void **)(&hmod->jnmod_handle_packet) =
	    dlsym(hmod->handle, "jnmod_handle_packet");
	if (!hmod->jnmod_handle_packet) {
		dlclose(hmod->handle);
		_wdeb(L"unable to resolve handle_packet()");
		free(path8);
//              free(path);
		return -JE_RESOLV;
	}

	hmod->copyname = (wchar_t *) malloc(WBYTES(copyname));
	wcscpy(hmod->copyname, copyname);

	hmod->objid = objid;

	hmod->modname = (wchar_t *) malloc(WBYTES(entry->pathname));
	wcscpy(hmod->modname, entry->pathname);

//      free(path);

	*ptr = hmod;

	_wdeb2(L"returning OK");

	return 0;
}

int jn_com_reload(jn_h * h, jn_comod_entry * hmod, jn_conn * conn)
{
	struct stat s;
	char *path8;
	int ret;

	void *handle;

	int (*jnmod_init) (void *h, void *com_h, void *vconn);
	int (*jnmod_cleanup) (void *vh, void *vcom_h);
	int (*jnmod_handle_join) (void *vh, void *vcom_h, void *vconn,
				  void *req_pkt, void *vrfifo_fn);
	int (*jnmod_handle_leave) (void *vh, void *vcom_h, void *vconn,
				   void *req_pkt, void *vrfifo_fn);
	int (*jnmod_handle_packet) (void *h, void *com_h, void *job,
				    void *rfifo_fn);

	_lock_mx(&hmod->somx);

	if (!(path8 = _jn_mod_path(h, hmod->copyname))) {
		ret = -JE_INV;
		goto end;
	}

	if (stat(path8, &s)) {
		_wdeb(L"stat() fails");
		ret = -JE_STAT;
		goto end;
	}

	if (!S_ISREG(s.st_mode)) {
//              _wdeb(L"< %S > is not a regular file", path);
		ret = -JE_NOFILE;
		goto end;
	}

	handle = dlopen(path8, RTLD_LAZY);
	if (!handle) {
		free(path8);
//              free(path);
		ret = -JE_OPEN;
		goto end;
	}
	free(path8);

	_wdeb1(L"loading symbols");
	jnmod_init = dlsym(handle, "jnmod_init");
	if (!jnmod_init) {
		dlclose(handle);
		_wdeb(L"unable to resolve init()");
		free(path8);
//              free(path);
		ret = -JE_RESOLV;
		goto end;
	}

	jnmod_cleanup = dlsym(handle, "jnmod_cleanup");
	if (!jnmod_cleanup) {
		dlclose(handle);
		_wdeb(L"unable to resolve cleanup()");
		free(path8);
//              free(path);
		ret = -JE_RESOLV;
		goto end;
	}

	jnmod_handle_join = dlsym(handle, "jnmod_handle_join");
	if (!jnmod_handle_join) {
		dlclose(handle);
		_wdeb(L"unable to resolve join()");
		free(path8);
//              free(path);
		ret = -JE_RESOLV;
		goto end;
	}

	jnmod_handle_leave = dlsym(handle, "jnmod_handle_leave");
	if (!jnmod_handle_leave) {
		dlclose(handle);
		_wdeb(L"unable to resolve leave()");
		free(path8);
//              free(path);
		ret = -JE_RESOLV;
		goto end;
	}

	jnmod_handle_packet = dlsym(handle, "jnmod_handle_packet");
	if (!jnmod_handle_packet) {
		dlclose(handle);
		_wdeb(L"unable to resolve handle_packet()");
		free(path8);
//              free(path);
		ret = -JE_RESOLV;
		goto end;
	}

 end:
	//assign handles        

	if (!ret) {
		dlclose(handle);

		hmod->handle = handle;
		hmod->jnmod_init = jnmod_init;
		hmod->jnmod_cleanup = jnmod_cleanup;
		hmod->jnmod_handle_join = jnmod_handle_join;
		hmod->jnmod_handle_leave = jnmod_handle_leave;
		hmod->jnmod_handle_packet = jnmod_handle_packet;

	}

	ret = jnmod_init((void *)h, (void *)hmod, (void *)conn);

	_unlock_mx(&hmod->somx);

	return ret;
}

int jn_mod_register(jn_h * h, wchar_t * name, jobjid_t * objid)
{
	int ret;
	jac_orec orec;
	if (!&h->jac)
		return -JE_NOJAC;

	_lock_mx(&h->jac_mx);
	jac_add_object(&h->jac, name, NULL);
	ret = jac_find_object(&h->jac, name, &orec);
	if (ret < 0)
		goto end;

	*objid = orec.objid;
 end:
	_unlock_mx(&h->jac_mx);
	return ret;
}

int jn_mod_register_dir(jn_h * h)
{
	struct dirent **namelist;
	int n;
	int ret;
	wchar_t name[JN_MAX_COM_NAME];
	char *path8;

	if (!h->conf.mod_path)
		return -JE_NULLPTR;
	if (!&h->jac)
		return -JE_NOJAC;

	path8 = (char *)malloc(WBYTES(h->conf.mod_path));
	wtojcs(h->conf.mod_path, (uchar *) path8, 0);

	n = scandir(path8, &namelist, 0, alphasort);
	if (n < 0) {
		_wdeb(L"scandir() fails");
		free(path8);
		return -JE_SYSE;
	}

	free(path8);

	ret = 0;
	_lock_mx(&h->jac_mx);
	while (n--) {
		if (CWBYTES(namelist[n]->d_name) < JN_MAX_COM_NAME) {
			jcstow((uchar *) namelist[n]->d_name, name, 0);
			if (!jac_add_object(&h->jac, name, NULL))
				ret++;
		}
	}
	_unlock_mx(&h->jac_mx);

	return ret;

}

int _jn_mod_list_buf(jn_h * h, uchar ** buf, uint32_t * bufsize,
		     jobjid_t * nmods)
{
	uint32_t pos;
	jn_mod_entry *entry;
	int ret = 0;
	_lock_mx(&h->mod_list->mx);
	if (h->mod_list->cnt == 0) {
		ret = -JE_EMPTY;
		goto end;
	}
	*nmods = h->mod_list->cnt;
	*bufsize = enc_data_size(h->mod_list->cnt * sizeof(jobjid_t));

	for (entry = h->mod_list->first; entry; entry = entry->next) {
		pos = wtojcs_len(entry->pathname, 0) + 1;	//1 for null termination               
		if (pos == ((size_t) - 1)) {
			_wdeb(L"failed at %S", entry->pathname);
			ret = -JE_INV;
			goto end;
		}
		_wdeb2(L"added %S, jcslen = %u", entry->pathname, pos);
		*bufsize += pos;
	}

	_wdeb2(L"creating mod list, nmods = %u, buffer_size = %u", *nmods,
	       *bufsize);

	*buf = (uchar *) malloc(*bufsize);
	pos = 0;
	for (entry = h->mod_list->first; entry; entry = entry->next) {
		packi32((*buf) + pos, entry->objid);
		pos += sizeof(jobjid_t);
		pos += wtojcs(entry->pathname, (*buf) + pos, 0);
		pos++;		//for null termination.
	}

	_wdeb2(L"finished creating mod list, pos = %u", pos);
	_dbuf(*buf, pos);

 end:
	_unlock_mx(&h->mod_list->mx);

	return ret;

}

int jn_send_modlist(jn_h * h, jn_conn * conn, jtransid_t req_transid)
{
	int ret;
	uchar *buf;
	uint32_t bufsize;
	jobjid_t nmods;
	jn_pkt pkt;
	//uint32_t total;
	uint16_t pos;
	uint16_t to_send;

	//there is no problem to use buf as datapool, malloc is rounded
	//to enc_data_size in call, see jn_mod_list_buf() code;
	if ((ret = _jn_mod_list_buf(h, &buf, &bufsize, &nmods)) < 0) {
		_wdeb(L"_jn_mod_list_buf() fails: %i", ret);
		jn_send_plain(conn, req_transid, JNT_FAIL, ret, NULL);
		return ret;
	}

	pkt.hdr.trans_id = req_transid;
	pkt.hdr.type = JNT_OK;
	pkt.hdr.flags = 0x00;

	if (bufsize > JN_MAX_PKT_DATA) {
		to_send = JN_MAX_PKT_DATA;
		pkt.hdr.rs = bufsize - to_send;
	} else {
		to_send = bufsize;
		pkt.hdr.rs = 0L;
	}

	pos = 0;
	while (to_send) {
		//malloc
		pkt.hdr.data_len = to_send;
		pkt.data = buf + pos;
		pos += to_send;

		//send async
		if ((ret = jn_send_poll(conn, &pkt, 0)) < 0) {
			free(buf);
			return ret;
		}
		//calc to_send  & rs    
		if (pkt.hdr.rs > JN_MAX_PKT_DATA) {
			to_send = JN_MAX_PKT_DATA;
			pkt.hdr.rs -= JN_MAX_PKT_DATA;
		} else {
			to_send = pkt.hdr.rs;
			pkt.hdr.rs = 0L;
		}
	}

	free(buf);

	return 0;
}

int _jn_mod_create_list(jn_h * h)
{
	struct dirent **namelist;
	int n;
	int ret;
	jac_orec orec;
	jn_mod_entry *entry;
	char *path8;

	_wdeb(L"creating list, mod_path = %S", h->conf.mod_path);

	if (!h->conf.mod_path)
		return -JE_NULLPTR;
	if (!&h->jac)
		return -JE_NOJAC;

	_deb1("ok");

	path8 = (char *)malloc(WBYTES(h->conf.mod_path));
	wtojcs(h->conf.mod_path, (uchar *) path8, 0);

	n = scandir(path8, &namelist, 0, alphasort);
	if (n < 0) {
		_wdeb(L"scandir() fails");
		free(path8);
		return -JE_SYSE;
	}

	free(path8);

	_lock_mx(&h->mod_list->mx);

	while (n--) {

		entry = (jn_mod_entry *) malloc(sizeof(jn_mod_entry));
		entry->pathname =
		    (wchar_t *) malloc(CWBYTES(namelist[n]->d_name));
		jcstow((uchar *) namelist[n]->d_name, entry->pathname, 0);

		_lock_mx(&h->jac_mx);
		ret = jac_find_object(&h->jac, entry->pathname, &orec);
#ifndef NDEBUG
		if (ret)
			_wdeb(L"Object %S not found in jac, ret = %i",
			      entry->pathname, ret);
#endif
		_unlock_mx(&h->jac_mx);

		if (!ret) {
			entry->objid = orec.objid;
			if (h->mod_list->first == NULL) {
				h->mod_list->first = entry;
				h->mod_list->last = entry;
				h->mod_list->cnt = 1L;
			} else {
				h->mod_list->last->next = entry;
				h->mod_list->last = h->mod_list->last->next;
				h->mod_list->cnt++;
			}
		} else {
			free(entry->pathname);
			free(entry);
		}

		_wdeb1("jac_find_object returns %i for < %S >", ret,
		       entry->pathname);
	}

	_unlock_mx(&h->mod_list->mx);

	free(namelist);

	return 0;

}

/*
jn_mod_entry* _jn_find_mod(jn_h* h, wchar_t* name){
	
	jn_mod_entry* entry;
	
	_lock_mx(&h->mod_list->mx);
	
	entry = h->mod_list->first;
	while(entry){
		if(!wcscmp(name, entry->pathname)){
			_unlock_mx(&h->mod_list->mx);
			return entry;
		}
		entry = entry->next;
	}
	
	_unlock_mx(&h->mod_list->mx);
	
	return NULL;
}

int _jn_add_mod(jn_h* h, wchar_t* copyname){
	
	jmodid_t* mod_id;
	jmodid_t cnt;
	
	return -JE_IMPLEMENT;
	
	if(_jn_find_mod(h, copyname)) return -JE_EXISTS;
	
	mod_id = (jmodid_t*)malloc(sizeof(jmodid_t)*h->max_mods);
	
	for(cnt = 0; cnt < h->max_mods; cnt++){
		mod_id[cnt]  = cnt;
	}
	
	
}

int _jn_rm_com(jn_h* h, wchar_t* copyname){
	return -JE_IMPLEMENT;
}
*/
