#include "jnet.h"
#include "jer.h"
#include "debug.h"

#define _wdeb1(f, a...)		//cb fifo
#define _wdeb2(f, a...)		//fifo
#define _wdeb3 _wdeb		//cb tracker
#define _jn_dump_cbfifo(fifo)
/*
#ifndef NDEBUG
void _jn_dump_cbfifo(jn_cb_fifo* fifo){
	jn_cb_entry* cb;
	uint32_t cnt = 0;
#ifndef JN_SINGLE_CLT
	_lock_mx(&fifo->mx);
#endif
	for(cb = fifo->first; cb; cb = cb->next){
		_wdeb(L"cb_entry[ %u ]: tid = %u, comid = %u", cnt, cb->tid, 
			cb->comid);
		cnt++;
	}
#ifndef JN_SINGLE_CLT
	_unlock_mx(&fifo->mx);
#endif
}
#else
#define _jn_dump_cbfifo(fifo)
#endif
*/

jn_fifo *_jn_fifo_new(jn_conf * conf)
{
	jn_fifo *fifo;
#ifndef JN_SINGLE_CLT
#ifndef NDEBUG
	jmxattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif
#endif

	fifo = (jn_fifo *) malloc(sizeof(jn_fifo));
	if (!fifo)
		return NULL;

#ifndef JN_SINGLE_CLT
#ifndef NDEBUG
	_init_mx(&fifo->mx, &attr);
#else
	_init_mx(&fifo->mx, NULL);
#endif
	_init_sem(&fifo->sem);
#endif

	fifo->cnt = 0L;
	fifo->first = NULL;
	fifo->flags = 0x00;
	/*
	   fifo->nq = 0UL;
	   fifo->qbufpoolpos = 0UL;
	   if(conf){
	   fifo->nmaxq = conf->nmaxfifoq;
	   fifo->qbufpoolsize = conf->fifoqbufsize;
	   } else {
	   fifo->nmaxq = JN_CONF_FIFO_Q_POOL_N;
	   fifo->qbufpoolsize = JN_CONF_FIFO_QBUF_POOL_SIZE;
	   }
	 */

	return fifo;
}

void _jn_fifo_destroy(jn_fifo * fifo)
{

	jn_fifo_entry *entry;

#ifndef JN_SINGLE_CLT
	_lock_mx(&fifo->mx);
#endif

	fifo->flags |= JN_FF_NO_NEW;

	entry = fifo->first;

	while (fifo->first) {
		fifo->first = fifo->first->next;
		free(entry);
		entry = fifo->first;
	}

#ifndef JN_SINGLE_CLT
	_destroy_sem(&fifo->sem);

	_unlock_mx(&fifo->mx);

	_destroy_mx(&fifo->mx);
#endif

	free(fifo);

}

void _jn_fifo_enq(jn_fifo * fifo, jn_fifo_entry * entry)
{

	entry->next = NULL;

#ifndef JN_SINGLE_CLT
	_lock_mx(&fifo->mx);
#endif

	if (fifo->flags & JN_FF_NO_NEW) {
#ifndef JN_SINGLE_CLT
		_unlock_mx(&fifo->mx);
#endif
		return;
	}

	/*
	   if(fifo->flags & JN_FF_USE_POOL){
	   if(fifo->nq == fifo->nmaxq){

	   fifo->nq = 0;
	   }

	   }

	   if(fifo->flags & JN_FF_USE_DATA_POOL){

	   }
	 */

	if (!fifo->first) {
		fifo->first = entry;
		fifo->last = entry;
		fifo->cnt = 1L;
	} else {
		fifo->last->next = entry;
		fifo->last = fifo->last->next;
		fifo->cnt++;
	}

#ifndef JN_SINGLE_CLT
	_post_sem(&fifo->sem);

	_unlock_mx(&fifo->mx);
#endif

}

jn_fifo_entry *_jn_fifo_wait(jn_fifo * fifo)
{

	jn_fifo_entry *entry;

#ifndef JN_SINGLE_CLT
	_wait_sem(&fifo->sem);
	_lock_mx(&fifo->mx);
#endif

	entry = fifo->first;
	fifo->first = fifo->first->next;

	fifo->cnt--;

#ifndef JN_SINGLE_CLT
	_unlock_mx(&fifo->mx);
#endif

	return entry;
}

jn_cb_fifo *_jn_cb_fifo_new()
{
	jn_cb_fifo *fifo;

#ifndef JN_SINGLE_CLT
#ifndef NDEBUG
	jmxattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif
#endif
	_wdeb1(L"allocating memmory");

	fifo = (jn_cb_fifo *) malloc(sizeof(jn_cb_fifo));
	if (!fifo)
		return NULL;

	_wdeb1(L"done, initing mx");

#ifndef JN_SINGLE_CLT
#ifndef NDEBUG
	_init_mx(&fifo->mx, &attr);
#else
	_init_mx(&fifo->mx, NULL);
#endif

	_init_sem(&fifo->sem);
#endif

	_wdeb1(L"done, setting values...");

	fifo->cnt = 0L;
	fifo->first = NULL;
	fifo->flags = 0x00;

	_wdeb1(L"done, returning...");

	return fifo;
}

void _jn_cb_fifo_destroy(jn_cb_fifo * fifo)
{

	jn_cb_entry *entry;

#ifndef JN_SINGLE_CLT
	_lock_mx(&fifo->mx);
#endif

	fifo->flags |= JN_FF_NO_NEW;

	entry = fifo->first;

	while (fifo->first) {
		fifo->first = fifo->first->next;
		free(entry);
		entry = fifo->first;
	}

#ifndef JN_SINGLE_CLT

	_destroy_sem(&fifo->sem);

	_unlock_mx(&fifo->mx);

	_destroy_mx(&fifo->mx);
#endif

	free(fifo);

}

