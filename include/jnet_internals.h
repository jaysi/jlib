#ifndef _JN_INTERNALS_H
#define _JN_INTERNALS_H

#include "jnet.h"
#include "ecc.h"

#ifdef __cplusplus
extern "C" {
#endif

	int _jn_clt_handshake(jn_conn * conn);
	int _jn_clt_login(jn_conn * conn, wchar_t * username, wchar_t * pass,
			  uchar flags);
	int _jn_handshake(jn_h * h, jn_conn * conn);
	int _jn_login(jn_h * h, jn_conn * conn);
	int _jn_assert_pkt(jn_pkt * pkt, int chkdata);
	int _jn_read_pipe(int fd, jn_ppkt * pkt);
	int _jn_write_pipe(int fd, jn_ppkt * pkt);
	int recv_pkt_all(int fd, uchar * buf, uint32_t * len,
			 struct timeval *tv, aes_context * aes, jn_pkt * pkt);
	int _jn_is_fixed_data(jn_hdr hdr);
	int _jn_assert_pkt(jn_pkt * pkt, int chkdata);
	int send_all(int fd, uchar * buf, uint32_t * len, struct timeval *tv);
	int recv_all(int fd, uchar * buf, uint32_t * len, struct timeval *tv);
	int _jn_hdr_crc(jn_pkt * pkt);
	int _jn_data_crc(jn_pkt * pkt);
	int _jn_prepare_hdr(jn_hdr * hdr);

	int _jn_conn_proc_buf(jn_conn * conn, jn_pkt_list ** list,
			      uint16_t * npkt, uchar * buf, uint16_t bufsize);

	jmodid_t _jn_get_free_com_id(jn_h * h);
	int _jn_mod_register(jn_h * h, wchar_t * name, jobjid_t * objid);
	int _jn_mod_create_list(jn_h * h);
	jn_mod_entry *_jn_find_mod(jn_h * h, wchar_t * name);
	int _jn_add_com(jn_h * h, wchar_t * copyname);
	int _jn_create_com(jn_h * h, jn_conn * conn, jn_pkt * req_pkt);
	int _jn_destroy_com(jn_h * h, jn_conn * conn, jmodid_t com_id);
	void *_jn_comod_thread(void *arg);
	void *_jn_comod_rthread(void *arg);
	int _jn_com_load(jn_h * h, jobjid_t objid, wchar_t * copyname,
			 jn_comod_entry ** ptr);
	int _jn_join_com(jn_h * h, jn_conn * conn, jn_pkt * pkt);
	int _jn_leave_com(jn_h * h, jn_conn * conn, jn_pkt * pkt);
	int _jn_close_com(jn_h * h, jn_conn * conn, jn_pkt * pkt);
	uint32_t _jn_com_req_dec(jn_comod_entry * com);
	uint32_t _jn_com_req_inc(jn_comod_entry * com);

	jn_poll_buck *_jn_alloc_poll_buck(jn_h * h);
	int _jn_add_poll_fd(jn_h * h, jn_poll_buck * buck, jn_conn * conn);
	jn_conn *_jn_rm_poll_fd(jn_h * h, jn_poll_buck * buck, int fd);
	void *_jn_poll_thread(void *arg);

	int _jn_read_pipe(int fd, jn_ppkt * pkt);
	int _jn_write_pipe(int fd, jn_ppkt * pkt);

	void *_jn_clt_callback_thread(void *arg);
	void *_jn_pkt_queue_thread(void *arg);

	void _jn_cb_fifo_enq(jn_cb_fifo * fifo, jn_cb_entry * entry);
	jn_cb_entry *_jn_cb_fifo_wait(jn_cb_fifo * fifo);
	jn_cb_entry *_jn_cb_fifo_search_flags(jn_cb_fifo * fifo, uchar flags);
	jn_cb_entry *_jn_cb_fifo_search(jn_cb_fifo * fifo, jn_pkt * pkt);
	void _jn_cb_fifo_destroy(jn_cb_fifo * fifo);
	jn_cb_fifo *_jn_cb_fifo_new();

	void _jn_pack_hdr(uchar * buf, jn_pkt * pkt);
	void _jn_unpack_hdr(uchar * buf, jn_pkt * pkt);

	jn_fifo *_jn_fifo_new();
	void _jn_fifo_destroy(jn_fifo * fifo);
	void _jn_fifo_enq(jn_fifo * fifo, jn_fifo_entry * entry);
	jn_fifo_entry *_jn_fifo_wait(jn_fifo * fifo);

	ssize_t _jn_prepare_pkt(jn_conn * conn, jn_pkt * pkt, uchar * buf);

	uint16_t _jn_conn_req_chk(jn_conn * conn);
	uint16_t _jn_conn_req_dec(jn_conn * conn);
	uint16_t _jn_conn_req_inc(jn_conn * conn);
	int _jn_conn_req_chkinc(jn_conn * conn);
	jn_conn_list *_jn_conn_list_new();
	void _jn_conn_destroy(jn_conn * conn);
	jn_conn *_jn_conn_new();
	int _jn_conn_init(jn_conf * conf, jn_conn * conn);
	void _jn_conn_list_destroy(jn_conn_list * list);
	int _jn_conn_find(jn_conn_list * list, jn_conn * compare,
			  jn_conn ** entry, jn_conn ** prev, uint16_t pattern);

//assm/disassm helpers
//com-open/close
	void _jn_assm_proto1_pkt(jn_pkt * pkt, jmodid_t modid, uchar type);
	int _jn_assm_com_open(jn_pkt * pkt, uchar flags, jobjid_t objid,
			      wchar_t * name);
	int _jn_disassm_com_open(jn_pkt * pkt, uchar * flags, jobjid_t * objid,
				 wchar_t ** name);
	int assm_com_join(jn_pkt * pkt, uchar flags, jmodid_t mod_id);
	void disassm_com_join(jn_pkt * pkt, uchar * flags, jmodid_t * mod_id);
	int disassm_plain(uchar * buf, wchar_t * msg, uint16_t * msg_len);
	void assm_com_leave(jn_pkt * pkt, jmodid_t mod_id);
	void assm_com_close(jn_pkt * pkt, jmodid_t mod_id);
	void assm_com_pause(jn_pkt * pkt, jmodid_t mod_id);
	void assm_com_resume(jn_pkt * pkt, jmodid_t mod_id);

	jn_trans_a *_jn_transa_new(jtransid_t total);
	void _jn_transa_chg(jn_trans_a * trans_a, jtransid_t total);
	void _jn_transa_destroy(jn_trans_a * trans_a);
	jtransid_t _jn_transa_get(jn_trans_a * trans_a);
	void _jn_transa_rm(jn_trans_a * trans_a, jtransid_t tid);

	void _jn_assm_hdr(uchar * buf, uchar * data, uchar flags, uchar type,
			  uint16_t datalen, uint16_t comod_id, uchar trans_id,
			  uint32_t rs);
	int _jn_disassm_hdr(jn_hdr * hdr, uchar * buf);

	int _jn_request_send(jn_poll_buck * buck, int fd, uchar * buf,
			     uint32_t bufsize, uchar flags);
//triplet functions used for sending bulk requests
	int _jn_begin_send_requests(jn_poll_buck * buck);
	int _jn_add_send_request(jn_poll_buck * buck, int fd, uchar * buf,
				 uint32_t bufsize, uchar flags);
	void _jn_finish_send_requests(jn_poll_buck * buck);

	void *_jn_accept_thread(void *arg);

	int jnpoll(jn_poll_buck * buck);

	typedef struct {
		jn_h *h;
		jn_conn *conn;
		jn_comod_entry *com;
		int fd;
		jn_poll_buck *buck;
		jacid_t uid;
	} th_arg;

	typedef struct {
		jn_h *h;
		jn_poll_buck *buck;
	} buck_arg;

	typedef struct jbufferlist {
		int fd;
		uint32_t bufsize;
		uchar *buf;
		struct jbufferlist *next;
	} buflist;

	typedef struct pollbuck_list {
		uint32_t nfd;
		jn_poll_buck *buck;
		buflist *bfirst, *blast;
		struct pollbuck_list *next;
	} jn_pb_list;

#ifdef __cplusplus
}
#endif
/*
#define _jn_assm_proto1_pkt(pkt, modid, type) 	(pkt)->hdr.flags &= ~JNF_PLAIN;\
						(pkt)->hdr.type = type; \
						(pkt)->hdr.comod_id = modid; \
						(pkt)->hdr.data_len = 0L; \
						(pkt)->hdr.rsize = 0L;
*/
#define _jn_pack_hdr(buf, pkt) pack(buf, "hcchhclhc", (pkt)->hdr.magic, \
				 (pkt)->hdr.ver | (pkt)->hdr.flags, \
				(pkt)->hdr.type, (pkt)->hdr.data_len, \
				(pkt)->hdr.comod_id, (pkt)->hdr.trans_id, \
				(pkt)->hdr.rs, (pkt)->hdr.data_crc, \
				(pkt)->hdr.hdr_crc);
