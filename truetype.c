#include "truetype.h"
#include "endian.h"
#include "error.h"
#include "sread.h"

int MONITOR_DPI = 96;

#include "instructions.c"

int read_name(struct font* _fnt, off_t _offset) {
	int i;
	
	UINT16 format;     	/* table format */
	UINT16 count;      	/* number of name records */
	UINT16 data_offset;	/* offset to beginning of data */
	
	UINT16 nameid;     	/* type of data stored here (font name = 4) */
	UINT16 length;     	/* length of string */
	UINT16 name_offset;	/* offset to beginning of string */
	
	lseek(_fnt->fd, _offset, SEEK_SET);
	
	/* read table header */
	sread(_fnt->fd, "WWW", &format, &count, &data_offset);
	
	/* only format 0 is supported (only checking because OpenType has
	 * another format specified). */
	if (format != 0) return -1;
	
	/* loop through as may as 'count' namerecords */
	for (i = 0; i < count; i++) {
		/* ignore all fields except nameid, length, and offset. search for
		 * and entry with nameid 4 to signify the font name (this is all
		 * we care about, for now). */
		sread(_fnt->fd, "222WWW", NULL, NULL, NULL, &nameid, &length,
			&name_offset);
		
		if (nameid == 4) {
			lseek(_fnt->fd, _offset + data_offset + name_offset, SEEK_SET);
			read(_fnt->fd, _fnt->name, (length > 255) ? 255 : length);
			return 0;
		}
	}
	
	return -1;
}

int read_head(struct font* _fnt, off_t _offset) {
	UINT16 flags;	/* font flags */
	UINT16 fupem;	/* units per em */
	
	INT16 loca_format;
	
	lseek(_fnt->fd, _offset, SEEK_SET);
	
	/* read table (most of it we will not need) */
	sread(_fnt->fd, "44442W882222222W2",
		NULL,
		NULL,
		NULL,
		NULL,
		&flags,
		&fupem,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		&loca_format,
		NULL
	);
	
	_fnt->cmap.loca = loca_format;
	_fnt->fupem = fupem;
	_fnt->flags = flags;
	
	return 0;
}

int read_cmap(struct font* _fnt, off_t _offset) {
	int i;
	
	UINT16 num_subtables;	/* number of encoding subtables present */
	
	/* note: for platform_id and specific_id, only combinations (0, 3) or
	 * (3, 1) are supported (these say that we have support for unicode 2.0
	 * with the basic multilingual plane (BMP) only). */
	 
	UINT16 platform_id;	/* platform identifier */
	UINT16 specific_id;	/* platform specific identifier */
	UINT32 offset;     	/* offset to subtable */
	
	UINT16 format;     	/* subtable format (only 4 is supported) */
	UINT16 length;     	/* subtable length */
	
	UINT16 segcount;   	/* number of subtable segments */
	
	UINT16* end_codes;  	/* glyph index lookup parameters */
	UINT16* start_codes;	/* ... */
	UINT16* deltas;
	UINT16* offsets;
	
	lseek(_fnt->fd, _offset, SEEK_SET);
	
	/* read table header */
	sread(_fnt->fd, "2W", NULL, &num_subtables);
	
	/* search for a suitable character mapping (must be unicode 2.0 - BMP
	 * only) */
	for (i = 0; i < num_subtables; i++) {
		sread(_fnt->fd, "WWD", &platform_id, &specific_id, &offset);
		
		if ((platform_id == 0 && specific_id == 3) ||
		    (platform_id == 3 && specific_id == 1)) break;
		
		/* if this is not a suitable mapping then we will set 'offset' to
		 * something irrational in order to signify that no mapping has
		 * been found (zero wil suffice as at least four bytes are needed
		 * for the table header). */
		offset = 0;
	}
	
	/* if no suitable matching was found, return -1 to indicate that an
	 * error has occurred. */
	if (offset == 0) return -1;
	
	lseek(_fnt->fd, _offset + offset, SEEK_SET);
	
	/* read subtable */
	sread(_fnt->fd, "WW2W222", &format, &length, NULL, &segcount, NULL,
		NULL, NULL);
	
	/* only a format 4 mapping is supported */
	if (format != 4) return -1;
	
	/* table stores twice the number of segments, so it needs to
	 * be divided by 2 to get the actual value. */
	segcount /= 2;
	
	/* read end codes */
	end_codes = smalloc(sizeof(UINT16) * segcount);
	sreadn(_fnt->fd, 'W', segcount, end_codes);
	
	/* skip 'reserved padding' */
	sread(_fnt->fd, "2", NULL);
	
	/* read start codes */
	start_codes = smalloc(sizeof(UINT16) * segcount);
	sreadn(_fnt->fd, 'W', segcount, start_codes);
	
	/* read deltas */
	deltas = smalloc(sizeof(UINT16) * segcount);
	sreadn(_fnt->fd, 'W', segcount, deltas);
	
	/* read offsets */
	offsets = smalloc(sizeof(UINT16) * segcount);
	sreadn(_fnt->fd, 'W', segcount, offsets);
	
	_fnt->cmap.segcount = segcount;
	_fnt->cmap.start_codes = start_codes;
	_fnt->cmap.end_codes = end_codes;
	_fnt->cmap.offsets = offsets;
	_fnt->cmap.deltas = deltas;
	
	return 0;
}

