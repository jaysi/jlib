#ifndef _JNET_H
#define _JNET_H

#define USE_AES
#undef USE_PACK
#undef NO_HEAP

#include "ecc.h"
#include "aes.h"
#include "jbits.h"
#include "jdb.h"
#include "jtypes.h"
#include "ring_buf.h"
#include "jpack.h"
#include "jconst.h"

#ifndef JN_SINGLE_CLT
#define _J_USE_PTHREAD
#include "jcompat.h"
#endif

//#include <linux/time.h>
#include <sys/time.h>
#ifndef _WIN32
#include <sys/wait.h>
#include <poll.h>
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif
#ifdef linux
#include <aio.h>
#endif

/*
 * My best guess at if you are big-endian or little-endian.  This may
 * need adjustment.
 */
#if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
     __BYTE_ORDER == __LITTLE_ENDIAN) || \
    (defined(i386) || defined(__i386__) || defined(__i486__) || \
     defined(__i586__) || defined(__i686__) || defined(vax) || defined(MIPSEL))
#define JN_LITTLE_ENDIAN 1
#define JN_BIG_ENDIAN 0
#elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
       __BYTE_ORDER == __BIG_ENDIAN) || \
      (defined(sparc) || defined(POWERPC) || defined(mc68000) || defined(sel))
#define JN_LITTLE_ENDIAN 0
#define JN_BIG_ENDIAN 1
#else
#define JN_LITTLE_ENDIAN 0
#define JN_BIG_ENDIAN 0
#endif

#define JN_VER	0x0
#define JN_SVR_OBJ_NAME			L"JNetSvr"

/*
	checked permissions:
	
		start/halt/stop server: o | x to the server
		server_user_list: o | x to the server
		login to server: being in the jac!
		
		upload/update mod: o | w to the server
		list mod: o | x | r to the server
		
		list comod: o | x | r to the server
		start/stop comod: o | x to the mod OR o | x to the server
		join comod: o | rw  to the mod OR o | x to the server		
		com_user_list: comod decides OR o | x to the server
		
		user_kick from comod:	o | x to the mod object or server
		user_ban/unban from server: o | x to the server
*/

#define JN_MAX_PKT			4096
#define JN_MAX_COM_NAME			256
#define JN_MAX_PLAIN_MSG		1024

#ifdef _WIN32
#define JN_WIN_PIPE_PORT	"50000"
#define JN_WIN_PIPE_HOSTNAME	"localhost"
#endif

//timeouts in milliseconds
#define JN_CONF_HANDSHAKE_TIMEOUT	5000L
#define JN_CONF_LOGIN_TIMEOUT		5000L
#define JN_CONF_PKT_TIMEOUT		10000L
#define JN_CONF_PORT			"49999"	//a miller?
#define JN_CONF_MOD_PATH		L"jnmod/"
#define JN_CONF_MAX_COM			1000
#define JN_CONF_MAX_UCOM		10
#define JN_CONF_MAX_MODS		1000
#define JN_CONF_MAX_USERS		1000
#define JN_CONF_LISTEN_BACKLOG		10
#define JN_CONF_CONN_BUF		JN_MAX_PKT
#define JN_CONF_MAX_CLT_SVR_TRANS_SIZE	1024L
#define JN_CONF_KEY_LEN			1024
#define JN_CONF_NFDS_BUCK		100
#define JN_CONF_MAX_PDATA		JN_MAX_PKT
#define JN_CONF_MAX_CONN_REQ		10
#define JN_CONF_MAX_COM_REQ		100
#define JN_CONF_MAX_TRANS		255
#define JN_CONF_CBQ_TIMEOUT		30	//callback timeout,
#define JN_CONF_CBQ_TRIGGER		255	//callback garbage collector's trigger
#define JN_CONF_POLL_Q			63
#define JN_CONF_SVR_Q			10
#define JN_CONF_POLL_WR_REQ		100
#define JN_MAX_LIO			100	//users per lio_list call
#define JN_MAX_WRITE_RETRIES		10
#define JN_MAX_MALOC_RETRIES		5
#define JN_CONN_CLOSE_WAIT_TIME		5
#define JN_COM_CLOSE_WAIT_TIME		5
#define JN_CONF_FIFO_Q_POOL_N		10
#define JN_CONF_FIFO_QBUF_POOL_SIZE	JN_MAX_PKT

