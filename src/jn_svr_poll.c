#include "jnet.h"
#include "jer.h"
#include "ecc.h"
#include "jac.h"
#include "debug.h"
#include "jhash.h"
#include "jcs.h"
#include "jnet_internals.h"

#ifndef _WIN32
#include "aio.h"		//link with -lrt
#endif

#ifndef __WIN32
#include "unistd.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/in.h"
#else
#include "winsock.h"
#endif
#include "sys/types.h"
#include "sys/stat.h"
#include "string.h"
#include "wchar.h"
#include "stdio.h"
#include "stdlib.h"
#include "signal.h"
#include "jrand.h"
#include "sys/wait.h"
#include "errno.h"

#define _deb1(f, a...)
#define _wdeb1 _wdeb
#define _deb2(f, a...)
#define _wdeb2 _wdeb
#define _wdeb3 _wdeb		//trio, submit funcions

#ifdef USE_JNPOLL

int jnpoll(jn_poll_buck * buck)
{
	nfds_t i;
	int ret;
	struct timeval tv;

	if (!buck->timeout) {
		if (select(buck->max_fd + 1, &buck->readfds,
			   &buck->writefds, &buck->exceptfds, NULL) == -1) {

			return -JE_SYSE;
		}
	} else {
		_mill2tv(buck->timeout, &tv);
		if (select(buck->max_fd + 1, &buck->readfds,
			   &buck->writefds, &buck->exceptfds, &tv) == -1) {

			return -JE_SYSE;
		}
	}

	ret = 0;
	_lock_mx(&buck->conn_a_mx);
	for (i = 0; i < buck->nfds; i++) {
		if (buck->fds[i].events & POLLIN) {
			if (FD_ISSET(buck->fds[i].fd, &buck->readfds)) {
				ret++;
				buck->fds[i].revents = POLLIN;
			}
		}

		if (buck->fds[i].events & POLLOUT) {
			if (FD_ISSET(buck->fds[i].fd, &buck->writefds)) {
				ret++;
				buck->fds[i].revents |= POLLOUT;
			}
		}

	}
	_unlock_mx(&buck->conn_a_mx);

	return ret;

}

#else

#define jnpoll(buck) poll((buck)->fds, (buck)->nfds, (buck)->timeout)

#endif				//USE_JNPOLL

