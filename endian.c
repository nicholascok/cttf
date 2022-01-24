#include "endian.h"

uint64_t __SWAP_ENDIAN_QWORD(uint64_t _x) {
	((uint32_t*) &_x)[0] = SWAP_ENDIAN_DWORD(((uint32_t*) &_x)[0]);
	((uint32_t*) &_x)[1] = SWAP_ENDIAN_DWORD(((uint32_t*) &_x)[1]);
	((uint32_t*) &_x)[0] ^= ((uint32_t*) &_x)[1];
	((uint32_t*) &_x)[1] ^= ((uint32_t*) &_x)[0];
	((uint32_t*) &_x)[0] ^= ((uint32_t*) &_x)[1];
	return _x;
}
