static inline void packi16(unsigned char *buf, unsigned int i)
{
	*buf++ = i >> 8;
	*buf++ = i;
}

/*
** packi32() -- store a 32-bit int into a char buffer (like htonl())
*/
static inline void packi32(unsigned char *buf, unsigned long i)
{
	*buf++ = i >> 24;
	*buf++ = i >> 16;
	*buf++ = i >> 8;
	*buf++ = i;
}

static inline void packi64(unsigned char *buf, unsigned long long i)
{
	*buf++ = i >> 56;
	*buf++ = i >> 48;
	*buf++ = i >> 40;
	*buf++ = i >> 32;
	*buf++ = i >> 24;
	*buf++ = i >> 16;
	*buf++ = i >> 8;
	*buf++ = i;
}

/*
** unpacki16() -- unpack a 16-bit int from a char buffer (like ntohs())
*/
static inline unsigned int unpacki16(unsigned char *buf)
{
	return (buf[0] << 8) | buf[1];
}

/*
** unpacki32() -- unpack a 32-bit int from a char buffer (like ntohl())
*/
static inline unsigned long unpacki32(unsigned char *buf)
{
	return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

static inline unsigned long long unpacki64(unsigned char *buf)
{
	return (unsigned long long)((buf[0] << 56) | (buf[1] << 48) | (buf[2] << 40) | (buf[3] << 32)
	    | (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7]);
}