jn_poll_buck *_jn_alloc_poll_buck(jn_h * h)
{

	int pipefd[2];
	int cnt;
	jn_poll_buck *buck;

#ifndef JN_SINGLE_CLT
#ifndef NDEBUG
	jmxattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif
#endif

	if (pipe(pipefd))
		return NULL;

	_wdeb2(L"allocating bucket");

	buck = (jn_poll_buck *) malloc(sizeof(jn_poll_buck));
	if (!buck) {
		return NULL;
	}
	buck->next = NULL;

	buck->fds =
	    (struct pollfd *)malloc(sizeof(struct pollfd) * h->conf.nfds_buck);
	if (!buck->fds) {
		close(pipefd[0]);
		close(pipefd[1]);
		free(buck);
		return NULL;
	}

	buck->wr_a =
	    (jn_wr_array *) malloc(sizeof(jn_wr_array) * h->conf.poll_wr_req);
	if (buck->wr_a) {
		buck->max_wr_req = h->conf.poll_wr_req;	//see _jn_request_send()
		buck->nwr_req = 0UL;
		for (cnt = 0; cnt < h->conf.poll_wr_req; cnt++) {
			buck->wr_a[cnt].fd = -1;
		}
	} else {
		close(pipefd[0]);
		close(pipefd[1]);
		free(buck->fds);
		free(buck);
		return NULL;

	}

	buck->nwr_req =
	    (uint16_t *) malloc(sizeof(uint16_t) * h->conf.nfds_buck);
	if (!buck->nwr_req) {
		close(pipefd[0]);
		close(pipefd[1]);
		free(buck->fds);
		free(buck->wr_a);
		free(buck);
		return NULL;

	}

	buck->nreq = (uint16_t *) malloc(sizeof(uint16_t) * h->conf.nfds_buck);
	if (!buck->nreq) {
		close(pipefd[0]);
		close(pipefd[1]);
		free(buck->fds);
		free(buck->wr_a);
		free(buck->nwr_req);
		free(buck);
		return NULL;
	}

	buck->conn_a = (jn_conn *) malloc(sizeof(jn_conn) * h->conf.nfds_buck);
	if (buck->conn_a) {
		for (cnt = 0; cnt < h->conf.nfds_buck; cnt++) {
			buck->conn_a[cnt].uid = JACID_INVAL;
			buck->nreq[cnt] = 0UL;
		}
	} else {
		free(buck->wr_a);
		close(pipefd[0]);
		close(pipefd[1]);
		free(buck->fds);
		free(buck->nwr_req);
		free(buck->nreq);
		free(buck);
		return NULL;
	}

	buck->fds[0].fd = pipefd[0];
	buck->fds[0].events = POLLIN;
	buck->fds[1].fd = pipefd[1];
	buck->fds[1].events = 0;
	buck->nfds = 2;
	buck->timeout = 0;
	buck->fdop_flag = 0;
#ifndef NDEBUG
	_init_mx(&buck->conn_a_mx, &attr);
#else
	_init_mx(&buck->conn_a_mx, NULL);
#endif
	_init_cond(&buck->fdop_cond, NULL);

	buck->fifo = _jn_fifo_new();
	if (!buck->fifo) {
		_destroy_cond(&buck->fdop_cond);
		free(buck->wr_a);
		close(pipefd[0]);
		close(pipefd[1]);
		free(buck->fds);
		free(buck->nwr_req);
		free(buck->nreq);
		free(buck->conn_a);
		free(buck);
		return NULL;

	}

	buck->maxconnreq = h->conf.maxconnreq;

#ifdef USE_JNPOLL
	FD_ZERO(&buck->readfds);
	FD_ZERO(&buck->writefds);
	FD_ZERO(&buck->exceptfds);
	buck->max_fd = -1;
#endif

	_lock_mx(&h->poll_list->mx);
	if (!h->poll_list->first) {
		_wdeb2(L"added as first");
		h->poll_list->first = buck;
		h->poll_list->last = buck;
		h->poll_list->cnt = 1L;
	} else {
		_wdeb2(L"added as last");
		h->poll_list->last->next = buck;
		h->poll_list->last = h->poll_list->last->next;
		h->poll_list->cnt++;
	}

	_unlock_mx(&h->poll_list->mx);

	_wdeb1(L"finished");

	return buck;
}

void _jn_free_buck(void *arg)
{
	buck_arg *ba = (buck_arg *) arg;
	jn_poll_buck *entry, *prev;
	nfds_t n;

	_wdeb1(L"called");

	_lock_mx(&ba->h->poll_list->mx);

	entry = ba->h->poll_list->first;
	prev = entry;
	while (entry) {
		if (entry->fds[0].fd == ba->buck->fds[0].fd) {

			if (entry->fds[0].fd ==
			    ba->h->poll_list->first->fds[0].fd) {
				_deb1("first");
				ba->h->poll_list->first =
				    ba->h->poll_list->first->next;

				if (ba->h->poll_list->first->fds[0].fd ==
				    ba->h->poll_list->last->fds[0].fd) {
					_deb1("first = last");
					ba->h->poll_list->last = NULL;
				}

			} else {
				prev->next = entry->next;

				if (entry->fds[0].fd ==
				    ba->h->poll_list->last->fds[0].fd) {
					_deb1("last");
					ba->h->poll_list->last = prev;
				}
			}

			//close pipes;
#ifndef _WIN32
			close(entry->fds[0].fd);
			close(entry->fds[1].fd);
#else
			closesocket(entry->fds[0].fd);
			closesocket(entry->fds[1].fd);
#endif
			//close socket fds
			for (n = 0; n < ba->h->conf.nfds_buck; n++) {
				if (entry->conn_a[n].uid != JACID_INVAL) {
					jn_close(ba->h, &entry->conn_a[n], 0);
				}
			}

			_destroy_cond(&entry->fdop_cond);

			_jn_fifo_destroy(entry->fifo);

			_cancel_thread(entry->qth);

			for (n = 0; n < entry->max_wr_req; n++) {
				if (entry->wr_a[n].fd > 0) {
					if (entry->
					    wr_a[n].flags & JN_WR_FREE_BUF) {
						free(entry->wr_a[n].buf);
					}
				}
			}

			free(entry->wr_a);
			free(entry->conn_a);
			free(entry->fds);
			free(entry->nwr_req);
			free(entry);

			goto end;
		} else {
			prev = entry;
			entry = entry->next;
		}
	}
 end:
	_unlock_mx(&ba->h->poll_list->mx);
}