//used in jn_svr_poll()'s read call
#define JN_MODE_BLOCK			0
#define JN_MODE_NONBLOCK		1

#define JN_MIN_TXT_SIZE			2	/*a byte and a null,
						   first byte must not be '\0' */

#define JNMOD_MAGIC		0xdeaf

//search patterns
#define JN_FIND_FLAGS	(0x0001<<0)
#define JN_FIND_OBJID	(0x0001<<1)
#define JN_FIND_MODID	(0x0001<<2)
#define JN_FIND_NAME	(0x0001<<3)
#define JN_FIND_UID	(0x0001<<4)
#define JN_FIND_FD	(0x0001<<5)

/*		***	packet and payload	***		*/

#define JN_HS_BIG_ENDIAN	(0x1<<0)
#define MAX_PUBKEY_LEN		42
typedef struct JNetHandshakePacket {
	char ver:4, flags:4;
	char px[MAX_PUBKEY_LEN];
	char py[MAX_PUBKEY_LEN];
} jn_hs_pkt;

typedef struct JNetHandshakeReplyPacket {
	char ver:4, flags:4;
	char ekey[JN_CONF_KEY_LEN + ECIES_OVERHEAD];
} jn_hs_reply_pkt;

//jcs encoded
typedef struct JNetLoginData {
	//uchar flags;
	uchar username[MAX_UNAME];
	uchar pass[MAX_PASS];
} jn_login_payload;

//packet types
#define JNT_LOGIN		0x00
//control packets
#define JNT_OK			0x10
#define JNT_TIMEOUT		0x11
#define JNT_DC			0x12

#define JNT_COMLIST		0x14
#define JNT_COMOPEN		0x15
#define JNT_COMJOIN		0x17
#define JNT_COMLEAVE		0x19
#define JNT_COMPAUSE		0x1a
#define JNT_COMRESUME		0x1b
#define JNT_COMCLOSE		0x1c
#define JNT_COMREQ		0x1e

#define JNT_MODLIST		0x1f

//administration
#define JNT_USERKICK		0x50
#define JNT_USERBAN		0x51
#define JNT_USERUNBAN		0x52
#define JNT_USERLIST		0x53
#define JNT_SVRSHUTDOWN		0x54
#define JNT_SVRPAUSE		0x55
#define JNT_SVRRESUME		0x56
#define JNT_SVRUSERLIST		0x57
//failure
#define JNT_FAIL		0xff

#define JN_PKT_MAGIC		0xd1e4

#define JNF_PLAIN		(0x1<<0)	/*data's text message allowed on
						   JNT_FAIL or JNT_OK */
#define JNF_LOST		(0x1<<1)	/*the trans_id field set as
						   the lost packet's */
#define JNF_ERR			(0x1<<2)	/*error packet, rs will be errno
						 */
/*keep header size on 16 or expect errors (AES_BLOCK_SIZE)*/
struct JNetPacketHeader {
	uint16_t magic;
	uchar ver:4, flags:4;
	uchar type;
	uint16_t data_len;
	uint16_t comod_id;
	uchar trans_id;		/*transaction id, a transaction includes request
				   and replies, segmented if needed */
	uint32_t rs;		/*remainig size of transaction request
				   or reply or error code if JNF_ERR flags set
				   the rs includes packet header and encrypted data size
				   see jn_calc_rs() call in jn.c */
	uint16_t data_crc;
	uchar hdr_crc;
} __attribute__ ((packed));
typedef struct JNetPacketHeader jn_hdr;

#define JN_MAX_PKT_DATA	(JN_MAX_PKT - sizeof(jn_hdr))

typedef struct JNetPacket {
	jn_hdr hdr;
	uchar *data;		//[JN_MAX_PKT_DATA];//use if NO_HEAP defined
} jn_pkt;

typedef struct JNetPacketListEntry {
	jn_pkt *pkt;
	struct JNetPacketListEntry *next;
} jn_pkt_list;

/*
	Modules-Comod Control Messages
	
	COMOPEN:
		requests creating a new comod, requires user
		to have execute permissions to module
		requires module_id, comod_name. returns comod id
	COMJOIN:
		requests joining to an existing comod, needs at least read
		permission to the module to recieve announcements,
		requires module_id, comod_id.
		returns success or failure.
	COMCLOSE:
		requests closing a comod, needs ownership of the comod
	COMLIST:
		requests list of all comods in the server, requires read perm
		to the server object.
	COMUSERLIST:
		requests list of comod users
*/

