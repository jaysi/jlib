#ifndef _ECC_H
#define _ECC_H

typedef struct _ecc_keys {
	char px[42];
	char py[42];
	char priv[42];
} ecc_keys;

//typedef struct _ecc_keys ecc_keys;

#define DEGREE 163		/* the degree of the field polynomial */
#define MARGIN 3		/* don't touch this */
#define NUMWORDS ((DEGREE + MARGIN + 31) / 32)
#define ECIES_OVERHEAD (8 * NUMWORDS + 8)

/*memmory allocation for encrypted message
  int len = strlen(text) + 1;
  char *encrypted = malloc(len + ECIES_OVERHEAD);
  char *decrypted = malloc(len);
*/
//extern int ECIES_overhead();
extern ecc_keys *ECIES_generate_keys();
extern int ECIES_public_key_validation(const char *Px, const char *Py);
extern void ECIES_encryption(char *msg, const char *text, int len,
			     const char *Px, const char *Py);
extern int ECIES_decryption(char *text, const char *msg, int len,
			    const char *privkey);
#endif