int _jn_add_poll_fd(jn_h * h, jn_poll_buck * buck, jn_conn * conn)
{
	//0. it does not lock mx
	//1. send an empty msg via pipe
	//2. add to end
	//3. update nfd

	jn_ppkt ppkt;
	uint16_t cnt;
	ppkt.hdr.type = JN_PT_FD_OP;
	ppkt.hdr.src_type = JN_PS_SVR;
	ppkt.hdr.src_id = 0;
	ppkt.hdr.data_len = 0;

	_wdeb2(L"called, adding fd %i to slot %u", conn->sockfd, buck->nfds);

	if (buck->nfds >= JN_CONF_NFDS_BUCK)
		return -JE_FUL;

	//_jn_conn_init(NULL, conn);

	_lock_mx(&buck->conn_a_mx);

	_jn_write_pipe(buck->fds[1].fd, &ppkt);

#ifdef USE_JNPOLL
	FD_SET(conn->sockfd, &buck->readfds);
	if (conn->sockfd > buck->max_fd)
		buck->max_fd = conn->sockfd;
#endif

	buck->fds[buck->nfds].fd = conn->sockfd;
	buck->fds[buck->nfds].events = POLLIN;
	buck->nfds++;
	buck->nwr_req[buck->nfds] = 0;

	conn->poll_buck = (void *)buck;
	for (cnt = 0; cnt < h->conf.nfds_buck; cnt++) {
		if (buck->conn_a[cnt].uid == JACID_INVAL) {
			_wdeb2
			    (L"found empty slot at %u, adding < %S >, fd = %i",
			     cnt, conn->username, conn->sockfd);
			_jn_conn_init(&h->conf, &buck->conn_a[cnt]);
			buck->conn_a[cnt].flags = JN_CF_SVR;
			memcpy(&buck->conn_a[cnt].aes, &conn->aes,
			       sizeof(aes_context));
			memcpy(buck->conn_a[cnt].key, conn->key,
			       JN_CONF_KEY_LEN);
			buck->conn_a[cnt].username = conn->username;
			buck->conn_a[cnt].uid = conn->uid;
			buck->conn_a[cnt].sockfd = conn->sockfd;
			buck->conn_a[cnt].poll_pos = cnt;
			buck->conn_a[cnt].poll_buck = (void *)buck;
			buck->nreq[cnt] = 1;	//for login
			memcpy(&buck->conn_a[cnt].addr, &conn->addr,
			       sizeof(struct sockaddr));
			((jn_poll_buck *) conn->poll_buck)->
			    nreq[conn->poll_pos] = 0;
			break;
		}
	}

	buck->fdop_flag = 1;
	_signal_cond(&buck->fdop_cond);
	_unlock_mx(&buck->conn_a_mx);

	_wdeb2(L"returning");

	return 0;

}

