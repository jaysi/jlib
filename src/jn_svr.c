#include "jnet.h"
#include "jer.h"
#include "ecc.h"
#include "debug.h"
#include "jcs.h"
#include "jrand.h"
#include "jhash.h"
#include "sha256.h"

#include "jnet_internals.h"

#ifndef __WIN32
#include "unistd.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/in.h"
#include <netdb.h>
#else
#include "winsock2.h"
#include "ws2tcpip.h"
#endif
#include "sys/types.h"
#include "sys/stat.h"
#include "string.h"
#include "wchar.h"
#include "stdio.h"
#include "stdlib.h"
#include "signal.h"
#include "errno.h"
#ifdef linux
#include "error.h"
#include "sys/wait.h"
#endif
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define _deb1(f, a...)
#define _deb2(f, a...)
#define _wdeb1 _wdeb		//old, hs, login
#define _deb3(f, a...)
#define _wdeb3(f, a...)
#define _wdeb4 _wdeb		//com request
#define _wdeb5 _wdeb		//accept, fw test
#define _wdeb6(f, a...)		//new packet queue system
#define _wdeb7(f, a...)		//com open debug
#define _wdeb8(f, a...)		//packet queue
#define _wdeb9(f, a...)		//dumps plain packet contents in jcs
#define _wdeb10 _wdeb		//new conn proc buf code
#define _wdeb12 _wdeb		//closing connection

//see jac.c
extern int _check_name(wchar_t * name, size_t max_size);

/*
	checked permissions:
	
		start/pause/stop server: o | x to the server
		server_user_list: o | x to the server
		login to server: being in the jac!
		
		upload/update mod: o | w to the server
		*list mod: o | x | r to the server
		
		*list comod: o | x | r to the server
		*start/pause/stop comod: o | x to the mod OR o | x to the server
		*join comod: o | rw  to the mod OR o | x to the server
		*com_user_list: comod decides OR o | x to the server NOT IMPLEMENTED @ server level
		
		user_kick from comod:	o | x to the mod object or server
		user_ban/unban from server: o | x to the server
*/
/*
static inline int _jn_perm_svr_ox(jac_h * jac, wchar_t * svr_obj_name,
				  wchar_t * username)
{
	uchar otp;
	if (!jac_find_object_dep(jac, svr_obj_name, username, JAC_USER, &otp)) {
		if ((otp & JOBJ_OWNER) || (otp & JPERM_EX)) {
			return 0;
		}
	}

	return -JE_NOPERM;
}
*/

int jn_perm(jn_h * h, jn_conn * conn, wchar_t * obj_name, uchar ptype)
{
	int ret = -JE_NOPERM;
	uchar otp;

	_wdeb(L"WARNING: PERMISSIONS DISABLED DUE TO JAC->JDB MIGRATION!");
	return 0;
	/*

	_lock_mx(&h->jac_mx);

	switch (ptype) {
	case JNT_MODLIST:
	case JNT_COMLIST:
		if (!jac_find_object_dep
		    (&h->jac, h->svr_obj_name, conn->username, JAC_USER,
		     &otp)) {

			if ((otp & JOBJ_OWNER) || (otp & JPERM_EX) ||
			    (otp & JPERM_RD)) {

				ret = 0;

			}
		}
		break;

	case JNT_COMOPEN:
	case JNT_COMCLOSE:
	case JNT_COMPAUSE:

		ret = _jn_perm_svr_ox(&h->jac, h->svr_obj_name, conn->username);		

		if (ret) {
			if (!jac_find_object_dep
			    (&h->jac, obj_name, conn->username, JAC_USER,
			     &otp)) {

				if ((otp & JOBJ_OWNER) || (otp & JPERM_EX)) {

					ret = 0;

				}
			}
		}

		break;

	case JNT_COMJOIN:

		ret = _jn_perm_svr_ox(&h->jac, h->svr_obj_name, conn->username);

		if (ret) {
			if (!jac_find_object_dep
			    (&h->jac, obj_name, conn->username, JAC_USER,
			     &otp)) {
				if ((otp & JOBJ_OWNER)
				    || (otp & (JPERM_RD | JPERM_WR))) {

					ret = 0;

				}
			}
		}
		break;

	case JNT_SVRSHUTDOWN:
	case JNT_SVRPAUSE:
	case JNT_SVRRESUME:
	case JNT_SVRUSERLIST:

		ret = _jn_perm_svr_ox(&h->jac, h->svr_obj_name, conn->username);

		break;

	}

	_unlock_mx(&h->jac_mx);

	return ret;
	*/
}

