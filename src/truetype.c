#include "error.h"
#include "specific.h"

#include "truetype.h"
#include "truetype_parser.h"
#include "truetype_virtual.h"

int MONITOR_DPI = 96;

float dot2f(struct vec2f _v1, struct vec2f _v2) {
	return _v1.x * _v2.x + _v1.y * _v2.y;
}

float fisqrt(float _x) {
	union { uint32_t u; float f; } x;
	x.f = _x;
	
	x.u = (0x5F400000UL - x.u) >> 1;
	
	x.f *= (1.5 - (_x * 0.5 * x.f * x.f));
	x.f *= (1.5 - (_x * 0.5 * x.f * x.f));
	x.f *= (1.5 - (_x * 0.5 * x.f * x.f));
	x.f *= (1.5 - (_x * 0.5 * x.f * x.f));
	
	return x.f;
}

int set_font_size(struct font* _fnt, int _points) {
	struct vm_state state;
	struct vm* virtm;
	
	UINT32 i;
	
	/* at this point the 'maxp' and 'cvt ' tables should have already
	 * been processed and the font program should have been executed. */
	
	_fnt->point_size = _points;
	_fnt->ppem = FROUND((_fnt->point_size * MONITOR_DPI) / 72.0);
	
	/* if we do not have a prep program, return. note that this is
	 * not errorneous as some fonts are without hinting. */
	if (!_fnt->prep_program.stream) return 0;
	
	state.num_instructions = _fnt->prep_program.length;
	state.instructions = _fnt->prep_program.stream;
	
	state.glyph = NULL;
	
	_fnt->cvt[0] = 0;
	for (i = 0; i < _fnt->max.control_values; i++) {
		_fnt->cvt[i + 1] = _fnt->unscaled_cvt[i];
		_fnt->cvt[i + 1] *= _fnt->point_size;
		_fnt->cvt[i + 1] *= MONITOR_DPI * 64;
		_fnt->cvt[i + 1] /= _fnt->fupem * 72;
	}
	
	virtm = make_virtual(_fnt);
	init_virtual(virtm, state);
	exec_virtual(virtm, 1);
	free_virtual(virtm);
	
	return 0;
}

struct font* load_font(char* _fp) {
	struct font* fnt = scalloc(sizeof(struct font), 1);
	
	fnt->fd = open(_fp, O_RDONLY);
	read_directory(fnt);
	
	/* set default point size */
	set_font_size(fnt, DEFAULT_POINT_SIZE);
	
	return fnt;
}

#include "truetype_virtual.h"