//used for a single send request
int _jn_request_send(jn_poll_buck * buck, int fd, uchar * buf, uint32_t bufsize,
		     uchar flags)
{

	//0. it does not lock mx
	//1. send an empty msg via pipe
	//2. add to end
	//3. update nfds

	jn_ppkt ppkt;
	uint32_t cnt;

	//if(buck->nwr_req >= buck->max_wr_req) return -JE_FUL;

	ppkt.hdr.type = JN_PT_FD_OP;
	ppkt.hdr.src_type = JN_PS_SVR;
	ppkt.hdr.src_id = 0;
	ppkt.hdr.data_len = 0;

	_lock_mx(&buck->conn_a_mx);

	_jn_write_pipe(buck->fds[1].fd, &ppkt);

	for (cnt = 0; cnt < buck->max_wr_req; cnt++) {
		if (buck->wr_a[cnt].fd < 0) {
			buck->wr_a[cnt].fd = fd;
			buck->wr_a[cnt].bufsize = bufsize;
			buck->wr_a[cnt].pos = 0UL;

			if (flags & JN_SEND_FREE) {
				buck->wr_a[cnt].flags = JN_WR_FREE_BUF;
			}

			if (flags & JN_SEND_COPY) {
				buck->wr_a[cnt].flags = JN_WR_FREE_BUF;
				buck->wr_a[cnt].buf = (uchar *) malloc(bufsize);
				if (!buck->wr_a[cnt].buf)
					return -JE_MALOC;
				memcpy(buck->wr_a[cnt].buf, buf, bufsize);
			} else {
				buck->wr_a[cnt].flags = 0x00;
				buck->wr_a[cnt].buf = buf;
			}

			break;
		}
	}

	if (cnt == buck->max_wr_req) {	//means the buffer is full
		if (flags & JN_SEND_FREE || JN_SEND_COPY) {
			free(buf);
			return -JE_FUL;
		}
	}
	//find the connection index

	for (cnt = 0; cnt < buck->nfds; cnt++) {
		if (buck->fds[cnt].fd == fd) {

			buck->fds[cnt].events |= POLLOUT;
			buck->nwr_req[cnt]++;
#ifdef USE_JNPOLL
			FD_SET(fd, &buck->writefds);
#endif
			break;
		}
	}

	buck->fdop_flag = 1;
	_signal_cond(&buck->fdop_cond);
	_unlock_mx(&buck->conn_a_mx);

	_wdeb2(L"submitted request");

	return 0;
}

//triplet functions used for sending bulk requests
int _jn_begin_send_requests(jn_poll_buck * buck)
{
	jn_ppkt ppkt;
	uint32_t cnt;

	//if(buck->nwr_req >= buck->max_wr_req) return -JE_FUL;

	_wdeb3(L"called");

	ppkt.hdr.type = JN_PT_FD_OP;
	ppkt.hdr.src_type = JN_PS_SVR;
	ppkt.hdr.src_id = 0;
	ppkt.hdr.data_len = 0;

	_lock_mx(&buck->conn_a_mx);

	_jn_write_pipe(buck->fds[1].fd, &ppkt);

	_wdeb3(L"returning");

	return 0;
}

int _jn_add_send_request(jn_poll_buck * buck, int fd, uchar * buf,
			 uint32_t bufsize, uchar flags)
{

	nfds_t cnt;

	_wdeb3(L"adding send request for fd %i, bufsize = %u, flags = %x", fd,
	       bufsize, flags);

	for (cnt = 0; cnt < buck->max_wr_req; cnt++) {
		if (buck->wr_a[cnt].fd < 0) {
			buck->wr_a[cnt].fd = fd;
			buck->wr_a[cnt].bufsize = bufsize;
			buck->wr_a[cnt].pos = 0UL;

			if (flags & JN_SEND_FREE) {
				buck->wr_a[cnt].flags = JN_WR_FREE_BUF;
			}

			if (flags & JN_SEND_COPY) {
				buck->wr_a[cnt].flags = JN_WR_FREE_BUF;
				buck->wr_a[cnt].buf = (uchar *) malloc(bufsize);
				if (!buck->wr_a[cnt].buf)
					return -JE_MALOC;
				memcpy(buck->wr_a[cnt].buf, buf, bufsize);
			} else {
				buck->wr_a[cnt].flags = 0x00;
				buck->wr_a[cnt].buf = buf;
			}

			break;
		}
	}

	if (cnt == buck->max_wr_req) {	//means the buffer is full
		if (flags & JN_SEND_FREE || JN_SEND_COPY) {
			_wdeb(L"FULL");
			free(buf);
			return -JE_FUL;
		}
	}
	//find the connection index

	for (cnt = 0; cnt < buck->nfds; cnt++) {
		if (buck->fds[cnt].fd == fd) {
			buck->fds[cnt].events |= POLLOUT;
			buck->nwr_req[cnt]++;
#ifdef USE_JNPOLL
			FD_SET(fd, &buck->writefds);
#endif
			break;
		}
	}

	_wdeb3(L"returning");

	return 0;

}

