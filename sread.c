#include "sread.h"

#include "endian.h"
#include "error.h"

size_t sread(int _fd, char* _fmt, ...) {
	/* (I know this is stupid, its not a big issue) */
	static unsigned char lookup[256] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 4, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 2,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	size_t argcnt;	/* number of arguments processed so far */
	short argsz;  	/* argument size */
	void* ptr;    	/* argument (pointer) */
	
	va_list fields;
	va_start(fields, _fmt);
	
	for (argcnt = 0; _fmt[argcnt]; argcnt++) {
		ptr = va_arg(fields, void*);
		
		/* get argument size */
		argsz = lookup[(int) _fmt[argcnt]];
		
		if (ptr == NULL) {
			/* if an argument is assigned NULL, we will skip the same
			 * amount of bytes specified in its format specifier. */
			if (lseek(_fd, argsz, SEEK_CUR) == -1)
				return argcnt;
		} else {
			/* otherwise, we will read the same amount of bytes specified
			 * in its format specifier. */
			if (read(_fd, ptr, argsz) < argsz)
				return argcnt;
			
			/* then, according again to the format specifier for our
			 * argument, we will determine whether values should be
			 * converted to big or little endian, or left as is. */
			switch (_fmt[argcnt]) {
				case 'W':
					*((uint16_t*) ptr) = BIG_ENDIAN_WORD(*((uint16_t*) ptr));
					break;
				case 'w':
					*((uint16_t*) ptr) = LITTLE_ENDIAN_WORD(*((uint16_t*) ptr));
					break;
				case 'D':
					*((uint32_t*) ptr) = BIG_ENDIAN_DWORD(*((uint32_t*) ptr));
					break;
				case 'd':
					*((uint32_t*) ptr) = LITTLE_ENDIAN_DWORD(*((uint32_t*) ptr));
					break;
				case 'Q':
					*((uint64_t*) ptr) = BIG_ENDIAN_QWORD(*((uint64_t*) ptr));
					break;
				case 'q':
					*((uint64_t*) ptr) = LITTLE_ENDIAN_QWORD(*((uint64_t*) ptr));
					break;
				
				case 'b': case 'B': case '1': case '2':
				case '4': case '8': break;
				
				default: continue;
			}
		}
	}
	
	return argcnt;
}

size_t sreadn(int _fd, char _fmt, size_t _cnt, void* _buf) {
	char format[2] = {0};
	size_t size = 0;
	size_t i;
	
	format[0] = _fmt;
	
	switch (_fmt) {
		case 'B': case 'b': case '1': size = 1; break;
		case 'W': case 'w': case '2': size = 2; break;
		case 'D': case 'd': case '4': size = 4; break;
		case 'Q': case 'q': case '8': size = 8; break;
	}
	
	for (i = 0; i < _cnt; i++) {
		sread(_fd, format, _buf);
		*((uint8_t**) &_buf) += size;
	}
	
	return _cnt;
}

uint8_t  sreadB(int _fd) { uint8_t  x; sread(_fd, "B", &x); return x; }
uint8_t  sreadb(int _fd) { uint8_t  x; sread(_fd, "b", &x); return x; }
uint16_t sreadW(int _fd) { uint16_t x; sread(_fd, "W", &x); return x; }
uint16_t sreadw(int _fd) { uint16_t x; sread(_fd, "w", &x); return x; }
uint32_t sreadD(int _fd) { uint32_t x; sread(_fd, "D", &x); return x; }
uint32_t sreadd(int _fd) { uint32_t x; sread(_fd, "d", &x); return x; }
uint64_t sreadQ(int _fd) { uint64_t x; sread(_fd, "Q", &x); return x; }
uint64_t sreadq(int _fd) { uint64_t x; sread(_fd, "q", &x); return x; }
uint8_t  sread1(int _fd) { uint8_t  x; sread(_fd, "1", &x); return x; }
uint16_t sread2(int _fd) { uint16_t x; sread(_fd, "2", &x); return x; }
uint32_t sread4(int _fd) { uint32_t x; sread(_fd, "4", &x); return x; }
uint64_t sread8(int _fd) { uint64_t x; sread(_fd, "8", &x); return x; }

void* smalloc(size_t _n) {
	void* ptr = malloc(_n);
	if (ptr == NULL) XERR(XEMSG_NOMEM, ENOMEM);
	
	return ptr;
}
