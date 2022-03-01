#include "specific.h"

void* smalloc(size_t _sz, size_t _n) {
	void* ptr = malloc(_sz * _n);
	
	if (ptr == NULL) {
		perror("smalloc");
		exit(errno);
	}
	
	return ptr;
}

void* scalloc(size_t _sz, size_t _n) {
	void* ptr = calloc(_sz, _n);
	
	if (ptr == NULL) {
		perror("scalloc");
		exit(errno);
	}
	
	return ptr;
}

/* this is a bit stupid but care I do not... */
static char size_lookup[256] = {
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

size_t sread(int _fd, char* _fmt, ...) {
	size_t argcnt;	/* number of arguments processed thus far */
	char argsz;   	/* size of current format specifier in bytes */
	void* ptr;    	/* argument for current specifier */
	
	va_list fields;
	va_start(fields, _fmt);
	
	for (argcnt = 0; _fmt[argcnt]; argcnt++) {
		/* extract an argument, */
		ptr = va_arg(fields, void*);
		
		/* and get that argument's size based on the current
		 * format specifier. */
		argsz = size_lookup[(unsigned) _fmt[argcnt]];
		
		/* if the argument associated with the current format
		 * specifier is NULL, then we will skip it. */
		if (ptr == NULL) {
			/* if lseek fails we will assume that the stream
			 * has been exhausted and return. */
			if (lseek(_fd, argsz, SEEK_CUR) == -1)
				return argcnt;
			
			continue;
		}
		
		/* otherwise, we will read the amount of bytes
		 * needed to fill the argument and perform any necessary
		 * processing of the aquired data. */
		
		/* if lseek fails we will assume that the stream has been
		 * exhausted and return the number of fufilled arguments. */
		if (read(_fd, ptr, argsz) < argsz)
			return argcnt;
		
		/* otherwise, we will determine how the argument is
		 * processed based on the current format specifier. */
		switch (_fmt[argcnt]) {
			case 'W':
				*((uint16_t*) ptr) =
					BIG_ENDIAN_WORD(*((uint16_t*) ptr));
				break;
			case 'w':
				*((uint16_t*) ptr) =
					LITTLE_ENDIAN_WORD(*((uint16_t*) ptr));
				break;
			case 'D':
				*((uint32_t*) ptr) =
					BIG_ENDIAN_DWORD(*((uint32_t*) ptr));
				break;
			case 'd':
				*((uint32_t*) ptr) =
					LITTLE_ENDIAN_DWORD(*((uint32_t*) ptr));
				break;
			case 'Q':
				*((uint64_t*) ptr) =
					BIG_ENDIAN_QWORD(*((uint64_t*) ptr));
				break;
			case 'q':
				*((uint64_t*) ptr) =
					LITTLE_ENDIAN_QWORD(*((uint64_t*) ptr));
				break;
			
			case 'b': case 'B': case '1': case '2':
			case '4': case '8': break;
			
			default: continue;
		}
	}
	
	return argcnt;
}

size_t sreadn(int _fd, char _fmt, size_t _N, void* _buf) {
	char sz = size_lookup[(unsigned) _fmt];
	size_t cnt;
	
	/* convert format character to a string so it can be passed to
	 * sread (see above). */
	char format[2] = {0};
	format[0] = _fmt;
	
	/* attempt to read _N times... */
	for (cnt = 0; cnt < _N; cnt++) {
		if (sread(_fd, format, _buf) < 1)
			break;
		
		_buf = (uint8_t*) _buf + sz;
	}
	
	return cnt;
}

uint8_t sreadB(int _fd) {
	uint8_t x = 0;
	sread(_fd, "B", &x);
	return x;
}

uint8_t sreadb(int _fd) {
	uint8_t x = 0;
	sread(_fd, "b", &x);
	return x;
}

uint16_t sreadW(int _fd) {
	uint16_t x = 0;
	sread(_fd, "W", &x);
	return x;
}

uint16_t sreadw(int _fd) {
	uint16_t x = 0;
	sread(_fd, "w", &x);
	return x;
}

uint32_t sreadD(int _fd) {
	uint32_t x = 0;
	sread(_fd, "D", &x);
	return x;
}

uint32_t sreadd(int _fd) {
	uint32_t x = 0;
	sread(_fd, "d", &x);
	return x;
}

uint64_t sreadQ(int _fd) {
	uint64_t x = 0;
	sread(_fd, "Q", &x);
	return x;
}

uint64_t sreadq(int _fd) {
	uint64_t x = 0;
	sread(_fd, "q", &x);
	return x;
}

uint8_t sread1(int _fd) {
	uint8_t  x = 0;
	sread(_fd, "1", &x);
	return x;
}

uint16_t sread2(int _fd) {
	uint16_t x = 0;
	sread(_fd, "2", &x);
	return x;
}

uint32_t sread4(int _fd) {
	uint32_t x = 0;
	sread(_fd, "4", &x);
	return x;
}

uint64_t sread8(int _fd) {
	uint64_t x = 0;
	sread(_fd, "8", &x);
	return x;
}

uint16_t __SWAP_ENDIAN_WORD(uint16_t X) {
	return ((X << 8) & 0xFF00U) |
	       ((X >> 8) & 0x00FFU);
}

uint32_t __SWAP_ENDIAN_DWORD(uint32_t X) {
	return (((uint32_t) X >> 24) & 0x000000FFUL) |
	       (((uint32_t) X >> 8 ) & 0x0000FF00UL) |
	       (((uint32_t) X << 8 ) & 0x00FF0000UL) |
	       (((uint32_t) X << 24) & 0xFF000000UL);
}

uint64_t __SWAP_ENDIAN_QWORD(uint64_t X) {
	((uint32_t*) &X)[0] = __SWAP_ENDIAN_DWORD(((uint32_t*) &X)[1]);
	((uint32_t*) &X)[1] = __SWAP_ENDIAN_DWORD(((uint32_t*) &X)[0]);
	return X;
}