int jn_socket(jn_h * h)
{
	int fd;
	struct addrinfo hints, *servinfo, *p;
	//struct sockaddr_storage their_addr; // connector's address information
	//socklen_t sin_size;
	//struct sigaction sa;
	int yes = 1;
	//char s[INET6_ADDRSTRLEN];
	int rv;

#ifdef _WIN32
	int sizeofyes = sizeof(int);
#endif

#ifndef NDEBUG
	jmxattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;	// use my IP

#ifdef __WIN32
	WSADATA wsaData;
	if (WsaStartup(MAKEWORD(1, 1), &wsaData)) {
		return -JE_SYSE;
	}
#endif
	if ((rv = getaddrinfo(NULL, h->conf.port, &hints, &servinfo)) != 0) {
		return -JE_SYSE;
	}
	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((fd = socket(p->ai_family, p->ai_socktype,
				 p->ai_protocol)) == -1) {
			//perror("server: socket");
			continue;
		}

#ifndef _WIN32
		if (setsockopt
		    (fd, h->conf.so_level, h->conf.so_optname, &yes,
		     sizeof(int)) == -1) {
			return -JE_SETSOCKOPT;
		}
#else
		if (setsockopt
		    (fd, h->conf.so_level, h->conf.so_optname, (char*)&yes,
		     sizeofyes) == -1) {
			return -JE_SETSOCKOPT;
		}
		
#endif
		if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
			close(fd);
			//perror("server: bind");
			continue;
		}

		break;
	}

	if (!p) {
		return -JE_BIND;
	}

	freeaddrinfo(servinfo);	// all done with this structure

	if (listen(fd, h->conf.nlisten) == -1) {
		close(fd);
		return -JE_LISTEN;
	}

	if (fd >= 0) {

		h->flags = JNF_SVR;

		h->syse = 0;

		h->poll_list = (jn_poll_list *) malloc(sizeof(jn_poll_list));
		if (!h->poll_list)
			return -JE_MALOC;
		h->poll_list->first = NULL;
		h->mod_list = (jn_mod_list *) malloc(sizeof(jn_mod_list));
		if (!h->mod_list) {
			free(h->poll_list);
			return -JE_MALOC;
		}
		h->mod_list->first = NULL;
		h->com_list = (jn_comod_list *) malloc(sizeof(jn_comod_list));
		if (!h->com_list) {
			free(h->poll_list);
			free(h->mod_list);
			return -JE_MALOC;
		}
		h->com_list->first = NULL;
		//h->conn_list = _jn_init_conn_list();
		h->msg_list = NULL;
		h->block_list = (jn_block_list *) malloc(sizeof(jn_block_list));
		if (!h->block_list) {
			free(h->poll_list);
			free(h->mod_list);
			free(h->com_list);
			return -JE_MALOC;
		}
		h->block_list->first = NULL;

		//h->queue = NULL;
		//_init_sem(&h->qsem);

		h->fifo = _jn_fifo_new();
		h->reply_fifo = _jn_fifo_new();
#ifndef NDEBUG
		_wdeb1(L"Initing mutexes");
		_init_mx(&h->jac_mx, &attr);
		_init_mx(&h->poll_list->mx, &attr);
		_init_mx(&h->mod_list->mx, &attr);
		_init_mx(&h->block_list->mx, &attr);
		_init_mx(&h->com_list->mx, &attr);
		_init_mx(&h->comlist_buf_mx, &attr);
		//_init_mx(&h->qmx, &attr);
		_wdeb1(L"finished init mx");
#else

		_init_mx(&h->jac_mx, NULL);
		_init_mx(&h->poll_list->mx, NULL);
		_init_mx(&h->mod_list->mx, NULL);
		_init_mx(&h->block_list->mx, NULL);
		_init_mx(&h->com_list->mx, NULL);
		_init_mx(&h->comlist_buf_mx, NULL);
		//_init_mx(&h->qmx, NULL);
#endif
		h->poll_list->cnt = 0L;
		h->mod_list->cnt = 0L;
		h->block_list->cnt = 0L;
		h->com_list->cnt = 0L;
		h->comlist_bufsize = 0L;
		h->nusers = 0L;

		h->magic = JN_MAGIC;

		h->svr_obj_name = JN_SVR_OBJ_NAME;

	}

	_wdeb1(L"assigned %i", fd);
	return fd;
}