int read_loca(struct font* _fnt, off_t _offset) {
	void* offsets;
	
	lseek(_fnt->fd, _offset, SEEK_SET);
	
	/* long form of loca */
	if (_fnt->cmap.loca == 1) {
		offsets = smalloc(sizeof(UINT32) * (_fnt->max.glyphs + 1));
		sreadn(_fnt->fd, 'D', _fnt->max.glyphs + 1, offsets);
		
		_fnt->cmap.locate.f1 = offsets;
		return 0;
	}
	
	/* short form of loca */
	if (_fnt->cmap.loca == 0) {
		offsets = smalloc(sizeof(UINT16) * (_fnt->max.glyphs + 1));
		sreadn(_fnt->fd, 'W', _fnt->max.glyphs + 1, offsets);
		
		_fnt->cmap.locate.f0 = offsets;
		return 0;
	}
	
	return -1;
}

int read_maxp(struct font* _fnt, off_t _offset) {
	UINT16 glyphs;  	/* number of lgyphs in font */
	UINT16 points;  	/* maximum number of points in a single glyph */
	UINT16 contours;	/* maximum number of contours in a single glyph */
	
	UINT16 twilight;        	/* maximum number of points in the twilight */
	                        	/*   zone (zone 0). */
	UINT16 storage;         	/* maximum number of storage locations used */
	UINT16 function_defs;   	/* number of functions defined in font */
	UINT16 instruction_defs;	/* number of instructions overridden */
	UINT16 stack_depth;     	/* maximum stack depth */
	UINT16 instructions;    	/* maximum number of instructions in font */
	                        	/*   programs */
	
	lseek(_fnt->fd, _offset, SEEK_SET);
	
	/* read table */
	sread(_fnt->fd, "4WWW222WWWWWW22",
		NULL,
		&glyphs,
		&points,
		&contours,
		NULL,
		NULL,
		NULL,
		&twilight,
		&storage,
		&function_defs,
		&instruction_defs,
		&stack_depth,
		&instructions,
		NULL,
		NULL
	);
	
	_fnt->max.glyphs = glyphs;
	_fnt->max.points = points;
	_fnt->max.contours = contours;
	_fnt->max.instructions = instructions;
	_fnt->max.twilight = twilight;
	_fnt->max.storage = storage;
	_fnt->max.function_defs = function_defs;
	_fnt->max.instruction_defs = instruction_defs;
	
	return 0;
}

int read_directory(struct font* _fnt) {
	int err = 0;
	int i;
	
	/* keep track of table offsets */
	static off_t table_offsets[INDEX_COUNT] = {0};
	
	UINT32 format;    	/* PostScript = 0x74797031 (ascii 'typ1') */
	                  	/* OpenType   = 0x4F54544F (ascii 'OTTO') */
	                  	/* TrueType   = 0x74727565 (ascii '1') */
	                  	/*           or 0x00010000 */
	UINT16 num_tables;	/* number of tables in directory */
	
	UINT32 tag;   	/* table identifier (e.g. 'cmap', 'glyf', etc.) */
	UINT32 offset;	/* offset from start of file to table */
	UINT32 length;	/* length of table */
	
	/* read header */
	sread(_fnt->fd, "DW222", &format, &num_tables, NULL, NULL, NULL);
	
	/* locate tables */
	for (i = 0; i < num_tables; i++) {
		sread(_fnt->fd, "D4DD", &tag, NULL, &offset, &length);
		
		switch (tag) {
			case TAG_HEAD: table_offsets[INDEX_HEAD] = offset; break;
			case TAG_MAXP: table_offsets[INDEX_MAXP] = offset; break;
			case TAG_LOCA: table_offsets[INDEX_LOCA] = offset; break;
			case TAG_CMAP: table_offsets[INDEX_CMAP] = offset; break;
			case TAG_NAME: table_offsets[INDEX_NAME] = offset; break;
			case TAG_GLYF: table_offsets[INDEX_GLYF] = offset; break;
		}
	}
	
	_fnt->cmap.glyf_start = table_offsets[INDEX_GLYF];
	
	/* process entries... */
	err += read_head(_fnt, table_offsets[INDEX_HEAD]);
	err += read_maxp(_fnt, table_offsets[INDEX_MAXP]);
	err += read_loca(_fnt, table_offsets[INDEX_LOCA]);
	err += read_cmap(_fnt, table_offsets[INDEX_CMAP]);
	err += read_name(_fnt, table_offsets[INDEX_NAME]);
	
	if (err) XERR("failed to parse font!", EXIT_FAILURE);
	
	return 0;
}

