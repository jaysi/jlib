#include "jnet.h"
#include "jer.h"
#include "ecc.h"
#include "debug.h"
#include "jcs.h"
#include "jrand.h"
#include "jhash.h"
#include "hardio.h"

#include "jnet_internals.h"
    
#ifndef __WIN32
#include "unistd.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/in.h"
#include <netdb.h>
#else				/* 
 */
#include "winsock.h"
#define close(fd) close_socket(fd)
#endif				/* 
 */
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
#endif				/* 
 */
#include "sys/wait.h"
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
    
/* _deb1? definitions*/ 
#define _deb1(f, a...)
#define _wdeb1(f, a...)
#define _deb2(f, a...)
#define _wdeb2(f, a...)
#define _wdeb4 _wdeb		//send_io_completion_handler
/*
void _jn_send_aio_free_buf(int signo, siginfo_t* info, void* context){
	if(info->si_signo == SIGIO){		
		free(info->si_value.sival_ptr);
	}	
}

int jn_send_aio(jn_conn* conn, jn_pkt* pkt, uchar flags){

#ifdef linux

	uchar* buf = (uchar*)malloc(enc_pkt_size(pkt->hdr.data_len));
	if(!buf) return -JE_MALOC;
	//uchar buf[JN_MAX_PKT];
	ssize_t len;
	int cnt;
	int ret;
	struct aiocb cb;
	struct sigaction sig_act;	
	
	cnt = conn->sockfd;
	
	if((len = _jn_prepare_pkt(conn, pkt, buf)) < 0){
		free(buf);
		return len;
	}
	
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = SA_SIGINFO;
	sig_act.sa_sigaction = _jn_send_aio_free_buf;
	
		
	bzero((char*)&cb, sizeof(struct aiocb));
	
	cb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	cb.aio_sigevent.sigev_signo = SIGIO;
	//cb.aio_sigevent.sigev_notify_function = &_jn_send_aio_free_buf;
	cb.aio_sigevent.sigev_notify_attributes = NULL;
	cb.aio_sigevent.sigev_value.sival_ptr = buf;
	
		
	cb.aio_fildes = cnt;//already coppied
	cb.aio_buf = buf;
	cb.aio_nbytes = len;
	cb.aio_offset = 0L;
	
	ret = sigaction(SIGIO, &sig_act, NULL);

	cnt = 0;
write_again:
	cnt++;
	if((ret = aio_write(&cb))==-1){
		switch(errno){
			case EAGAIN:
				if(cnt >= JN_MAX_WRITE_RETRIES){
					free(buf);
					return -JE_LOWRES;
				}
				sleep(cnt);
				goto write_again;
			break;
			
			default:
				free(buf);
				return -JE_SYSE;
			break;//to beautify the code!
		}
	}
	
	return 0;
#else
	return -JE_IMPLEMENT;
#endif
}
*/ 

int jn_send_poll(jn_conn * conn, jn_pkt * pkt, uchar flags)
{
	ssize_t len;
	uchar * buf = (uchar *) malloc(enc_pkt_size(pkt->hdr.data_len));
	if (!buf)
		return -JE_MALOC;
	if ((len = _jn_prepare_pkt(conn, pkt, buf)) < 0) {		
		free(buf);		
		return len;
	}
	return _jn_request_send((jn_poll_buck *) conn->poll_buck,
				   
		conn->sockfd, buf, len, JN_SEND_FREE);

}


 
 
 
/*
int jn_send_poll_many(jn_conn** conn, jn_pkt** pkt, uint32_t n, uchar flags){
	//1. create poll buck list
	//2. use 

	uint32_t nbuck;
	void** pb;
	uint32_t cnt, pblast;
	uchar buf[JN_MAX_PKT];
	ssize_t bufsize;
	
	pb = malloc(n*sizeof(void*));
	if(!pb) return -JE_MALOC;
	pblast = 0UL;
	for(cnt = 0; cnt < n; cnt++){
		for(cnt2 = 0; cnt2 < pblast; cnt2++){
			if(conn[cnt].poll_buck == *pb[cnt2]) break;
		}
		if(cnt2 == pblast){
			*pb[pblast] = conn[cnt]->poll_buck;
			plbast++;
		}
	}

	for(cnt = 0; cnt < pblast; cnt++){
		if(_jn_start_send_requests((jn_poll_buck*)(*pb[cnt])) < 0){
			*pb[cnt] = NULL;
		} else {
			for(cnt = 0; cnt < n; cnt++){
				if((bufsize = _jn_prepare_pkt(conn[cnt],
							pkt[cnt], buf)) > 0){
					_jn_add_send_request(
						(jn_poll_buck*)(*pb[cnt]),
						
				}
			}
		
		}
	}	
}
*/ 