/*
void jn_shutdown(jn_h* h){

	if(h->magic != JN_MAGIC) return;	
		
	close(h->sockfd);
	
	_cancel_thread(h->accept_th);
	
	free(h->mod_path);
	
	if(h->jac){
		_lock_mx(&h->jac_mx);
		jac_shutdown(h->jac);
		free(h->jac);
		h->jac = NULL;
		_unlock_mx(&h->jac_mx);
		_destroy_mx(&h->jac_mx);
	}
	
	_jn_free_conn_list(h->conn_list);
	_jn_free_com_list(h->com_list);
			
	h->magic = 0x0000;
}
*/

int jn_close(jn_h * h, jn_conn * conn, uchar flags)
{
	jn_conn *entry;		//, *prev;
	int ret;
	jn_fifo_entry *job;
	jn_ucom_entry *ucentry;
	jn_comod_entry *comentry;
	jmodid_t *comids;
	uint32_t i, j;
//      jn_conn* conn;

	_deb2("closing fd %i", sockfd);

//      if(flags & JN_CF_CLOSENOW){
#ifndef _WIN32
	close(conn->sockfd);
#else
	closesocket(conn->sockfd);
#endif
//      } else {
//              shutdown(sockfd, SHUT_RD);
//      }

	if (h) {
		if (h->magic != JN_MAGIC)
			return -JE_MAGIC;

		//remove from poll list

		_lock_mx(&h->poll_list->mx);
		entry =
		    _jn_rm_poll_fd(h, (jn_poll_buck *) conn->poll_buck,
				   conn->sockfd);
		_unlock_mx(&h->poll_list->mx);

#ifndef NDEBUG
		if (!entry) {
			_wdeb(L"not found");
		}
#endif

		_wdeb(L"WARNING: LOGOUT DISABLED DUE TO JAC->JDB MIGRATION");
/*
		//logout

		_lock_mx(&h->jac_mx);
		_wdeb12(L"logging out < %S >", entry->username);
		if ((ret = jac_logout(&h->jac, entry->username)) < 0) {
			_wdeb(L"faild to logout < %S >", entry->username);
			_unlock_mx(&h->jac_mx);
			return ret;
		}
		_unlock_mx(&h->jac_mx);
*/
		//remove from comods
		_wdeb12(L"removing from comods, cnt = %u",
			entry->ucom_list->cnt);
		//_lock_mx(&entry->mx);
		comids =
		    (jmodid_t *) malloc(sizeof(jmodid_t) *
					(entry->ucom_list->cnt));
		if (comids) {
			for (i = 0; i < entry->ucom_list->cnt; i++) {
				_wdeb12(L"adding comod %S",
					entry->ucom_list->first->copyname);
				comids[i] = entry->ucom_list->first->mod_id;
				ucentry = entry->ucom_list->first;
				entry->ucom_list->first =
				    entry->ucom_list->first->next;
				free(ucentry->copyname);
				free(ucentry);
			}
		}
		//_unlock_mx(&entry->mx);
		free(entry->ucom_list);

		_lock_mx(&h->com_list->mx);
		for (comentry = h->com_list->first; comentry;
		     comentry = comentry->next) {

			job = (jn_fifo_entry *) malloc(sizeof(jn_fifo_entry));
			if (job) {
				job->next = NULL;
				job->conn = entry;
				job->type = JN_FT_RM_CONN;
				_wdeb(L"WARNING: DISABLED JACID_INVAL ASSIGNMENT");
				//job->uid = JACID_INVAL;
			}

			for (j = 0; j < i; j++) {
				if (comentry->com_id == comids[j]) {
					//submit RM_CONN queue
					_wdeb12
					    (L"submitted request for comod %S",
					     comentry->copyname);
					_jn_com_req_inc(comentry);

					_jn_fifo_enq(comentry->fifo, job);
				}
			}

		}
		_unlock_mx(&h->com_list->mx);

		_wdeb12(L"got here");

		//this one comes from _login call               
		_jn_conn_req_dec(entry);

		//free(job);

		_wdeb12(L"got here");

 wait_again:
		sleep(JN_CONN_CLOSE_WAIT_TIME);
		if (_jn_conn_req_chk(entry)) {
			goto wait_again;
		}
		//_destroy_mx(&conn->mx);

		free(entry->username);

		//free(job);

		h->nusers--;

		_wdeb(L"-> connection closed <-");

		entry->uid = JACID_INVAL;

		return 0;

	} else {
		return -JE_IMPLEMENT;
	}
}

