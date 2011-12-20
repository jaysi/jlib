#ifndef _AES_H
#define _AES_H

#include "jtypes.h"

#define AES_BSIZE	16

typedef struct {
	uint32 erk[64];		/* encryption round keys */
	uint32 drk[64];		/* decryption round keys */
	int nr;			/* number of rounds */
} aes_context;

int aes_set_key(aes_context * ctx, uint8 * key, int nbits);
void aes_encrypt(aes_context * ctx, uint8 input[AES_BSIZE],
		 uint8 output[AES_BSIZE]);
void aes_decrypt(aes_context * ctx, uint8 input[AES_BSIZE],
		 uint8 output[AES_BSIZE]);

#endif				/* aes.h */