void _jn_finish_send_requests(jn_poll_buck * buck)
{

	_wdeb3(L"called");

	buck->fdop_flag = 1;
	_signal_cond(&buck->fdop_cond);
	_unlock_mx(&buck->conn_a_mx);

	_wdeb3(L"submitted requests");
}

jn_conn *_jn_rm_poll_fd(jn_h * h, jn_poll_buck * buck, int fd)
{
	//0. it does not lock mx
	//1. send an empty msg via pipe
	//2. replace with last fd
	//3. update nfds
	jn_ppkt ppkt;
	nfds_t i, cnt;
	jn_conn *ret = NULL;

	_wdeb1(L"called, removing fd %i", fd);

	ppkt.hdr.type = JN_PT_FD_OP;
	ppkt.hdr.src_type = JN_PS_SVR;
	ppkt.hdr.src_id = 0;
	ppkt.hdr.data_len = 0;

	_lock_mx(&buck->conn_a_mx);

	_jn_write_pipe(buck->fds[1].fd, &ppkt);

#ifdef USE_JNPOLL
	FD_CLR(fd, &buck->readfds);
#endif

	for (i = 2; i < buck->nfds; i++) {
		if (buck->fds[i].fd == fd) {
			_wdeb2(L"replacing fd %i from slot %u", fd, i);
			if (i != buck->nfds - 1) {	//not last entry
				buck->fds[i].fd = buck->fds[buck->nfds - 1].fd;
				buck->fds[i].events =
				    buck->fds[buck->nfds - 1].events;
				buck->fds[i].revents =
				    buck->fds[buck->nfds - 1].revents;
			}
			buck->nfds--;

			for (cnt = 0; cnt < h->conf.nfds_buck; cnt++) {
				if (buck->conn_a[cnt].sockfd == fd) {
					ret = &buck->conn_a[cnt];
					//buck->conn_a[cnt].uid = JACID_INVAL;
					//don't invalidate uid here
					break;
				}
			}
#ifndef NDEBUG
			if (cnt == h->conf.nfds_buck)
				_wdeb(L"WARNING HERE");
#endif

			break;
		}
	}

	_wdeb(L"nwr_req[%i] = %u", i, buck->nwr_req[i]);
	if (buck->nwr_req[i]) {
		for (cnt = 0; cnt < buck->max_wr_req; cnt++) {
			if (buck->wr_a[cnt].fd == fd) {
				_wdeb(L"found wr_a[%u] match", cnt);
				if (buck->wr_a[cnt].flags & JN_WR_FREE_BUF) {
					free(buck->wr_a[cnt].buf);
				}
				buck->wr_a[cnt].fd = -1;
				buck->nwr_req[i]--;
				if (!buck->nwr_req[i])
					break;
			}
		}
	}
#ifndef NDEBUG
	_wdeb(L"nwr_req[%i] = %u ( = 0 ? )", i, buck->nwr_req[i]);
#endif

#ifdef USE_JNPOLL
	FD_CLR(fd, &buck->writefds);
#endif

	buck->fdop_flag = 1;
	_signal_cond(&buck->fdop_cond);
	_unlock_mx(&buck->conn_a_mx);

	return ret;
}