static inline int _jn_proc_svrreq(jn_h * h, jn_conn * conn, jn_pkt * pkt)
{
	int ret = 0;
	jn_comod_entry *com;

	_wdeb4(L"got request from < %S >, 0x%x", conn->username, pkt->hdr.type);

	switch (pkt->hdr.type) {
	case JNT_DC:
		_jn_conn_req_dec(conn);	//from poll
		//_jn_conn_req_dec(conn); //from login, moved to
		//jn_close()
		free(pkt);
		ret = jn_close(h, conn, 0);
		return ret;
		break;

	case JNT_COMLIST:

		ret = jn_send_comlist(h, conn, pkt->hdr.trans_id);
		break;

	case JNT_MODLIST:
		ret = jn_send_modlist(h, conn, pkt->hdr.trans_id);
		break;

	case JNT_COMOPEN:	//has payload
		_wdeb7(L"Open com request from %S, TID: %u", conn->username,
		       pkt->hdr.trans_id);
		ret = _jn_create_com(h, conn, pkt);
		free(pkt->data);
		break;

	case JNT_COMJOIN:
		ret = _jn_join_com(h, conn, pkt);
		break;

	case JNT_COMLEAVE:
		ret = _jn_leave_com(h, conn, pkt);
		break;

	case JNT_COMCLOSE:
		ret = _jn_close_com(h, conn, pkt);
		break;

	case JNT_COMPAUSE:

		_lock_mx(&h->com_list->mx);
		for (com = h->com_list->first; com; com = com->next) {
			if (com->com_id == pkt->hdr.comod_id) {
				if (com->owner_id == conn->uid) {
					com->flags |= JN_COM_PAUSED;
					_unlock_mx(&h->com_list->mx);
					jn_send_plain(conn,
						      pkt->hdr.trans_id,
						      JNT_OK, 0, NULL);
					ret = 0;
					goto ending1;
					break;
				} else {
					_unlock_mx(&h->com_list->mx);
					jn_send_plain(conn,
						      pkt->hdr.trans_id,
						      JNT_FAIL,
						      -JE_NOPERM, NULL);
					ret = -JE_NOPERM;
					goto ending1;
					break;
				}
			}
		}
		_unlock_mx(&h->com_list->mx);
		ret = -JE_NOTFOUND;

		break;

	case JNT_COMRESUME:
		_lock_mx(&h->com_list->mx);
		for (com = h->com_list->first; com; com = com->next) {
			if (com->com_id == pkt->hdr.comod_id) {
				if (com->owner_id == conn->uid) {
					com->flags &= ~JN_COM_PAUSED;
					_unlock_mx(&h->com_list->mx);
					jn_send_plain(conn,
						      pkt->hdr.trans_id,
						      JNT_OK, 0, NULL);
					ret = 0;
					goto ending1;
					break;
				} else {
					_unlock_mx(&h->com_list->mx);
					jn_send_plain(conn,
						      pkt->hdr.trans_id,
						      JNT_FAIL,
						      -JE_NOPERM, NULL);
					ret = -JE_NOPERM;
					goto ending1;
					break;
				}
			}
		}
		_unlock_mx(&h->com_list->mx);
		ret = -JE_NOTFOUND;

		break;
	case JNT_USERKICK:
		ret = -JE_IMPLEMENT;
		break;

	case JNT_USERBAN:
		ret = -JE_IMPLEMENT;
		break;

	case JNT_USERUNBAN:
		ret = -JE_IMPLEMENT;
		break;

	case JNT_SVRSHUTDOWN:
		ret = -JE_IMPLEMENT;
		break;
	default:
		ret = -JE_UNK;
		break;

	}

 ending1:
	free(pkt);
	_jn_conn_req_dec(conn);	//from poll

	return ret;
}