int read_glyph(struct font* _fnt, off_t _offset) {
	struct vm* virtm;
	
	struct vm_state state;
	struct glyph glyph;
	FUNIT coord;
	
	UINT8 b;
	UINT32 i, j;
	
	FWORD x_min;	/* minimum x for glyph bounding box */
	FWORD y_min;	/* minimum y for glyph bounding box */
	FWORD x_max;	/* maximum x for glyph bounding box */
	FWORD y_max;	/* maximum y for glyph bounding box */
	
	lseek(_fnt->fd, _offset + _fnt->_cmap.glyf_table_offset, SEEK_SET);
	
	sread(_fnt->fd, "WWWWW", &glyph.num_contours, &x_min, &y_min,
		&x_max, &y_max);
	
	/* if the number of contours is negative then this is a compoud glyph,
	 * otherwise it is a regular glyph with that many contours. */
	if (glyph.num_contours >= 0) {
		/* read contour endpoints */
		glyph.contours = smalloc(sizeof(INT32), glyph.num_contours + 1);
		
		for (j = 0; j < (UINT32) glyph.num_contours; j++)
			glyph.contours[j + 1] = sreadW(_fnt->fd);
		
		/* set index zero to 0 (its easier to lookup the start and
		 * end of a contour this way). */
		glyph.contours[0] = -1;
		
		/* number of points is index of last endpoint + 1 */
		glyph.num_points = glyph.contours[glyph.num_contours] + 1;
		
		/* read instructions */
		state.num_instructions = sreadW(_fnt->fd);
		
		state.instructions = smalloc(sizeof(BYTE), state.num_instructions);
		sreadn(_fnt->fd, 'B', state.num_instructions, state.instructions);
		
		/* allocate memory for points (w eadd four to make room for
		 * phantom points). */
		glyph.points = smalloc(sizeof(struct point), glyph.num_points + 4);
		
		/* read point flags */
		for (i = 0; i < glyph.num_points; i++) {
			glyph.points[i].flags = sreadB(_fnt->fd);
			
			/* if the repeat bit is set, the next bytes
			 * determines how many times this same flag is to
			 * be repeated. */
			if (glyph.points[i].flags & POINT_MASK_REPEATS)
				for (b = sreadB(_fnt->fd); b > 0; b--, i++)
					glyph.points[i + 1].flags =
						glyph.points[i].flags;
		}
		
		/* note that coordinates are stored relative in the
		 * following sense: if the first value read is 15 and the
		 * second is 20, then point 1 will have an x coordinate of
		 * 15 and point 2 will  have an x coordinate of 35.
		 * similarly, if the next value read is -5, point 3 will
		 * have an x coordinate of 30. */
		
		/* read point x coordinates (coord is initially 0) */
		coord = 0;
		
		for (i = 0; i < glyph.num_points; i++) {
			if (glyph.points[i].flags & POINT_MASK_X_SHORT) {
				/* if X_SHORT bit is set, x coordinate is
				 * one byte and its sign depends on the
				 * PX_SAME bit, one for positive or zero
				 * for negative. */
				 
				/* read coordinate */
				b = sreadB(_fnt->fd);
				coord += (glyph.points[i].flags &
					POINT_MASK_PX_SAME) ? b : -b;
			} else {
				/* if X_SHORT bit is not set and PX_SAME is
				 * set, the x coordinate is the same as the
				 * previous x coordinate, or if PX_SAME is
				 * also not set, the x coordinate is a two
				 * byte signed integer. */
				
				if ( !(glyph.points[i].flags &
				    POINT_MASK_PX_SAME) )
					coord += sreadW(_fnt->fd);
			}
			
			/* set coordinate and convert units to pixels. */
			glyph.points[i].pos.x = coord;
			glyph.points[i].pos.x *= _fnt->point_size;
			glyph.points[i].pos.x *= MONITOR_DPI * 64;
			glyph.points[i].pos.x /= _fnt->fupem * 72;
		}
		
		/* read point y coordinates (coord is initially 0) */
		coord = 0;
		
		for (i = 0; i < glyph.num_points; i++) {
			if (glyph.points[i].flags & POINT_MASK_Y_SHORT) {
				/* if Y_SHORT bit is set, y coordinate is
				 * one byte and its sign depends on the
				 * PY_SAME bit, one for positive or zero
				 * for negative. */
				 
				/* read coordinate */
				b = sreadB(_fnt->fd);
				coord += (glyph.points[i].flags &
					POINT_MASK_PY_SAME) ?  b : -b;
			} else {
				/* if Y_SHORT bit is not set and PY_SAME is
				 * set, the y coordinate is the same as the
				 * previous y coordinate, or if PY_SAME is
				 * also not set, the y coordinate is a two
				 * byte signed integer. */
				
				if ( !(glyph.points[i].flags &
				    POINT_MASK_PY_SAME) )
					coord += sreadW(_fnt->fd);
			}
			
			/* set coordinate and convert units to pixels. */
			glyph.points[i].pos.y = coord;
			glyph.points[i].pos.y *= _fnt->point_size;
			glyph.points[i].pos.y *= MONITOR_DPI * 64;
			glyph.points[i].pos.y /= _fnt->fupem * 72;
			
			/* we are done with the POINT_MASK_X_SHORT and
			 * POINT_MASK_Y_SHORT bits, so they are repurposed
			 * to hold the touch state of a point. these will
			 * be initialised to zeros. */
			glyph.points[i].flags &= ~POINT_MASK_TOUCHED;
		}
		
		/* add four phantom points! */
		glyph.num_points += 4;
		
		glyph.points[glyph.num_points - 4].pos.x = x_min - 64;
		glyph.points[glyph.num_points - 4].pos.y = 0;
		glyph.points[glyph.num_points - 3].pos.x = x_max + 64;
		glyph.points[glyph.num_points - 3].pos.y = 0;
		
		glyph.points[glyph.num_points - 2].pos.x = x_min - 64;
		glyph.points[glyph.num_points - 2].pos.y = y_max + 10;
		glyph.points[glyph.num_points - 1].pos.x = x_max + 64;
		glyph.points[glyph.num_points - 1].pos.y = y_max + 10;
	}
	
	/* attatch glyph to virtual machine state... */
	state.glyph = &glyph;
	
	/* at this point all glyph data is ready and we can begin to execute
	 * instructions. */
	
	virtm = make_virtual(_fnt);
	init_virtual(virtm, state);
	exec_virtual(virtm,1);
	
	free_virtual(virtm);
	
	return 0;
}

static UINT16 __get_glyph_index(struct font* _fnt, UINT16 _code) {
	INT32 match = -1;
	UINT16* pnt;
	int i;
	
	/* locate range containing _gid */
	for (i = 0; i < _fnt->_cmap.segcount; i++) {
		if (_fnt->_cmap.end_codes[i] > _code) {
			match = i;
			break;
		}
	}
	
	/* if no match was found, return index of NULL glyph (0). */
	if (match == -1) return 0;
	
	/* search for code */
	if (_fnt->_cmap.start_codes[match] < _code) {
		if (_fnt->_cmap.offsets[match] == 0) {
			return _code + _fnt->_cmap.deltas[match];
		} else {
			pnt = _fnt->_cmap.offsets + match + (_fnt->_cmap.offsets[match] / 2);
			pnt += _code - _fnt->_cmap.start_codes[match];
			
			if (*pnt == 0) return 0;
			return *pnt + _fnt->_cmap.deltas[match];
		}
	}
	
	return 0;
}

int request_glyph(struct font* _fnt, UINT16 _code) {
	UINT16 index = __get_glyph_index(_fnt, _code);
	UINT16 offset = (_fnt->_cmap.loca) ?
		_fnt->_cmap.locate.f1[index] :
		_fnt->_cmap.locate.f0[index] * 2;
	
	read_glyph(_fnt, offset);
	
	return 0;
}

int main(int argc, char** argv) {
	/* /home/nicholas/Downloads/OpenSans-Regular.ttf */
	struct font* lm = load_font("/usr/share/fonts/liberation/LiberationMono-Italic.ttf");
	/* /usr/share/fonts/liberation/LiberationMono-Italic.ttf */
	char k;
	printf("asfasf\n");
	for (;;k = getchar()) {
		printf("%c\n", k);
		if (k != '\n')
		request_glyph(lm, k);
	}
	
	printf("%s\n", lm->name);
	printf("%d\n", lm->ppem);
	
	return EXIT_SUCCESS;
}