#define JN_SF_STREAM	(0x01<<0)	//stream comod, pass data to module on arrive
#define JN_SF_ANNOUNCE	(0x01<<1)	//enable others to recv announcements from this comod
typedef struct JNetOpenComodRequest {
	uchar flags;
	jobjid_t objid;		//module object id, registered in a JAC DB
	uchar *com_name;	//copyname
} jn_open_com_req;

#define JN_JOIN_COM_UPDATE	(0x01<<0)	/*update me! currently the module
						   decides to send updates to which
						   connection, so the flag gets passed
						   to the module and there's no
						   guarranty to recieve updates
						   with this flag */
typedef struct JNetJoinComodRequest {
	uchar flags;
	jmodid_t com_id;
} jn_join_com_req;

typedef struct JNetLeaveComodRequest {
	jmodid_t com_id;
} jn_leave_com_req;

typedef struct JNetComodUserListRequest {
	jmodid_t com_id;
} jn_com_user_list_req;

typedef struct JNetComodUserListReply {
	jacid_t nusr;
	uchar *name_list;
} jn_com_user_list_rep;

typedef struct JNetComodCloseRequest {
	jmodid_t com_id;
} jn_com_close_req;

/*		***	module	***		*/

#define JN_PT_HALT	0x00
#define JN_PT_SHUTDOWN	0x01
#define JN_PT_WAKEUP	0x02
#define JN_PT_KICK	0x03
#define JN_PT_BAN	0x04
#define JN_PT_UNBAN	0x05
#define JN_PT_FD_OP	0x06	//add/remove fd
#define JN_PT_TRANS	0x07

#define JN_PS_ADMIN	0x00
#define JN_PS_USER	0x01
#define JN_PS_COMOD	0x02
#define JN_PS_SVR	0x03

typedef struct JNetPipePacketHeader {
	uchar type;
	uchar src_type;
	jacid_t src_id;		//who's boss? no problem for mod_id, same len as jacid
	uint16_t data_len;
} jn_phdr;

typedef struct JNetPipePacket {
	jn_phdr hdr;
	uchar data[JN_CONF_MAX_PDATA];
} jn_ppkt;

typedef struct JNetModuleListEntry {
	jobjid_t objid;
	wchar_t *pathname;
	struct JNetModuleListEntry *next;
} jn_mod_entry;

typedef struct JNetModuleList {
	uint32_t cnt;
#ifndef JN_SINGLE_CLT
	jmx_t mx;
#endif
	jn_mod_entry *first;
	jn_mod_entry *last;
} jn_mod_list;

/*		***	connection-struct	***	*/

#define JN_UCF_OWNER	(0x01<<0)

typedef struct JNetUserComodEntry {
	uchar flags;
	jobjid_t objid;
	jmodid_t mod_id;
	wchar_t *copyname;
	struct JNetUserComodEntry *next;
} jn_ucom_entry;

typedef struct JNetUserComodList {

/* use connection mx
#ifndef JN_SINGLE_CLT
	jmx_t mx;
#endif
*/
	uint16_t cnt;
	jn_ucom_entry *first;
	jn_ucom_entry *last;
} jn_ucom_list;

#define JN_CB_KEEP	(0x01<<0)
#define JN_CB_CONTINUE	(0x01<<1)
#define JN_CB_ERR	(0x01<<2)
#define JN_CB_TID	(0x01<<3)
#define JN_CB_COMID	(0x01<<4)
#define JN_CB_NOFREE	(0x01<<5)
#define JN_CB_THREAD	(0x01<<6)

typedef struct JNetCallbackFifoEntry {
	uchar flags;		//same as JN_PSF_* flags
	jtransid_t tid;		//trans id;
	jmodid_t comid;		//comod id;
	uint32_t nbytes;
	uint32_t nremain;
	time_t reg_time;

	char *buf;
	void *conn;
	jn_pkt *pkt;
	int jerr;
	void *progress_callback;
	void *progress_context;
	void *if_callback;
	void *if_context;
	struct JNetCallbackFifoEntry *next;
} jn_cb_entry;