//this have to use hdr, see jn_base.c
#define _jn_unpack_hdr(buf, pkt) (pkt)->hdr.magic = unpacki16(buf);\
				(pkt)->hdr.ver = buf[sizeof(uint16_t)]<<4; (pkt)->hdr.flags = buf[sizeof(uint16_t)];\
				unpack(buf+sizeof(uint16_t)+sizeof(uchar), "chhclhc", \
				&((pkt)->hdr.type), &((pkt)->hdr.data_len), \
				&((pkt)->hdr.comod_id), &((pkt)->hdr.trans_id), \
				&((pkt)->hdr.rs), &((pkt)->hdr.data_crc), \
				&((pkt)->hdr.hdr_crc));
#define enc_pkt_size(data_len) (((data_len+sizeof(jn_hdr))%16)?((((data_len+sizeof(jn_hdr))/16)*16)+16):(data_len+sizeof(jn_hdr)))
#define enc_data_size(data_len) ((data_len%16)?(((data_len/16)*16)+16):data_len)
#define pmalloc(size) malloc(enc_data_size(size))
#define _tv2milli(tv) ((tv.tv_sec*1000) + (tv.tv_usec/1000))
#define _milli2tv(to, tv) \
	((tv)->tv_sec = to/1000); \
	((tv)->tv_usec = (to%1000)*1000);
// Set the socket I/O mode: In this case FIONBIO// enables or disables the blocking mode for the // socket based on the numerical value of iMode.// If iMode = 0, blocking is enabled; // If iMode != 0, non-blocking mode is enabled.//u_long iMode = 0;//ioctlsocket(m_socket, FIONBIO, &iMode);
#ifndef WIN32
#define _jn_block_mode(sockfd, modeptr)	fcntl(sockfd, F_SETFL, *(modeptr)==JN_MODE_NONBLOCK?O_NONBLOCK:0)
#else
#define _jn_block_mode(sockfd, modeptr)	socket_fcntl(sockfd, FIONBIO, modeptr)
#endif
#ifdef USE_JNPOLL
#define POLLIN	(0x01<<0)
#define POLLOUT	(0x01<<1)
typedef size_t nfds_t;

#endif

#endif
