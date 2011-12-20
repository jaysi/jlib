#include "jnet.h"
#include "jnet_internals.h"
#include "debug.h"
#include "jer.h"

#include <string.h>
#include <assert.h>

uint16_t _jn_conn_req_dec(jn_conn* conn){

	uint16_t n;

#ifndef JN_SINGLE_CLT
	_lock_mx(&((jn_poll_buck*)conn->poll_buck)->conn_a_mx);
#endif

	if(((jn_poll_buck*)conn->poll_buck)->nreq[conn->poll_pos] > 0){
		((jn_poll_buck*)conn->poll_buck)->nreq[conn->poll_pos]--;
	}
	n = ((jn_poll_buck*)conn->poll_buck)->nreq[conn->poll_pos];

#ifndef JN_SINGLE_CLT
	_unlock_mx(&((jn_poll_buck*)conn->poll_buck)->conn_a_mx);
#endif
	
	return n;
}

uint16_t _jn_conn_req_inc(jn_conn* conn){

	uint16_t n;	


#ifndef JN_SINGLE_CLT
	_lock_mx(&((jn_poll_buck*)conn->poll_buck)->conn_a_mx);
#endif

	if(((jn_poll_buck*)conn->poll_buck)->nreq[conn->poll_pos] < ((jn_poll_buck*)conn->poll_buck)->maxconnreq){
		((jn_poll_buck*)conn->poll_buck)->nreq[conn->poll_pos]++;
	}
		
	n = ((jn_poll_buck*)conn->poll_buck)->nreq[conn->poll_pos];

#ifndef JN_SINGLE_CLT
	_unlock_mx(&((jn_poll_buck*)conn->poll_buck)->conn_a_mx);
#endif

	return n;
}

int _jn_conn_req_chkinc(jn_conn* conn){


#ifndef JN_SINGLE_CLT
	_lock_mx(&((jn_poll_buck*)conn->poll_buck)->conn_a_mx);
#endif


	if(((jn_poll_buck*)conn->poll_buck)->nreq[conn->poll_pos] < ((jn_poll_buck*)conn->poll_buck)->maxconnreq){
		((jn_poll_buck*)conn->poll_buck)->nreq[conn->poll_pos]++;

#ifndef JN_SINGLE_CLT
		_unlock_mx(&((jn_poll_buck*)conn->poll_buck)->conn_a_mx);
#endif

		return 0;
	} else {

#ifndef JN_SINGLE_CLT		
		_unlock_mx(&((jn_poll_buck*)conn->poll_buck)->conn_a_mx);
#endif

		return -JE_LOWRES;
	}		
}

uint16_t _jn_conn_req_chk(jn_conn* conn){
	
	uint16_t n;

#ifndef JN_SINGLE_CLT
	_lock_mx(&((jn_poll_buck*)conn->poll_buck)->conn_a_mx);
#endif


	n = ((jn_poll_buck*)conn->poll_buck)->nreq[conn->poll_pos];

#ifndef JN_SINGLE_CLT
	_unlock_mx(&((jn_poll_buck*)conn->poll_buck)->conn_a_mx);
#endif

	return n;
}