static inline int _jn_proc_comreq(jn_h * h, jn_conn * conn, jn_pkt * pkt)
{
	int ret = 0;
	jn_fifo_entry *job;
	jn_comod_entry *com;
	jn_cc_entry *cc;

	_wdeb4(L"got com request from < %S >, comid = %u",
	       conn->username, pkt->hdr.comod_id);

#ifndef NDEBUG
	if (pkt->hdr.flags & JNF_PLAIN) {
		_wdeb9
		    (L"username: %S, comid: %u, tid: %u, datalen: %u, jcs: %s",
		     conn->username, pkt->hdr.comod_id, pkt->hdr.trans_id,
		     pkt->hdr.data_len, pkt->data);
	}
#endif

	//1. find comod
	//2. check membership
	//2. assemble job
	//3. enqueue job

	//_jn_conn_req_dec(conn);//no need to lock, the com-thread
	//has already locked the conn from join/open

	_lock_mx(&h->com_list->mx);
	for (com = h->com_list->first; com; com = com->next) {
		if (com->com_id == pkt->hdr.comod_id) {
			if (com->flags & JN_COM_PAUSED) {
				_unlock_mx(&h->com_list->mx);
				jn_send_plain(conn,
					      pkt->hdr.trans_id,
					      JNT_FAIL, -JE_PAUSE, NULL);
				_jn_com_req_inc(com);
				return -JE_PAUSE;
			}

			_jn_com_req_inc(com);

			break;
		}
	}
	_unlock_mx(&h->com_list->mx);

	if (!com)
		return -JE_NOTFOUND;

	_lock_mx(&com->cc_list->mx);
	for (cc = com->cc_list->first; cc; cc = cc->next) {
		if (cc->conn->uid == conn->uid)
			break;
	}
	_unlock_mx(&com->cc_list->mx);

	if (!cc) {

		_jn_com_req_dec(com);

		return -JE_NOPERM;
	}

	job = (jn_fifo_entry *) malloc(sizeof(jn_fifo_entry));
	if (!job) {

		_jn_com_req_dec(com);

		return -JE_MALOC;
	}

	job->flags = 0x00;
	job->type = JN_FT_PKT;
	job->datalen = sizeof(jn_hdr) + pkt->hdr.data_len;
	job->uid = conn->uid;
	job->conn = conn;
	job->ptr = (void *)pkt;
	job->next = NULL;

	_jn_fifo_enq(com->fifo, job);

	_wdeb4(L"queued");

	return ret;
}

