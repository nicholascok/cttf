#ifndef __ENDIAN_H__
#define __ENDIAN_H__

#include <stdint.h>

#define SWAP_ENDIAN_WORD(X) (((X << 8) & 0xFF00) | ((X >> 8) & 0x00FF))

#define SWAP_ENDIAN_DWORD(X) ((uint32_t) (\
	(((uint32_t) X >> 24) & 0x000000FFUL) |\
	(((uint32_t) X >> 8 ) & 0x0000FF00UL) |\
	(((uint32_t) X << 8 ) & 0x00FF0000UL) |\
	(((uint32_t) X << 24) & 0xFF000000UL)))

#define SWAP_ENDIAN_QWORD(X) ( __SWAP_ENDIAN_QWORD(X) )

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	
	#define LITTLE_ENDIAN_WORD(X) SWAP_ENDIAN_WORD(X)
	
	#define LITTLE_ENDIAN_DWORD(X) SWAP_ENDIAN_DWORD(X)
	
	#define LITTLE_ENDIAN_QWORD(X) SWAP_ENDIAN_QWORD(X)
	
	#define BIG_ENDIAN_WORD(X)  X
	
	#define BIG_ENDIAN_DWORD(X) X
	
	#define BIG_ENDIAN_QWORD(X) X
	
#else
	
	#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
		#warning Could not determine system endianness (assumng little endian).
	#endif
	
	#define BIG_ENDIAN_WORD(X) SWAP_ENDIAN_WORD(X)
	
	#define BIG_ENDIAN_DWORD(X) SWAP_ENDIAN_DWORD(X)
	
	#define BIG_ENDIAN_QWORD(X) SWAP_ENDIAN_QWORD(X)
	
	#define LITTLE_ENDIAN_WORD(X)  X
	
	#define LITTLE_ENDIAN_DWORD(X) X
	
	#define LITTLE_ENDIAN_QWORD(X) X
	
#endif

/* 
 * C89 doesn't officially support 64-bt integer constants, so
 * thats why this mess is here...  (there is a better way)
 */
uint64_t __SWAP_ENDIAN_QWORD(uint64_t _x);

#endif /* __ENDIAN_H__ */