struct font* load_font(char* _fp) {
	struct font* fnt;
	
	fnt = calloc(sizeof(struct font), 1);
	if (fnt == NULL) XERR(XEMSG_NOMEM, ENOMEM);
	
	fnt->fd = open(_fp, O_RDONLY);
	
	read_directory(fnt);
	
	/* allocate twilight zone */
	fnt->vm.zone[0] = smalloc(sizeof(struct point) * fnt->max.twilight);
	
	/* set default point size */
	fnt->point_size = 12;
	fnt->ppem = (fnt->point_size * MONITOR_DPI) / 72;
	
	return fnt;
}

#include "linuxfb.h"
#include "lfb2d.h"

static UINT16 __get_glyph_index(struct font* _fnt, UINT16 _code) {
	INT32 match = -1;
	UINT16* pnt;
	int i;
	
	/* locate range containing _gid */
	for (i = 0; i < _fnt->cmap.segcount; i++) {
		if (_fnt->cmap.end_codes[i] > _code) {
			match = i;
			break;
		}
	}
	
	/* if no match was found, return index of NULL glyph (0). */
	if (match == -1) return 0;
	
	/* search for code (just following specification) */
	if (_fnt->cmap.start_codes[match] < _code) {
		if (_fnt->cmap.offsets[match] == 0) {
			return _code + _fnt->cmap.deltas[match];
		} else {
			pnt = _fnt->cmap.offsets + match + (_fnt->cmap.offsets[match] / 2);
			pnt += _code - _fnt->cmap.start_codes[match];
			
			if (*pnt == 0) return 0;
			return *pnt + _fnt->cmap.deltas[match];
		}
	}
	
	return 0;
}