void *_jn_pkt_queue_thread(void *arg)
{

	th_arg *ca = (th_arg *) arg;
	jn_h *h = ca->h;
	jn_poll_buck *buck = ca->buck;
	jn_fifo_entry *nextq, *job;
	int cnt;
	int ret;
	uint16_t npkt, n;
	jn_pkt_list *plist, *entry, *delete;

	for (;;) {

		nextq = _jn_fifo_wait(buck->fifo);
		_wdeb8(L"got q");

		switch (nextq->type) {

		case JN_FT_PRE_PKT:
			/*
			   leave it for now, you'll need to use mutex
			   on conn->buf to make it work properly
			 */
			break;

		case JN_FT_PKT:

			_wdeb8(L"got packet q from %S", nextq->conn->username);
			ret = _jn_conn_proc_buf(nextq->conn, &plist,
						&npkt, nextq->ptr,
						(uint16_t) nextq->datalen);
			if (!ret) {
				_wdeb10(L"Got %u packets", npkt);
				entry = plist;
				for (n = 0; n < npkt; n++) {

					//already increased by one in poll
					if (n)
						_jn_conn_req_inc(nextq->conn);

					if (entry->pkt->hdr.flags & JNF_LOST) {
						jn_send_plain(nextq->conn,
							      entry->pkt->
							      hdr.trans_id,
							      JNT_FAIL,
							      -JE_PKTLOST,
							      NULL);
						free(entry->pkt);
						_jn_conn_req_dec(nextq->conn);
						goto next;
					}

					if (entry->pkt->hdr.type == JNT_COMREQ) {
						if (_jn_proc_comreq(h,
								    nextq->conn,
								    entry->pkt)
						    < 0) {

							_jn_conn_req_dec
							    (nextq->conn);

						}
						//this will decrease conn->nreq
					} else {
						job = (jn_fifo_entry *)
						    malloc(sizeof
							   (jn_fifo_entry));
						job->flags = 0x00;
						job->type = JN_FT_PKT;
						job->datalen =
						    sizeof(jn_hdr) +
						    entry->pkt->hdr.data_len;
						job->uid = nextq->conn->uid;
						job->conn = nextq->conn;
						job->ptr = (void *)entry->pkt;
						job->next = NULL;

						_jn_fifo_enq(h->fifo, job);

					}
 next:
					free(nextq);
					delete = entry;
					entry = entry->next;
					free(delete);
				}
			}
			break;

		default:
			_wdeb(L"Unknown Q type: 0x%x", nextq->type);
			break;
		}

	}
}

void *_jn_svr_fifo_thread(void *arg)
{
	jn_h *h = (jn_h *) arg;
	jn_fifo_entry *q;
	uint16_t nq = 0;

	for (;;) {

		q = _jn_fifo_wait(h->fifo);

		switch (q->type) {
		case JN_FT_PKT:
			_jn_proc_svrreq(h, q->conn, (jn_pkt *) q->ptr);
			break;
		default:
			break;
		}

		free(q);

	}

	return NULL;
}

/*
void* _jn_svr_reply_fifo_thread(void* arg){
	jn_h* h = (jn_h*)arg;
	
	for(;;){
	
	}
	
	return NULL;
}
*/

void _jn_sigchld_handler(int s)
{
	while (wait(NULL) > 0) ;
}

int jn_init_svr(jn_h * h)
{
	int ret = 0;
#ifndef _WIN32	
	struct sigaction sa;
#endif	

	if (h->magic == JN_MAGIC)
		return -JE_ALREADYINIT;

	_wdeb1(L"mutexes init'ed");

#ifndef _WIN32
	sa.sa_handler = _jn_sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		ret = -JE_SYSE;
		goto end;
	}

	_wdeb1(L"signal handler set");
