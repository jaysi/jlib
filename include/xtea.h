#ifndef XTEA_H
#define XTEA_H

#include "jtypes.h"

/*
  The recommended value for the "num_rounds" parameter is 32, not 64, 
  as each iteration of the loop does two Feistel-network rounds. 
  To additionally improve speed, the loop can be unrolled by pre-computing the 
  values of sum+key[].
*/
void XTEA_encipher(unsigned int num_rounds, uint32_t v[2], uint32_t const key[4]);
void XTEA_decipher(unsigned int num_rounds, uint32_t v[2], uint32_t const key[4]);

#else

#endif