typedef struct JNetCallbackFifo {
	uint32_t cnt;
	uchar flags;
#ifndef JN_SINGLE_CLT
	jmx_t mx;
	jsem_t sem;
#endif
	jn_cb_entry *first;
	jn_cb_entry *last;
} jn_cb_fifo;

typedef struct JNetTransArray {
	uchar cnt;
	uchar total;
#ifndef JN_SINGLE_CLT
	jmx_t mx;
#endif
	jtransid_t *tid;
} jn_trans_a;

#define JN_CF_CONNECT	(0x01<<0)
#define JN_CF_CLOSENOW	(0x01<<1)
#define JN_CF_BLOCK_TIME (0x01<<2)
#define JN_CF_BIG_ENDIAN_PEER	(0x01<<3)
#define JN_CF_PART_PKT	(0x01<<4)
#define JN_CF_SVR	(0x01<<5)	//server side connection
#define JN_CF_BCAST	(0x01<<6)	//used in bcast_pkt call
typedef struct JNetConnection {
	//flags, see LoginData JN_LF_* flags
	uchar flags;

	//ident/crypto
	jacid_t uid;		//set to JACID_INVAL to show that the slot is empty in a poll buck
	wchar_t *username;
	uchar key[JN_CONF_KEY_LEN];	//stored in jcs
	aes_context aes;
	struct sockaddr_storage addr;

	//track
	void *poll_buck;
#ifndef _WIN32
	nfds_t poll_pos;
#else
	uint32_t poll_pos;
#endif
	uint16_t errors;	//number of errors, set 0 on reset
	int sockfd;

#ifndef JN_SINGLE_CLT
	jthread_t cbthid;	//callback thread
	//jmx_t mx;
#endif
//      int pipefd[2];
	uint32_t bytes_rcvd;
	uint32_t bytes_sent;
	//uint16_t nreq; //total in-progress requests, gives flow control ability
	jn_ucom_list *ucom_list;

	uint16_t cbq_gc_trigger;	//callback garbage collector trigger
	uint16_t cbq_gc_to;	//time-out for tids

	jn_cb_fifo *cbfifo;
	jn_trans_a *trans_a;

	uchar prev[JN_MAX_PKT];
	uint32_t prev_len;

	//configuration
	uint16_t maxtrans;
	//uint16_t maxreq;

	struct JNetConnection *next;

} jn_conn;

typedef struct JNetConnectionLinkedList {
	uint32_t cnt;
#ifndef JN_SINGLE_CLT
	jmx_t mx;
#endif
	jn_conn *first;
	jn_conn *last;
	jn_conn *curr;		//used in comods
} jn_conn_list;

typedef struct JNetConnectionComodCopy {
	jn_conn *conn;
/*
#ifdef linux	
	struct aiocb cb;
#endif
*/
	struct JNetConnectionComodCopy *next;
} jn_cc_entry;

typedef struct JNetConnectionComodCopyList {
	uchar flags;
	uint32_t cnt;
#ifndef JN_SINGLE_CLT
	jmx_t mx;
#endif
	jn_cc_entry *first;
	jn_cc_entry *last;
} jn_cc_list;

#define JN_FF_FREE_CC_LIST	(0x01<<0)
#define JN_FF_FREE_DATA		(0x01<<1)	//free packet data
#define JN_FF_MAIN_CC_LIST	(0x01<<2)	//cc list is a pointer to the main list
					 //thus, there is no need to init a new
					 //cb list or free call                                  
#define JN_FF_NO_NEW		(0x01<<3)
#define JN_FF_USE_POOL		(0x01<<4)

#define JN_FT_PKT	0x00
#define JN_FT_PIPE	0x01
#define JN_FT_CLOSECOM	0x02
#define JN_FT_RM_CONN	0x03
#define JN_FT_PRE_PKT	0x04	//preprocessed packet, submitted by search thread
typedef struct JNetFifoEntry {
	uchar flags;
	uchar type;
	uint32_t datalen;
	jacid_t uid;
	jn_conn *conn;
	int fd;
	void *ptr;
	jn_cc_list *cc_list;
	struct JNetFifoEntry *next;
} jn_fifo_entry;