#endif

	jn_default_conf(&h->conf);
	h->sockfd = jn_socket(h);
	if (h->sockfd < 0) {
		ret = -JE_SOCKET;
	}

	_wdeb1(L"got socket fd");

	h->magic = JN_MAGIC;
	h->flags = 0;

 end:
	if (ret < 0) {
		h->magic = 0x0000;
	}

	_wdeb1(L"returns %i", ret);

	return ret;

}

int jn_start_svr(jn_h * h)
{

	int ret = 0;
	
	_wdeb(L"WARNING: DISABLED JAC ACCESS");
	//jac_orec orec;

	h->flags = JNF_SVR;

	if (h->magic != JN_MAGIC)
		return -JE_NOINIT;

	_wdeb1(L"openning jac");
	assert(sizeof(jn_hdr) == AES_BSIZE);
	
	ret = jac_open(&h->jac, L"jn.jac", L"helo", JHF_CRYPT | JHF_CRC);
	if (ret < 0) {
		_wdeb(L"failing %i ...", ret);
		return ret;
	}
	
	_wdeb1(L"checking for server object < %S >", h->svr_obj_name);

	ret = jac_find_object(&h->jac, h->svr_obj_name, &orec);
	if (ret < 0) {
		if (ret == -JE_NOTFOUND) {
			ret = jac_add_object(&h->jac, h->svr_obj_name, NULL);
			if (ret < 0) {
				_wdeb(L"failing %i", ret);
				return ret;
			}
		} else {
			_wdeb(L"failing %i", ret);
			return ret;
		}
	}

	_wdeb1(L"creating mod list...");

	ret = _jn_mod_create_list(h);
	if (ret < 0) {
		_wdeb(L"failing %i ...", ret);
		return ret;
	}

	_wdeb1(L"passed");

	if (_create_thread(&(h->accept_th), NULL, _jn_accept_thread, (void *)h)
	    < 0) {
		ret = -JE_THREAD;
		goto end;
	}

	pthread_detach(h->accept_th);

	if (_create_thread(&(h->fifo_th), NULL, _jn_svr_fifo_thread, (void *)h)
	    < 0) {
		ret = -JE_THREAD;
		goto end;
	}

	pthread_detach(h->fifo_th);

	/*
	   if(_create_thread(&(h->reply_fifo_th), NULL, _jn_svr_reply_fifo_thread, (void*)h) < 0){
	   ret = -JE_THREAD;
	   goto end;
	   }

	   pthread_detach(h->reply_fifo_th);
	 */

/*
	if(_join_thread(h->accept_th, NULL) < 0){
		_wdeb(L"failed to join thread");
		_cancel_thread(h->accept_th);
		ret = -JE_THREADJOIN;
		goto end;
	}
*/
 end:
	_wdeb1(L"returning %i ...", ret);
	return ret;
}

void jn_shutdown_svr(jn_h * h)
{

	if (h->magic != JN_MAGIC)
		return;

	if (h->flags & JNF_SVR) {

#ifndef _WIN32
		close(h->sockfd);
#else
		closesocket(h->sockfd);
#endif

		//not needed the accept thread should exit on error
		//_wdeb(L"cancelling accept thread...");
		//_cancel_thread(h->accept_th);
		_wdeb(L"cancelling fifo thread...");
		_join_thread(h->fifo_th, NULL);
		_cancel_thread(h->fifo_th);

		_lock_mx(&h->jac_mx);

		_wdeb(L"logging out everyone...");
		jac_logout_all(&h->jac);
		_wdeb(L"shutting down jac...");
		jac_shutdown(&h->jac);

		_unlock_mx(&h->jac_mx);

		h->flags &= ~JNF_SVR;

	}

	_wdeb(L"> server shutdown <");

	h->magic = 0;
}

