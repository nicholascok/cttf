#ifndef __TRUETYPE_COMMON_H__
#define __TRUETYPE_COMMON_H__

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>

/**************************************************************************
 * GENERAL PURPOSE MACROS
 *************************************************************************/

#define SIGN(X) ( ((X) >= 0) - ((X) < 0) )
#define ABS(X) ( ((X) < 0) ? (-(X)) : (X) )
#define MIN(A, B) ( ((A) < (B)) ? (A) : (B) )
#define MAX(A, B) ( ((A) > (B)) ? (A) : (B) )

#define FROUND(X) ( ((X) < 0) ? (INT32) ((X) - 0.5) : (INT32) ((X) + 0.5) )

#define SWAP32(X0, X1) {\
	(*((UINT32*) (X0))) ^= (*((UINT32*) (X1)));\
	(*((UINT32*) (X1))) ^= (*((UINT32*) (X0)));\
	(*((UINT32*) (X0))) ^= (*((UINT32*) (X1)));\
}

/* 
 * macros for operating with 26.6 fixed point numbers.
 */
#define F26D6_FRAC_MASK  ( (UINT32) (0x3FU) )
#define F26D6MUL(X0, X1) ( (INT32) (((UINT32) (((INT32) (X0)) * ((INT32) (X1)))) >> 6) )
#define F26D6DIV(X0, X1) ( ((INT32) (((UINT32) (X0)) << 6)) / ((INT32) (X1)) )

/**************************************************************************
 * POINT MACROS
 *************************************************************************/

#define POINT_MASK_ONCURVE (0x01)
#define POINT_MASK_ONCURVE (0x01)
#define POINT_MASK_X_SHORT (0x02)
#define POINT_MASK_Y_SHORT (0x04)
#define POINT_MASK_REPEATS (0x08)
#define POINT_MASK_PX_SAME (0x10)
#define POINT_MASK_PY_SAME (0x20)

#define IS_ONCURVE(PNT) ( PNT.flags & POINT_MASK_ONCURVE )

/* 
 * POINT_MASK_X_SHORT and POINT_MASK_Y_SHORT are repurposed after reading
 * a glyph to hold point touch flags.
 */
#define POINT_MASK_XTOUCHED (0x02)
#define POINT_MASK_YTOUCHED (0x04)
#define POINT_MASK_TOUCHED  (0x06)

#define IS_TOUCHED(PNT)   ( PNT.flags & POINT_MASK_TOUCHED )
#define IS_TOUCHED_X(PNT) ( PNT.flags & POINT_MASK_XTOUCHED )
#define IS_TOUCHED_Y(PNT) ( PNT.flags & POINT_MASK_YTOUCHED )

/* 
 * types...
 */
typedef uint8_t   UINT8;
typedef uint16_t  UINT16;
typedef uint32_t  UINT32;
typedef uint64_t  UINT64;

typedef int8_t    INT8;
typedef int16_t   INT16;
typedef int32_t   INT32;
typedef int64_t   INT64;

typedef int16_t   FUNIT;   	/* 'shortFrac' by spec */
typedef int16_t   FRAC;    	/* 'shortFrac' by spec */
typedef int32_t   FIXED;   	/* 'Fixed' by spec */
typedef int32_t   F26D6;   	/* 'F26Dot6' by spec */
typedef int16_t   FWORD;   	/* 'FWord' by spec */
typedef uint16_t  UFWORD;  	/* 'uFWord' by spec */
typedef int16_t   F2D14;   	/* 'F2Dot14' by spec */
typedef uint64_t  DATETIME;	/* 'longDateTime' by spec */
typedef uint8_t   BYTE;

/* 
 * 2D floating point vector...
 */
struct vec2f {
	float x, y;
};

/* 
 * used to hold 26.6 fixed-point point coordinates...
 */
struct vec32fx {
	INT32 x, y;
};

/* 
 * struct point holds cordinates and flags associated with a point.
 */
struct point {
	struct vec32fx pos;
	BYTE flags;
};

struct glyph {
	struct point* points;
	UINT32 num_points;
	
	INT32 num_contours;
	INT32* contours;
	
	UINT16 code;
};

/* 
 * the dpi of the user's primary display.
 */
extern int MONITOR_DPI;

/* 
 * dot2f(_v1, _v2) computes the dot product between the two vectors _v1
 * and _v2.
 */
float dot2f(struct vec2f _v1, struct vec2f _v2);

/* 
 * fisqrt(_x) computes the inverse of the square root of the floating point
 * value _x. (yes, based on the quake algorithm - to avoid math.h)
 */
float fisqrt(float _x);

#endif /* __TRUETYPE_COMMON_H__ */