int _jn_poll_handle_read(jn_h * h, jn_poll_buck * buck, int n)
{
	uchar buf[JN_MAX_PKT];
	unsigned long imode;
	ssize_t got;
	nfds_t cnt2;
	jn_fifo_entry *job;
	int ret;

	_wdeb1(L"called for slot %i", n);

	imode = JN_MODE_NONBLOCK;
	_jn_block_mode(buck->fds[n].fd, &imode);

	got = recv(buck->fds[n].fd, buf, JN_MAX_PKT, 0);

	imode = JN_MODE_BLOCK;
	_jn_block_mode(buck->fds[n].fd, &imode);

	_lock_mx(&buck->conn_a_mx);
	for (cnt2 = 0; cnt2 < h->conf.nfds_buck; cnt2++) {
		if (buck->conn_a[cnt2].sockfd == buck->fds[n].fd) {
			_wdeb1(L"found connection entry %S @ conn_a[%u]",
			       buck->conn_a[cnt2].username, cnt2);
			_unlock_mx(&buck->conn_a_mx);
			//_jn_conn_req_inc(&buck->conn_a[cnt2]);
			//don't use, two locks on a mutex
			_jn_conn_req_inc(&buck->conn_a[cnt2]);

			if (got <= 0) {

				_jn_conn_req_dec(&buck->conn_a[cnt2]);

				if (errno == EINTR) {
					//do nothing; poll again
					return -JE_AGAIN;
				} else if (errno == EAGAIN) {
					//do nothing;
					return -JE_AGAIN;
				} else {

					//_jn_conn_req_dec(&buck->conn_a[cnt2]);//from login, not here in jn_close
					jn_close(h, &buck->conn_a[cnt2], 0);
					return -JE_DC;
				}
			} else {

				buck->conn_a[cnt2].bytes_rcvd += got;

				job = (jn_fifo_entry *)
				    malloc(sizeof(jn_fifo_entry));
				job->type = JN_FT_PKT;
				job->conn = &buck->conn_a[cnt2];

				/*
				   putting a lock
				   on
				   conn->prev_len
				   and allocating
				   all needed
				   memmory here
				   could be
				   great.
				 */

				job->ptr = malloc(got);
				job->datalen = got;
				memcpy(job->ptr, buf, got);

				_jn_fifo_enq(buck->fifo, job);

				return 0;

			}

		}

	}

	_unlock_mx(&buck->conn_a_mx);

	if (cnt2 == h->conf.nfds_buck) {
		_wdeb(L"WARNING: conn entry for fd %i not found",
		      buck->fds[n].fd);
		return -JE_NOTFOUND;
	}

	return -JE_UNK;
}

int _jn_poll_handle_write(jn_h * h, jn_poll_buck * buck, int n)
{
	nfds_t i = 0;
	ssize_t wr;
	unsigned long imode;

	for (i = 0; i < buck->max_wr_req; i++) {
		if (buck->fds[n].fd == buck->wr_a[i].fd) {

			break;
		}
	}
	assert(i < buck->max_wr_req);

	imode = JN_MODE_NONBLOCK;
	_jn_block_mode(buck->fds[n].fd, &imode);

	wr = send(buck->fds[n].fd,
		  buck->wr_a[i].buf + buck->wr_a[i].pos,
		  buck->wr_a[i].bufsize - buck->wr_a[i].pos, 0);

	imode = JN_MODE_NONBLOCK;
	_jn_block_mode(buck->fds[n].fd, &imode);

	if (wr <= 0) {

		if (errno == EINTR) {
			//do nothing; poll again
			return -JE_AGAIN;
		} else if (errno == EAGAIN) {
			//do nothing;
			return -JE_AGAIN;
		} else {

			return -JE_SYSE;
		}
	} else if (wr == (buck->wr_a[i].bufsize - buck->wr_a[i].pos)) {

		buck->wr_a[i].fd = -1;
		if (buck->wr_a[i].flags & JN_WR_FREE_BUF) {
			free(buck->wr_a[i].buf);
		}
		/*if there is no more
		   write requests on this
		   fd remove it from poll
		   fds */
		return 0;

	} else {
		buck->wr_a[i].pos += wr;
		return JE_CONTINUE;
	}

	return -JE_UNK;
}

