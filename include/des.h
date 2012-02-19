#ifndef _DES_H
#define _DES_H

#include "jtypes.h"

#define DES_BSIZE	8
#define DES3_BSIZE	8

#define DES_KSIZE	8
#define DES3_KSIZE	8

typedef struct {
	uint32 esk[32];		/* DES encryption subkeys */
	uint32 dsk[32];		/* DES decryption subkeys */
} des_context;

typedef struct {
	uint32 esk[96];		/* Triple-DES encryption subkeys */
	uint32 dsk[96];		/* Triple-DES decryption subkeys */
} des3_context;

int des_set_key(des_context * ctx, uint8 key[DES_KSIZE]);
void des_encrypt(des_context * ctx, uint8 input[DES_BSIZE], uint8 output[DES_BSIZE]);
void des_decrypt(des_context * ctx, uint8 input[DES_BSIZE], uint8 output[DES_BSIZE]);

int des3_set_2keys(des3_context * ctx, uint8 key1[DES3_KSIZE], uint8 key2[DES3_KSIZE]);
int des3_set_3keys(des3_context * ctx, uint8 key1[DES3_KSIZE], uint8 key2[DES3_KSIZE],
		   uint8 key3[DES3_KSIZE]);

void des3_encrypt(des3_context * ctx, uint8 input[DES3_BSIZE], uint8 output[DES3_BSIZE]);
void des3_decrypt(des3_context * ctx, uint8 input[DES3_BSIZE], uint8 output[DES3_BSIZE]);

#endif				/* des.h */