static int __process_glyf(struct font* _fnt, off_t _offset) {
	struct point* zone;
	int i, j;
	long n;
	
	FUNIT coord;
	
	INT16 num_contours;
	FWORD x_min;	/* minimum x for all glyph bounding boxes */
	FWORD y_min;	/* minimum y for all glyph bounding boxes */
	FWORD x_max;	/* maximum x for all glyph bounding boxes */
	FWORD y_max;	/* maximum y for all glyph bounding boxes */
	
	UINT16* endpoints; 	/* list of contour endpoints (point indices) */
	UINT16  num_points;	/* number of points in glyph */
	UINT16  num_instructions; /* number of instructions for glyph */
	BYTE* instructions;
	
	
	lseek(_fnt->fd, _offset + _fnt->cmap.glyf_start, SEEK_SET);
	
	sread(_fnt->fd, "WWWWW", &num_contours, &x_min, &y_min, &x_max, &y_max);
	
	/* if the number of contours is negative then this is a compoud glyph,
	 * otherwise it is a regular glyph with that many contours. */
	if (num_contours >= 0) {
		/* read contour endpoints */
		endpoints = smalloc(sizeof(UINT16) * num_contours);
		sreadn(_fnt->fd, 'W', num_contours, endpoints);
		
		/* number of points is index of last endpoint + 1 */
		num_points = endpoints[num_contours - 1] + 1;
		
		/* read instructions */
		num_instructions = sreadW(_fnt->fd);
		
		instructions = smalloc(num_instructions);
		sreadn(_fnt->fd, 'B', num_instructions, instructions);
		
		/* allocate memory for points */
		zone = smalloc(sizeof(struct point) * num_points);
		
		/* read point flags */
		for (j = 0; j < num_points; j++) {
			zone[j].flags = sreadB(_fnt->fd);
			
			/* if the repeat bit is set, the next bytes determines how
			 * many times this same flag is to be repeated. */
			if (zone[j].flags & POINT_MASK_REPEATS)
				for (n = sreadB(_fnt->fd); n > 0; n--, j++)
					zone[j + 1].flags = zone[j].flags;
		}
		
		/* read point x coordinates */
		coord = 0;
		
		for (j = 0; j < num_points; j++) {
			if (zone[j].flags & POINT_MASK_X_SHORT) {
				/* if X_SHORT bit is set, x coordinate is one byte and its sign
				 * depends on the PX_SAME bit, one for positive or zero for
				 * negative. */
				 
				/* read coordinate */
				n = sreadB(_fnt->fd);
				coord += (zone[j].flags & POINT_MASK_PX_SAME) ? n : -n;
			} else {
				/* if X_SHORT bit is not set and PX_SAME is set, the x
				 * coordinate is the same as the previous x coordinate,
				 * or if PX_SAME is also not set, the x coordinate is a
				 * two bytes signed integer. */
				
				if ( !(zone[j].flags & POINT_MASK_PX_SAME) )
					coord += sreadW(_fnt->fd);
			}
			
			zone[j].pos.x = coord;
		}
		
		/* read point y coordinates */
		coord = 0;
		
		for (j = 0; j < num_points; j++) {
			if (zone[j].flags & POINT_MASK_Y_SHORT) {
				/* if Y_SHORT bit is set, y coordinate is one byte and its sign
				 * depends on the PY_SAME bit, one for positive or zero for
				 * negative. */
				 
				/* read coordinate */
				n = sreadB(_fnt->fd);
				coord += (zone[j].flags & POINT_MASK_PY_SAME) ? n : -n;
			} else {
				/* if Y_SHORT bit is not set and PY_SAME is set, the y
				 * coordinate is the same as the previous y coordinate,
				 * or if PY_SAME is also not set, the y coordinate is a
				 * two bytes signed integer. */
				
				if ( !(zone[j].flags & POINT_MASK_PY_SAME) )
					coord += sreadW(_fnt->fd);
			}
			
			zone[j].pos.y = coord;
		}
		
		_fnt->zone = zone;
	}
	
	fb_init("/dev/fb0");
	
	for (i = 0; i < num_points; i++)
		fb_draw_point((rgbx32) {255,255,0}, zone[i].pos.x / 4 + 600, -zone[i].pos.y / 4 + 600);
	
	fb_copy();
	
	/* at this point all glyph data is ready and we can begin to execute
	 * instructions. */
	
	_fnt->vm.zone[1] = _fnt->zone;
	
	_fnt->vm.stream_length = num_instructions;
	_fnt->vm.stream = instructions;
	
	_fnt->vm.reg[EP ] = 1;
	_fnt->vm.reg[LP ] = 0;
	_fnt->vm.reg[SP ] = 3;
	_fnt->vm.reg[RP0] = 0;
	_fnt->vm.reg[RP1] = 0;
	_fnt->vm.reg[RP2] = 0;
	_fnt->vm.reg[ZP0] = 1;
	_fnt->vm.reg[ZP1] = 1;
	_fnt->vm.reg[ZP2] = 1;
	
	_fnt->vm.graphics_state = (struct graphics_state) {
		{16384, 0}, {16384, 0}, {16384, 0},
		1, 0, 0, 0 , 0, 0, 0, {0, 0, 1}
	};
	
	_fnt->max.storage = 1024;
	_fnt->vm.memory = calloc(sizeof(int32_t), _fnt->max.storage);
	
	_fnt->max.cvt_entries = 1024;
	_fnt->vm.cvt = calloc(sizeof(int32_t), _fnt->max.cvt_entries);
	
	for (i = 0; i < num_instructions; i++)
		printf("0x%02X: %s\n", instructions[i], command_names[instructions[i]]);
	
	return 0;
}

int request_glyph(struct font* _fnt, UINT16 _code) {
	UINT16 index = __get_glyph_index(_fnt, _code);
	UINT16 offset = (_fnt->cmap.loca) ?
		_fnt->cmap.locate.f1[index] :
		_fnt->cmap.locate.f0[index] * 2;
	
	__process_glyf(_fnt, offset);
	
	return 0;
}

int main(int argc, char** argv) {
	struct font* lm = load_font("/usr/share/fonts/liberation/LiberationMono-Regular.ttf");
	request_glyph(lm, '?');
	printf("%s\n", lm->name);
	
	return EXIT_SUCCESS;
}
