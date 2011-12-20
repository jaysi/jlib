#include "jer.h"
#include "jcs.h"

#include <string.h>
#ifdef linux
#include <error.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <wchar.h>

static struct JErrorStrings {
	int no;
	const char *str;
} jerrors[] = {
	{
	0, "Success;"},
	    //file
	{
	JE_OPEN, "Could not open file;"}, {
	JE_READ, "Could not read from file;"}, {
	JE_WRITE, "Could not write to file;"}, {
	JE_CRC, "Bad CRC;"}, {
	JE_NOOPEN, "Not opened yet;"}, {
	JE_ISOPEN, "The file is already open;"}, {
	JE_LASTOP, "Last operation failed;"}, {
	JE_NOJAC, "No open JAC;"}, {
	JE_NOFILE, "No file;"}, {
	JE_PIPE, "Pipe failed;"}, {
	JE_HCRC, "Bad header CRC;"}, {
	JE_CORRUPT, "Data corrupted;"},
	    //user management
	{
	JE_NOUSER, "Bad user name;"}, {
	JE_NOGROUP, "Bad group name;"}, {
	JE_NOOBJECT, "Bad object name;"}, {
	JE_NOCONF, "Configuration file not loaded;"}, {
	JE_NOLOGIN, "Could not login user;"}, {
	JE_PASS, "Bad password;"}, {
	JE_UIDFULL, "No empty user id;"}, {
	JE_GIDFULL, "No empty group id;"}, {
	JE_NOPERM, "Permission denied;"}, {
	JE_TIKS, "No more tickets;"}, {
	JE_MAXLOGINS, "Maximum login limit;"}, {
	JE_SIZE, "Bad size;"}, {
	JE_BADCONF, "Bad configuration;"}, {
	JE_BUFOVER, "Buffer overflow;"},
	    //network
	{
	JE_ACCEPT, "Accept call faild;"}, {
	JE_SEND, "Sending failed;"}, {
	JE_RECV, "Recieve failed;"}, {
	JE_BADPUBKEY, "Bad public key;"}, {
	JE_SOCKET, "Socket failed;"}, {
	JE_SETSOCKOPT, "Failed to set socket options;"}, {
	JE_BIND, "Binding failed;"}, {
	JE_LISTEN, "Listen call failed;"}, {
	JE_CONNECT, "Connect failed;"}, {
	JE_HDR, "Bad packet header;"}, {
	JE_DC, "Disconnected;"}, {
	JE_BCAST, "Broadcasting failed;"}, {
	JE_PKTLOST, "Packet lost;"}, {
	JE_TID, "Bad transaction id;"},
	    //shared, common
	{
	JE_EXISTS, "Already exists;"}, {
	JE_NOTFOUND, "Not found;"}, {
	JE_UNK, "Unknown operation;"}, {
	JE_IMPLEMENT, "Not implemented;"}, {
	JE_TYPE, "Bad type;"}, {
	JE_NOINIT, "Not initialized;"}, {
	JE_ALREADYINIT, "Already initialized;"}, {
	JE_LIMIT, "Limitation reached;"}, {
	JE_DISABLE, "Currently disabled;"}, {
	JE_SYSE, "System error;"}, {
	JE_VER, "Bad version;"}, {
	JE_FORBID, "Forbidden operation;"}, {
	JE_TIMEOUT, "Operation timed-out;"}, {
	JE_GETTIME, "Failed to get local time;"}, {
	JE_CONTINUE, "Continue;"}, {
	JE_NEXT_I, "Next index failed;"}, {
	JE_NEXT_BLOCK, "Next block failed;"}, {
	JE_MAGIC, "Bad magic number;"}, {
	JE_END, "End of data;"}, {
	JE_INV, "Invalid data;"}, {
	JE_EMPTY, "Empty field;"}, {
	JE_FUL, "Full capacity;"}, {
	JE_FLAG, "Bad flags;"}, {
	JE_FAIL, "Operation failure;"}, {
	JE_PAUSE, "Service paused;"}, {
	JE_LOWRES, "Low resources;"}, {
	JE_LOWBUF, "Insufficient buffer;"}, {
	JE_TMPUNAV, "Temporary unavailable;"}, {
	JE_AGAIN, "Try again;"},
	    //memmory
	{
	JE_NOMEM, "Out of memmory;"}, {
	JE_MALOC, "Memmory allocation failed;"}, {
	JE_NULLPTR, "Null pointer;"},
	    //threads
	{
	JE_THREAD, "Could not start thread;"}, {
	JE_THREADJOIN, "Could not join thread;"},
	    //modules
	{
	JE_BADMOD, "Bad module;"}, {
	JE_RESOLV, "Unable to resolve;"},
	    //end of struct
	{
	0, 0}
};

int jer_lookup_str(int err, char *dest)
{
	struct JErrorStrings *e;

	if (err == -JE_SYSE) {
		strlcpy(dest, "System: ", MAX_ERR_STR);
		strlcat(dest, strerror(errno), MAX_ERR_STR);
		return 0;
	}

	for (e = jerrors; e->str; e++) {
		if (e->no == -err) {
			strlcpy(dest, e->str, MAX_ERR_STR);
			return 0;
		}
	}
	return -JE_NOTFOUND;
}
