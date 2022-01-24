#ifndef __SPECIFIED_READ_H__
#define __SPECIFIED_READ_H__

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>

/**
 * reads fields of data from a file descriptor given specified sizes and
 * endianness.
 * @param _fd: the file descriptor to be used.
 * @param _fmt: a null-terminated format specifier with
 * one character for each provided argument specifying the size and
 * endianness of that argmuent. entries can be any of the following:
 *   '2', '4', or '8': reads that many bytes, in spite of its endianness.
 *   'b', 'B', or '1': reads a 1-byte field.
 *   'W': reads a 2-byte big endian field.
 *   'w': reads a 2-byte little endian field.
 *   'D': reads a 4-byte big endian field.
 *   'd': reads a 4-byte little endian field.
 *   'Q': reads a 8-byte big endian field.
 *   'q': reads a 8-byte little endian field.
 * @note: any other (non null) character will be skipped.
 * @return the number of fufilled arguments.
 */
size_t sread(int _fd, char* _fmt, ...);

/**
 * a wrapper function for sread that reads using the specified format
 * character _cnt times.
 * @param _fd: the file descriptor to be used.
 * @param _fmt: a format character, as specified by sread
 * @param _cnt: the amount of times to read.
 * @param _buf: a buffer to read resulting data into.
 * @return _cnt
 */
size_t sreadn(int _fd, char _fmt, size_t _cnt, void* _buf);

/* 
 * the following functions are wrappers for sread that read a single
 * occurance of a specifier and return the result.
 */

uint8_t  sreadB(int _fd);
uint8_t  sreadb(int _fd);
uint16_t sreadW(int _fd);
uint16_t sreadw(int _fd);
uint32_t sreadD(int _fd);
uint32_t sreadd(int _fd);
uint64_t sreadQ(int _fd);
uint64_t sreadq(int _fd);
uint8_t  sread1(int _fd);
uint16_t sread2(int _fd);
uint32_t sread4(int _fd);
uint64_t sread8(int _fd);

/**
 * a wrapper function for normal malloc that checks the memory was actually
 * allocated, and if not the function throws a runtime error.
 * @param _n: the size of the allocation.
 * @return a pointer to the newly allocated memory.
 */
void* smalloc(size_t _n);

#endif /* __SPECIFIED_READ_H__ */
