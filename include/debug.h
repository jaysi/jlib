#ifndef NDEBUG
#include <unistd.h>
#include <stdio.h>
#include <wchar.h>
/*
# ifndef _WIN32
void _dbuf(unsigned char* buf, int len);
#  define _deb(format, args...) fprintf(stderr, "_deb: %s()@ %s-> %d { " \
					, __func__, __FILE__, __LINE__); \
			fprintf(stderr, format, ## args); \
			fprintf(stderr, " }\n");
			
#  define _wdeb(format, args...) fwprintf(stderr, L"_deb: %s()@ %s-> %d { " \
					, __func__, __FILE__, __LINE__); \
			fwprintf(stderr, format, ## args); \
			fwprintf(stderr, L" }\n");
				
# else
void _dbuf(unsigned char* buf, int len);
*/

#define _deb(format, args...) printf("_deb: %s()@ %s-> %d { ", __func__ \
					, __FILE__, __LINE__); \
			printf(format, ## args); \
			printf(" }\n");

#define _wdeb(format, args...) wprintf(L"_deb: %s()@ %s-> %d { ", __func__ \
					, __FILE__, __LINE__); \
			wprintf(format, ## args); \
			wprintf(L" }\n");
#define _wdump(buf, len) for(int _cntr = 0; _cntr < len; _cntr++)wprintf(L"%02x  ", buf[_cntr]);wprintf(L"\n");

//# endif

#else
#define _deb(format, args...)
#define _wdeb(format, args...)
#define _dbuf(buf, len)
#define _wdump(buf, len)
#endif