#ifdef USE_JNPOLL
int _jn_init_read_socket()
{
	int fd, sockfd;
	struct addrinfo hints, *servinfo, *p;
	//struct sockaddr_storage their_addr; // connector's address information
	//socklen_t sin_size;
	//struct sigaction sa;
	int yes = 1;
	//char s[INET6_ADDRSTRLEN];
	int rv;
	struct sockaddr_storage addr;
	socklen_t len;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;	// use my IP

	WSADATA wsaData;
	if (WsaStartup(MAKEWORD(1, 1), &wsaData)) {
		return -JE_SYSE;
	}

	if ((rv = getaddrinfo(NULL, JN_WIN_PIPE_PORT, &hints, &servinfo)) != 0) {
		return -JE_SYSE;
	}
	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((fd = socket(p->ai_family, p->ai_socktype,
				 p->ai_protocol)) == -1) {
			//perror("server: socket");
			continue;
		}

		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
		    == -1) {
			return -JE_SETSOCKOPT;
		}

		if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
			closesocket(fd);
			//perror("server: bind");
			continue;
		}

		break;
	}

	if (!p) {
		return -JE_BIND;
	}

	freeaddrinfo(servinfo);	// all done with this structure

	if (listen(fd, 1) == -1) {
		closesocket(fd);
		return -JE_LISTEN;
	}

	len = sizeof(struct sockaddr);

 get_again:
	if ((sockfd = accept(fd, (struct sockaddr *)&addr, &len)) == -1) {
		return -JE_ACCEPT;
	}
	//drop the connection if this is not from localhost
	/*
	   if(addr != localhost){               
	   closesocket(sockfd);
	   goto get_again;
	   }
	 */

	closesocket(fd);

	return sockfd;
}

int _jn_init_write_socket()
{
	int ret;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int sockfd memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(JN_WIN_PIPE_HOSTNAME, JN_WIN_PIPE_PORT, &hints,
			      &servinfo)) != 0) {
		_wdeb3(L"getaddrinfo: %s\n", gai_strerror(rv));
		return -JE_SYSE;
	}
	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				     p->ai_protocol)) == -1) {
			_perr("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			closesocket(sockfd);
			_perr("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		_wdeb3(L"client: failed to connect\n");
		return -JE_CONNECT;
	}
	return sockfd;
}

#endif

//fd[0] read, fd[1] write
int _jn_get_pipe(int fd[2])
{
#ifndef USE_JNPOLL
	if (pipe(fd))
		return -1;
	return 0;
#else

#endif
}

int _jn_read_pipe(int fd, jn_ppkt * pkt)
{
#ifndef USE_JNPOLL
	if (hard_read(fd, (char *)&pkt->hdr, sizeof(jn_phdr)) !=
	    sizeof(jn_phdr))
		return -JE_READ;
	//if(pkt->hdr.data_len > JN_CONF_MAX_PDATA) return -JE_LIMIT;
	if (hard_read(fd, (char *)pkt->data, pkt->hdr.data_len) !=
	    pkt->hdr.data_len)
		return -JE_READ;
#else
	if (recv(fd, (char *)&pkt->hdr, sizeof(jn_phdr)) != sizeof(jn_phdr))
		return -JE_READ;
	if (recv(fd, (char *)pkt->data, pkt->hdr.data_len) != pkt->hdr.data_len)
		return -JE_READ;
#endif
	return 0;

}

int _jn_write_pipe(int fd, jn_ppkt * pkt)
{
	//if(pkt->hdr.data_len > JN_CONF_MAX_PDATA) return -JE_LIMIT;
#ifndef USE_JNPOLL
	if (hard_write(fd, (char *)pkt, sizeof(jn_phdr) + pkt->hdr.data_len) !=
	    sizeof(jn_phdr) + pkt->hdr.data_len)
		return -JE_WRITE;
#else
	if (send(fd, (char *)pkt, sizeof(jn_phdr) + pkt->hdr.data_len) !=
	    sizeof(jn_phdr) + pkt->hdr.data_len)
		return -JE_WRITE;
#endif
	return 0;

}
