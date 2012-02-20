#include "aes.h"
#include "des.h"
#include "ecc.h"
#include "rc4.h"

//special
#define JCRYPT_NONE	0x00	//no encryption

//symmetric key - block cipher
#define JCRYPT_AES	0x01
#define JCRYPT_DES	0x02
#define JCRYPT_DES3	0x03

//symmetric key - stream cipher
#define JCRYPT_RC4	0xe0

//asymmetric key
#define JCRYPT_ECC	0xf0

//macros
#define crypt_size(size, bsize)	((size)%(bsize) == 0)?(size):((((size) / (bsize))+1)*(bsize))

struct jcrypt{
	unsigned char type;
	
};