typedef struct JNetFifo {
	uchar flags;
	uint32_t cnt;

	//max q is constant ( 32 )
	uint32_t qbmap;
	void *qpoolhead;

#ifndef JN_SINGLE_CLT
	jmx_t mx;
	jsem_t sem;
#endif
	jn_fifo_entry *first;
	jn_fifo_entry *last;

} jn_fifo;

#define JN_COM_DONT_LIST	(0x01<<0)	//don't list in comodlist req
#define JN_COM_ANNOUNCE_DC	(0x01<<1)	//announce user's dc to others
#define JN_COM_SINGLE_CC_LIST	(0x01<<2)	//single cc list, shared among
						//all reply-fifos
#define JN_COM_PAUSED		(0x01<<3)
typedef struct JNetCopyOfModuleEntry {
	uchar flags;

	//ids
#ifndef JN_SINGLE_CLT
	jthread_t th_id;
	jthread_t rth_id;
#endif
	jmodid_t com_id;	//comod-id, unique
	jobjid_t objid;		//mod-id, registered in a jac as an object rec
	jacid_t owner_id;

	//track
	uint32_t bytes_sent;
	uint32_t bytes_rcvd;
	uint32_t maxreq;
	uint32_t nreq;

	wchar_t *modname;
	wchar_t *copyname;

#ifndef JN_SINGLE_CLT
	jmx_t reqmx;
	jcond_t req_cond;
#endif
	jn_fifo *fifo;
	jn_fifo *reply_fifo;

	//lists
	jn_cc_list *cc_list;	//no way bro! create a copy of user conn

	//internal comod handle
	void *com_data;

#ifndef JN_SINGLE_CLT
	jmx_t somx;		//shared object mutex, lock on access to below functions, needed?
#endif

	void *handle;		//file handle, see dl_open()
	//jn_h*, jn_comod_entry*, jn_fifo, void*
	int (*jnmod_init) (void *h, void *com_h, void *vconn);
	int (*jnmod_cleanup) (void *vh, void *vcom_h);
	int (*jnmod_handle_join) (void *vh, void *vcom_h, void *vconn,
				  void *req_pkt, void *vrfifo_fn);
	int (*jnmod_handle_leave) (void *vh, void *vcom_h, void *vconn,
				   void *req_pkt, void *vrfifo_fn);
	int (*jnmod_handle_packet) (void *h, void *com_h, void *job,
				    void *rfifo_fn);

	struct JNetCopyOfModuleEntry *next;

} jn_comod_entry;

typedef struct JNetCopyOfModuleList {
	uint32_t cnt;
#ifndef JN_SINGLE_CLT
	jmx_t mx;
#endif
	jn_comod_entry *first;
	jn_comod_entry *last;
	jn_comod_entry *curr;
} jn_comod_list;

/*		***	block-list	***		*/

typedef struct JNetBlockListEntry {
	wchar_t *name;
	struct sockaddr sa;
	struct JNetUserBlockList *next;
} jn_block_list_entry;

typedef struct JNetBlockList {
	uint32_t cnt;
#ifndef JN_SINGLE_CLT
	jmx_t mx;
#endif
	jn_block_list_entry *first;
	jn_block_list_entry *last;
} jn_block_list;

#define JN_IF_BIND	(0x01<<0)	//bind with address
#define JN_IF_LISTEN	(0x01<<1)	//listen on socket
#define JN_IF_SOCKOPT	(0x01<<2)	//set socket options
#define JN_IF_LOGIN	(0x01<<3)	//need login
#define JN_IF_CRYPT	(0x01<<4)	//enable crypto
#define JN_IF_CRC	(0x01<<5)	//enable crc
#define JN_IF_GUEST	(0x01<<6)	//allow guest login
#define JN_IF_FREE_GRP	(0x01<<7)	//group membership will not be checked, for
				  //guests, see jac.h: JAC_REC_NO_RESTR flag
typedef struct JNetConfig {
	uchar flags;

	//socket options
	int so_level;
	int so_optname;

	char *addr;
	char *port;
	int nlisten;

	wchar_t *mod_path;

	uint16_t nfds_buck;
	uint32_t max_users;
	uint16_t max_com;
	uint16_t max_mod;
	uint16_t max_ucom;
	uint16_t maxconnreq;
	uint16_t maxcomreq;
	uint16_t max_conn_trans;
	uint16_t poll_q;	//for pool'ed malloc
	uint16_t svr_q;		//for pool'ed malloc
	uint16_t poll_wr_req;
	uint16_t cbq_gc_trigger;
	uint16_t cbq_gc_timeout;

	uint32_t nmaxfifoq;
	uint32_t fifoqbufsize;

	struct timeval hs_tv;
	struct timeval login_tv;
	struct timeval pkt_tv;
	struct timeval trans_tv;

} jn_conf;