void *_jn_poll_thread(void *arg)
{
	th_arg *ca = (th_arg *) arg;
	jn_h *h = ca->h;
	jn_poll_buck *buck = ca->buck;
	int n;
	jn_ppkt ppkt;
	buck_arg b;
	//jthread_t thid;
	jn_conn *conn;
	int cnt;
	ssize_t got;
	//size_t ebufsize;
	uchar buf[JN_MAX_PKT];
	//uchar prev[JN_MAX_PKT]; not needed using realloc
	//struct aiocb cb;
	unsigned long imode;
	uint16_t nq = 0;
	jn_fifo_entry *job;
	nfds_t cnt2;

	_wdeb2(L"thread started");

	b.h = h;
	b.buck = buck;
	_push_cleanup(_jn_free_buck, (void *)&b);

	buck->thid = _get_thread_id();

	if (_create_thread(&buck->qth, NULL, _jn_pkt_queue_thread, arg) < 0) {
		free(job);
		return NULL;
	}

	ppkt.hdr.type = JN_PT_FD_OP;
	ppkt.hdr.src_type = JN_PS_SVR;
	ppkt.hdr.src_id = 0;
	ppkt.hdr.data_len = 0;

	_jn_write_pipe(buck->fds[1].fd, &ppkt);

	for (;;) {

		if ((cnt = jnpoll(buck)) > 0) {
			if (buck->fds[0].revents & POLLIN) {	//read/proc pipe
				cnt--;
				buck->fds[0].revents &= ~POLLIN;
				_wdeb2(L"polled on pipe %i", buck->fds[0].fd);
				if (_jn_read_pipe(buck->fds[0].fd, &ppkt)) {
					_wdeb(L"pipe failed");
					return NULL;
				}
				switch (ppkt.hdr.type) {
				case JN_PT_WAKEUP:	//don't care
					break;

				case JN_PT_FD_OP:	//add/rm fd
					_lock_mx(&buck->conn_a_mx);

					while (!buck->fdop_flag) {
						_wait_cond(&buck->fdop_cond,
							   &buck->conn_a_mx);
					}
					//fd operation complete
					buck->fdop_flag = 0;
					_unlock_mx(&buck->conn_a_mx);
					break;

				case JN_PT_SHUTDOWN:
					_lock_mx(&buck->conn_a_mx);
					free(job);
					_unlock_mx(&buck->conn_a_mx);
					return NULL;
					break;

				default:
					_wdeb(L"unknown pipe_pkt_type 0x%x",
					      ppkt.hdr.type);
					break;
				}
			}

			n = 1;	//start from 2
			while (cnt > 0) {
				n++;
				if (buck->fds[n].revents & POLLIN) {
					_wdeb1(L"polled on %i slot %u",
					       buck->fds[n], n);
					cnt--;
					//_wdeb2(L"polled on %i", buck->fds[n].fd);                                     
					_jn_poll_handle_read(h, buck, n);

				}

				if (buck->fds[n].revents & POLLOUT) {
					cnt--;
					if (!_jn_poll_handle_write(h, buck, n)) {
						//reduce nwr_req
						//unset POLLOUT if nwr_req = 0
#ifndef NDEBUG
						if (buck->nwr_req[n] == 0) {
							_wdeb
							    (L"WARNING: nwr_req[%u] = 0",
							     n);
						}
#endif
						buck->nwr_req[n]--;
						if (!buck->nwr_req[n]) {
							buck->fds[n].events &=
							    ~POLLOUT;
#ifdef USE_JNPOLL
							FD_CLR(fd,
							       &buck->writefds);
#endif
						}
					}
				}	//POLLOUT
			}	//while
		}		//poll
	}			//for

	_pop_cleanup(1);

	return NULL;
}