void _jn_cb_fifo_enq(jn_cb_fifo * fifo, jn_cb_entry * entry)
{

	entry->next = NULL;

#ifndef JN_SINGLE_CLT
	_lock_mx(&fifo->mx);
#endif

	if (fifo->flags & JN_FF_NO_NEW) {
		_wdeb3(L"CB, JN_FF_NO_NEW flag set");
#ifndef JN_SINGLE_CLT
		_unlock_mx(&fifo->mx);
#endif
		return;
	}

	if (!fifo->first) {
		fifo->first = entry;
		fifo->last = entry;
		fifo->cnt = 1L;
	} else {
		fifo->last->next = entry;
		fifo->last = fifo->last->next;
		fifo->cnt++;
	}

	_wdeb3
	    (L"CB registered, tid = %u, comid = %u, flags = 0x%x,  pcb = 0x%p, pctx = 0x%p, icb = 0x%p, ictx = 0x%p",
	     entry->tid, entry->comid, entry->flags, entry->progress_callback,
	     entry->progress_context, entry->if_callback, entry->if_context);

#ifndef JN_SINGLE_CLT
	_post_sem(&fifo->sem);

	_unlock_mx(&fifo->mx);
#endif

}

jn_cb_entry *_jn_cb_fifo_wait(jn_cb_fifo * fifo)
{

	jn_cb_entry *entry;

#ifndef JN_SINGLE_CLT
	_wait_sem(&fifo->sem);
	_lock_mx(&fifo->mx);
#endif

	entry = fifo->first;
	fifo->first = fifo->first->next;
	fifo->cnt--;

#ifndef JN_SINGLE_CLT
	_unlock_mx(&fifo->mx);
#endif

	return entry;
}

jn_cb_entry *_jn_cb_fifo_search_flags(jn_cb_fifo * fifo, uchar flags)
{
	jn_cb_entry *entry;

	_wdeb3(L"CB searching for flag 0x%x", flags);

#ifndef JN_SINGLE_CLT
	_lock_mx(&fifo->mx);
#endif

	for (entry = fifo->first; entry; entry = entry->next) {
		if (entry->flags & flags) {
			_wdeb3(L"CB found flag @ tid = %u, comid = %u",
			       entry->tid, entry->comid);
			goto end;
		}
	}

 end:
#ifndef JN_SINGLE_CLT
	_unlock_mx(&fifo->mx);
#endif

	return entry;
}

jn_cb_entry *_jn_cb_fifo_search(jn_cb_fifo * fifo, jn_pkt * pkt)
{

	jn_cb_entry *entry, *prev;
	int found = 0;

	_jn_dump_cbfifo(fifo);

#ifndef JN_SINGLE_CLT
	_lock_mx(&fifo->mx);
#endif

	entry = fifo->first;
	prev = fifo->first;

	while (entry) {
		if ((entry->flags & JN_CB_TID) &&
		    (entry->tid == pkt->hdr.trans_id)) {
			_wdeb3(L"CB found TID match");
			found = 1;
			break;
		} else if ((entry->flags & JN_CB_COMID) &&
			   (entry->comid == pkt->hdr.comod_id)) {
			_wdeb3(L"CB found COMID match");
			found = 1;
			break;
		} else {
			prev = entry;
			entry = entry->next;
		}

	}

	if (found) {

		if (pkt->hdr.rs && !(pkt->hdr.flags & JNF_ERR)) {
			_wdeb3(L"CB rs = %u, setting CONTINUE flag",
			       pkt->hdr.rs);
			entry->flags |= JN_CB_CONTINUE;
		} else {
			entry->flags &= ~JN_CB_CONTINUE;
		}

		//the progress callback must decide about the
		//actual bytes got and remained
		//entry->nremain = pkt->hdr.rsize;
		//entry->nbytes += pkt->hdr.data_len;

		if ((entry->flags & JN_CB_KEEP)
		    || (entry->flags & JN_CB_CONTINUE)) {
			_wdeb3
			    (L"CB has KEEP or CONTINUE flags set, not removing entry");

		} else {
			if (entry == fifo->first) {
				_wdeb3(L"CB replacing FIRST entry");
				fifo->first = fifo->first->next;
			} else if (entry == fifo->last) {
				_wdeb3(L"CB replacing LAST entry with prev [");
				prev->next = NULL;
				fifo->last = prev;
			} else {
				_wdeb3(L"CB replacing MID entry");
				prev->next = entry->next;
			}
			fifo->cnt--;
			_wdeb3(L"CB cnt = %u", fifo->cnt);
		}
	}
#ifndef JN_SINGLE_CLT
	_unlock_mx(&fifo->mx);
#endif

	_wdeb1(L"returning");

	return entry;

}

int jn_run_cb(jn_conn * conn, jn_pkt * pkt)
{

	int ret = -JE_NOTFOUND;
	jn_cb_entry *cb;
	void (*if_callback) (jn_cb_entry *);

	cb = _jn_cb_fifo_search(conn->cbfifo, pkt);

	if (cb) {
		if (cb->flags & JN_CB_CONTINUE)
			ret = JE_CONTINUE;
		else
			ret = 0;

		if_callback = cb->if_callback;
		if_callback(cb);
		if ((!(cb->flags & JN_CB_KEEP)) &&
		    (!(cb->flags & JN_CB_CONTINUE)) &&
		    (!(cb->flags & JN_CB_NOFREE))
		    ) {
			free(cb);
		}

	}

	return ret;

}
