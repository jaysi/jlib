#ifndef _JTYPES_H
#define _JTYPES_H

#include <sys/types.h>
#include <stdint.h>

#ifndef u_int8_t
typedef unsigned char u_int8_t;
#endif

#ifndef u_int16_t
typedef unsigned short u_int16_t;
#endif

/*
#ifndef _WIN32
#ifndef u_int32_t
typedef unsigned long u_int32_t;
#endif
#endif
*/

#ifndef u_int64_t
typedef unsigned long long u_int64_t;
#endif

/*
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif
*/
#ifndef _STDINT_H

#ifndef uint8_t
typedef u_int8_t uint8_t;
#endif
#ifndef uint16_t
typedef u_int16_t uint16_t;
#endif

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif

#endif /*_STDINT_H*/

#ifndef uint8
typedef uint8_t uint8;
#endif
#ifndef uint16
typedef uint16_t uint16;
#endif

#ifndef uint32
typedef uint32_t uint32;
#endif
#ifndef uint64
typedef uint64_t uint64;
#endif

#ifndef uchar
typedef unsigned char uchar;
#endif

#ifndef jacid_t
typedef uint32_t jacid_t;
#endif

#ifndef jobjid_t
typedef uint32_t jobjid_t;
#endif

#ifndef jmodid_t
typedef uint16_t jmodid_t;
#endif

#ifndef jchar_t
typedef uint16_t jchar_t;
#endif

#ifndef jtransid_t
typedef uchar jtransid_t;
#endif

#ifndef jtypeid_t
typedef uint16_t jtypeid_t;
#endif

#ifndef jbid_t
typedef uint32_t jbid_t;
#endif

#endif
