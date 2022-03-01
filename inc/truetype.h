/* 
 * a font engine capable of parsing, hinting, and digitalising glyphs from
 * a TrueType font (.ttf) file.
 * 
 * a few notes:
 * 
 * -> the round_state IDs in the specification (RTG = 1, ROFF = 5, etc.)
 *    are not used. instructions that would normally set this value instead
 *    set the rounding parameters ROUND_PERIOD, ROUND_THRESHOLD, and
 *    ROUND_PHASE directly. they do this by setting a value 'vm.round.state'
 *    to the 8-bit binary structure taken from the stack and calling
 *    update_round_state().
 * 
 * -> zone pointers and reference points are stored in "registers" instead
 *    of the graphics state. this just makes sense.
 * 
 * -> the stack pointer register (SP) holds the index of the top of the
 *    stack plus one (the number of items in the stack), and the execution
 *    pointer register (EP) points to the next instruction to be executed,
 *    not the current one.
 * 
 * -> for now, floating point numbers are used in most intermediary
 *    calculations. this is why floating point versions of graphics state
 *    vectors are stored. this should change in the future.
 * 
 * -> the specifications from both Apple and Microsoft are terrible, they
 *    leave out alot of detail so there has been some guesswork involved.
 * 
 * -> on a related note, comments in the FreeType source code helped alot,
 *    they note a number of undocumented details they found by talking to
 *    some folk at Microsoft.
 * 
 * references:
 * 
 * Apple TrueType:
 * https://developer.apple.com/fonts/TrueType-Reference-Manual/RM04/Chap4.html
 * 
 * Microsoft OpenType (references TrueType):
 * https://docs.microsoft.com/en-us/typography/opentype/spec/tt_graphics_state
 * 
 * FreeType source:
 * https://github.com/freetype/freetype
 * 
 * TODO:
 * 
 * -> make VM isntruction take from glyph?
 * -> make contour[0] = 0 instead of -1 - a lot of headache...
 * 
 * -> switch to exclusively fixed-point arithmetic
 * 
 * -> add support for character mappings other than format 4, format 12 is
 *    of particular interest
 * 
 * -> add support for non-square pixels
 * 
 * -> clean code
 * 
 * -> kerning...
 * 
 * disclaimer: this was done for educational value, and is not intended for
 * production use.
 * 
 */

#ifndef __TRUETYPE_H__
#define __TRUETYPE_H__

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>

#include "truetype_common.h"
#include "truetype_virtual.h"

struct font {
	char name[NAME_LENGTH];
	
	int fd;
	
	F26D6 point_size;
	UINT16 fupem;
	UINT16 ppem;
	
	UINT16 flags;
	
	struct {
		UINT16 segcount;
		UINT16* start_codes;
		UINT16* end_codes;
		UINT16* offsets;
		UINT16* deltas;
		
		off_t glyf_table_offset;
		INT16 loca;
		
		union {
			UINT16* f0;
			UINT32* f1;
		} locate;
	} _cmap;
	
	struct {
		UINT16 glyphs;
		UINT16 points;
		UINT16 contours;
		UINT16 twilight;
		UINT16 storage;
		UINT16 control_values;
		UINT16 function_defs;
		UINT16 instruction_defs;
		UINT16 instructions;
		UINT16 stack_depth;
	} max;
	
	struct vm* virtm;
	
	struct proceedure prep_program;
	struct proceedure* functions;
	struct proceedure* userdefs;
	FWORD* unscaled_cvt;
	
	UINT32* storage;
	
	INT32* cvt;
};

/* 
 * load_font(_fp) imports the truetype font located at _fp and returns a
 * structure to contain it.
 */
struct font* load_font(char* _fp);

/* 
 * set_font_size(_fnt, _points) sets the point size of the font _fnt to
 * _points points and (if applicable) executes the font's 'prep' program
 * to update values in the control value table. if successful, the function
 * returns 0, otherwise, -1 is returned.
 */
int set_font_size(struct font* _fnt, int _points);

int read_glyph(struct font* _fnt, off_t _offset);

#endif /* __TRUETYPE_H__ */