//used in _jn_request_send()
#define JN_SEND_COPY			(0x01<<0)
#define JN_SEND_FREE			(0x01<<1)

#define JN_WR_FREE_BUF	(0x01<<0)

typedef struct JNetWritePollArray {
	uchar flags;
	int fd;
	size_t bufsize;
	size_t pos;
	uchar *buf;
} jn_wr_array;

typedef struct JNetPollfdBucket {
	int fdop_flag;
#ifndef JN_SINGLE_CLT
	//jmx_t fdop_mx; //removed, use conn_list->mx
	jcond_t fdop_cond;
	jthread_t thid;
#ifndef _WIN32
	nfds_t nfds;
#else
	uint32_t nfds;
#endif
	jthread_t qth;
	jmx_t conn_a_mx;
#endif

#ifdef USE_JNPOLL
	fd_set readfds, writefds, exceptfds;
	int max_fd;
#endif

	jn_fifo *fifo;
	struct pollfd *fds;

	uint16_t *nreq;
	uint16_t max_wr_req;
	uint16_t maxconnreq;
	uint16_t *nwr_req;	//keeps nwr_req for each connection
	jn_wr_array *wr_a;	//write array

	int timeout;
	jn_conn *conn_a;	//JACID_INVAL shows an empty slot
	struct JNetPollfdBucket *next;
} jn_poll_buck;

typedef struct JNetPollfdBucketList {
	uint16_t cnt;
#ifndef JN_SINGLE_CLT
	jmx_t mx;
#endif
	jn_poll_buck *first;
	jn_poll_buck *last;
} jn_poll_list;

typedef struct JNetMessageList {
	int code;
	wchar_t *msg;
	struct JNetMessageList *next;
} jn_msg_list;

#define JN_MAGIC	0x1b2b
#define JNF_PAUSE	(0x01<<0)
#define JNF_SVR		(0x01<<1)

typedef struct JNetHandle {
	//kinda header
	uint16_t magic;
	uint8_t flags;

	//config
	jn_conf conf;

	//track
	int sockfd;
	int syse;
	int pipefd[2];
#ifndef JN_SINGLE_CLT
	jthread_t accept_th;
	jthread_t fifo_th;	//, reply_fifo_th;
#endif
	uint32_t nusers;
	wchar_t *svr_obj_name;

	//lists
	jn_poll_list *poll_list;
	jn_mod_list *mod_list;
	jn_comod_list *com_list;
	//jn_conn_list* conn_list; moved to poll_list
	jn_msg_list *msg_list;
	jn_block_list *block_list;
	//server fifo   
	jn_fifo *fifo;
	jn_fifo *reply_fifo;

	//buffers
#ifndef JN_SINGLE_LIST
	jmx_t comlist_buf_mx;
#endif
	uint32_t comlist_bufsize;
	uchar *comlist_buf;

	//access control
	//jac_h jac;

	//mutexes & condition var.s
#ifndef JN_SINGLE_CLT
	jmx_t jac_mx;
#endif
} jn_h;

//standard progress context
//open / join
struct _jn_open_com {
	uchar flags;
	wchar_t *copyname;
	jobjid_t objid;
	jmodid_t modid;
};

/*
	standard call-back function used in:
		jn_open_com
		jn_join_com
		jn_recv_*list
		
		void fn_name(jn_fifo_entry*);
*/

#define JN_NPKT(datasize)	((datasize)%JN_MAX_PKT_DATA)?(((datasize)/JN_MAX_PKT_DATA)+1):((datasize)/JN_MAX_PKT_DATA)
#define JN_RS_BY_CNT(datasize, cnt)	((datasize) <= JN_MAX_PKT_DATA)?0:((datasize)-cnt*JN_MAX_PKT_DATA)

#ifdef __cplusplus
extern "C" {
#endif

// connect/disconnect (basic)
	void *get_in_addr(struct sockaddr *sa);
	void jn_default_conf(jn_conf * conf);
	int jn_socket(jn_h * h);
	int jn_accept(jn_h * h, jn_conn * conn);
	int jn_connect(jn_conn * conn, char *hostname, char *port,
		       wchar_t * user, wchar_t * pass);
	int jn_close(jn_h * h, jn_conn * conn, uchar flags);

// module management
	int jn_mod_register_dir(jn_h * h);
	int jn_mod_register(jn_h * h, wchar_t * name, jobjid_t * objid);
	int jn_send_modlist(jn_h * h, jn_conn * conn, jtransid_t req_transid);
	int jn_send_comlist(jn_h * h, jn_conn * conn, jtransid_t req_transid);

	int jn_com_reload(jn_h * h, jn_comod_entry * hmod, jn_conn * conn);

//client-side
	int jn_open_com(jn_conn * conn, void *if_callback, void *if_context,
			struct _jn_open_com *open_s);
	int jn_join_com(jn_conn * conn, void *if_callback, void *if_context,
			struct _jn_open_com *open_s);
	int jn_resume_com(jn_conn * conn, void *if_callback, void *if_context,
			  jmodid_t com_id);
	int jn_pause_com(jn_conn * conn, void *if_callback, void *if_context,
			 jmodid_t com_id);
	int jn_leave_com(jn_conn * conn, void *if_callback, void *if_context,
			 jmodid_t com_id);
	int jn_close_com(jn_conn * conn, void *if_callback, void *if_context,
			 jmodid_t com_id);

	int jn_recv_list(jn_conn * conn, void *if_callback, void *if_context,
			 uchar type);
	int jn_reg_cb(jn_conn * conn, jn_pkt * pkt, jn_cb_entry * newf);
	int jn_unreg_cb(jn_conn * conn, jn_cb_entry * rmf);
	int jn_chg_cb(jn_conn * conn, jn_cb_entry * oldf, jn_cb_entry * newf);
	int jn_run_cb(jn_conn * conn, jn_pkt * pkt);	//aimed for single-threaded
	//clients

//parse-helper calls
	size_t jn_pkt_array(uchar * buf, size_t bufsize, jn_pkt ** pkt,
			    uchar * datapooladdr);
	void jn_free_pkt_array(jn_pkt ** pkt, size_t npkt,
			       uchar * datapooladdr);
	int jn_conn_cclist_to_jcslist(jn_cc_list * cc_list,
				      jcslist_entry ** jcslist);
	int jn_parse_comlist(uchar * buf, uint32_t bufsize,
			     jn_comod_entry ** list);
	int jn_parse_modlist(uchar * buf, uint32_t bufsize,
			     jn_mod_entry ** list);
/*
uint16_t jn_open_com(jn_conn* conn, jmodid_t mod_id, uchar flags);//returns comod id
int jn_end_com(jn_conn* conn, uint16_t com_id);
*/

	int jn_assm_plain(jn_pkt * pkt, jtransid_t trans_id, uchar type,
			  int16_t err_code, wchar_t * msg);
	int jn_assm_ctl(jn_pkt * pkt, jtransid_t trans_id, uchar type,
			uchar * buf);
	jn_cb_entry *jn_assm_cb(jn_conn * conn, uchar flags, jtransid_t tid,
				jmodid_t comid, void *progress_callback,
				void *progress_context, void *if_callback,
				void *if_context);

// send/recieve
	int jn_send(jn_conn * conn, jn_pkt * pkt);
	int jn_recv(jn_conn * conn, jn_pkt * pkt);
	int jn_send_ctl(jn_conn * conn, jtransid_t trans_id, uchar type,
			uchar * buf);
	int jn_send_plain(jn_conn * conn, jtransid_t trans_id, uchar type,
			  int16_t err_code, wchar_t * msg);
	int jn_send_aio(jn_conn * conn, jn_pkt * pkt, uchar flags);
	int jn_send_poll(jn_conn * conn, jn_pkt * pkt, uchar flags);

// initializing and shutdown
	int jn_init_svr(jn_h * h);
	int jn_start_svr(jn_h * h);
	void jn_shutdown_svr(jn_h * h);

	int jn_init_clt(jn_h * h, jn_conn * conn);
	void jn_shutdown_clt(jn_h * h);

#ifdef __cplusplus
}
#endif
#endif
