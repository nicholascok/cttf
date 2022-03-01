#include "specific.h"
#include "error.h"

#include "truetype_parser.h"
#include "truetype_virtual.h"

static int read_name(struct font* _fnt, struct table_key _tbl) {
	int i;
	
	UINT16 format; 	/* table format */
	UINT16 count;  	/* number of name records */
	UINT16 dataoff;	/* offset to beginning of data */
	
	UINT16 nameid; 	/* type of data stored here (name = 4) */
	UINT16 length; 	/* length of string */
	UINT16 nameoff;	/* offset to beginning of string */
	
	lseek(_fnt->fd, _tbl.offset, SEEK_SET);
	
	/* read table header */
	sread(_fnt->fd, "WWW", &format, &count, &dataoff);
	
	/* only format 0 is supported (only checking because OpenType has
	 * another format specified). */
	if (format != 0) return -1;
	
	/* loop through as may as 'count' namerecords */
	for (i = 0; i < count; i++) {
		/* ignore all fields except nameid, length, and offset.
		 * search for an entry with nameid 4 to signify the font
		 * name (this is all we care about, for now). */
		sread(_fnt->fd, "222WWW", NULL, NULL, NULL, &nameid,
			&length, &nameoff);
		
		if (nameid == 4) {
			lseek(_fnt->fd, _tbl.offset + dataoff + nameoff, SEEK_SET);
			read(_fnt->fd, _fnt->name, (length > 255) ? 255 : length);
			
			return 0;
		}
	}
	
	return -1;
}

static int read_head(struct font* _fnt, struct table_key _tbl) {
	UINT16 flags;	/* font flags */
	UINT16 fupem;	/* units per em */
	
	INT16 loca_format;
	
	lseek(_fnt->fd, _tbl.offset, SEEK_SET);
	
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
	
	_fnt->_cmap.loca = loca_format;
	_fnt->fupem = fupem;
	_fnt->flags = flags;
	
	return 0;
}

static int read_cmap(struct font* _fnt, struct table_key _tbl) {
	int i;
	
	UINT16 num_subtables;	/* number of encoding subtables present */
	
	/* note: for platform_id and specific_id, only combinations (0, 3)
	 * or (3, 1) are supported (these say that we have support for
	 * unicode 2.0 with the basic multilingual plane (BMP) only). */
	 
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
	
	lseek(_fnt->fd, _tbl.offset, SEEK_SET);
	
	/* read table header */
	sread(_fnt->fd, "2W", NULL, &num_subtables);
	
	/* search for a suitable character mapping (must be unicode
	 * 2.0 - BMP only) */
	for (i = 0; i < num_subtables; i++) {
		sread(_fnt->fd, "WWD", &platform_id, &specific_id, &offset);
		
		if ((platform_id == 0 && specific_id == 3) ||
		    (platform_id == 3 && specific_id == 1)) break;
		
		/* if this is not a suitable mapping then we will set
		 * 'offset' to something irrational in order to signify
		 * that no mapping has been found (zero wil suffice as at
		 * least four bytes are needed for the table header). */
		offset = 0;
	}
	
	/* if no suitable matching was found, return -1 to indicate that an
	 * error has occurred. */
	if (offset == 0) return -1;
	
	lseek(_fnt->fd, _tbl.offset + offset, SEEK_SET);
	
	/* read subtable */
	sread(_fnt->fd, "WW2W222", &format, &length, NULL, &segcount, NULL,
		NULL, NULL);
	
	/* only a format 4 mapping is supported */
	if (format != 4) return -1;
	
	/* table stores twice the number of segments, so it needs to
	 * be divided by 2 to get the actual value. */
	segcount /= 2;
	
	/* read end codes */
	end_codes = smalloc(sizeof(UINT16), segcount);
	sreadn(_fnt->fd, 'W', segcount, end_codes);
	
	/* skip 'reserved padding' */
	sread(_fnt->fd, "2", NULL);
	
	/* read start codes */
	start_codes = smalloc(sizeof(UINT16), segcount);
	sreadn(_fnt->fd, 'W', segcount, start_codes);
	
	/* read deltas */
	deltas = smalloc(sizeof(UINT16), segcount);
	sreadn(_fnt->fd, 'W', segcount, deltas);
	
	/* read offsets */
	offsets = smalloc(sizeof(UINT16), segcount);
	sreadn(_fnt->fd, 'W', segcount, offsets);
	
	_fnt->_cmap.segcount = segcount;
	_fnt->_cmap.start_codes = start_codes;
	_fnt->_cmap.end_codes = end_codes;
	_fnt->_cmap.offsets = offsets;
	_fnt->_cmap.deltas = deltas;
	
	return 0;
}

static int read_loca(struct font* _fnt, struct table_key _tbl) {
	UINT32 length;	/* length of loca table */
	void* offsets;	/* pointer to loca data (glyph offsets) */
	
	lseek(_fnt->fd, _tbl.offset, SEEK_SET);
	
	/* long form of loca */
	if (_fnt->_cmap.loca == 1) {
		length = _tbl.length / sizeof(UINT32);
		offsets = smalloc(sizeof(UINT32), length);
		
		sreadn(_fnt->fd, 'D', length, offsets);
		_fnt->_cmap.locate.f1 = offsets;
		
		return 0;
	}
	
	/* short form of loca */
	if (_fnt->_cmap.loca == 0) {
		length = _tbl.length / sizeof(UINT16);
		offsets = smalloc(sizeof(UINT16), length);
		
		sreadn(_fnt->fd, 'W', length, offsets);
		_fnt->_cmap.locate.f0 = offsets;
		
		return 0;
	}
	
	return -1;
}

static int read_maxp(struct font* _fnt, struct table_key _tbl) {
	UINT16 glyphs;  	/* number of glyphs in font */
	UINT16 points;  	/* maximum points in a single glyph */
	UINT16 twilight;	/* maximum points in the twilight zone */
	UINT16 contours;	/* maximum contours in a single glyph */
	
	UINT16 storage;         	/* maximum storage locations used */
	UINT16 function_defs;   	/* number of functions defined */
	UINT16 instruction_defs;	/* number of instructions overridden */
	UINT16 stack_depth;     	/* maximum stack depth */
	UINT16 instructions;    	/* maximum number of instructions */
	                        	/*   in font programs */
	
	lseek(_fnt->fd, _tbl.offset, SEEK_SET);
	
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
	_fnt->max.stack_depth = stack_depth;
	
	return 0;
}

static int read_cvt_(struct font* _fnt, struct table_key _tbl) {
	UINT32 control_values = _tbl.length / sizeof(INT16);
	FWORD* cvt = smalloc(sizeof(FWORD), control_values);
	
	lseek(_fnt->fd, _tbl.offset, SEEK_SET);
	
	/* read table */
	sreadn(_fnt->fd, 'W', control_values, cvt);
	
	_fnt->max.control_values = control_values;
	_fnt->cvt = smalloc(sizeof(INT32), _fnt->max.control_values + 1);
	_fnt->unscaled_cvt = cvt;
	
	return 0;
}

static int read_prep(struct font* _fnt, struct table_key _tbl) {
	BYTE* stream = smalloc(sizeof(BYTE), _tbl.length);
	
	lseek(_fnt->fd, _tbl.offset, SEEK_SET);
	
	/* read table */
	sreadn(_fnt->fd, 'B', _tbl.length, stream);
	
	_fnt->prep_program.length = _tbl.length;
	_fnt->prep_program.stream = stream;
	
	return 0;
}

static int read_fpgm(struct font* _fnt, struct table_key _tbl) {
	struct vm_state state;
	struct vm* virtm;
	
	BYTE* stream = smalloc(sizeof(BYTE), _tbl.length);
	
	lseek(_fnt->fd, _tbl.offset, SEEK_SET);
	
	/* read table */
	sreadn(_fnt->fd, 'B', _tbl.length, stream);
	
	/* at this point the 'maxp' and 'cvt ' tables should have already
	 * been processed, so we will run the program now. */
	
	_fnt->functions = scalloc(sizeof(struct proceedure),
		_fnt->max.function_defs);
	
	_fnt->userdefs = scalloc(sizeof(struct proceedure),
		_fnt->max.instruction_defs);
	
	_fnt->storage = scalloc(sizeof(UINT32), _fnt->max.storage);
	
	state.num_instructions = _tbl.length;
	state.instructions = stream;
	state.glyph = NULL;
	
	printf("...%u\n", _tbl.length);
	virtm = make_virtual(_fnt);
	
	init_virtual(virtm, state);
	exec_virtual(virtm, 1);
	
	memcpy(_fnt->functions, virtm->functions,
		sizeof(struct proceedure) * _fnt->max.function_defs);
	
	memcpy(_fnt->userdefs, virtm->userdefs,
		sizeof(struct proceedure) * _fnt->max.instruction_defs);
	
	free_virtual(virtm);
	
	return 0;
}

int read_directory(struct font* _fnt) {
	int err = 0;
	int i;
	
	/* we will keep track of table offsets and lengths. */
	static struct table_key table_info[INDEX_COUNT] = {0};
	
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
			case TAG_FPGM:
				table_info[INDEX_FPGM].offset = offset;
				table_info[INDEX_FPGM].length = length;
				break;
			case TAG_PREP:
				table_info[INDEX_PREP].offset = offset;
				table_info[INDEX_PREP].length = length;
				break;
			case TAG_CVT_:
				table_info[INDEX_CVT_].offset = offset;
				table_info[INDEX_CVT_].length = length;
				break;
			case TAG_HEAD:
				table_info[INDEX_HEAD].offset = offset;
				table_info[INDEX_HEAD].length = length;
				break;
			case TAG_MAXP:
				table_info[INDEX_MAXP].offset = offset;
				table_info[INDEX_MAXP].length = length;
				break;
			case TAG_LOCA:
				table_info[INDEX_LOCA].offset = offset;
				table_info[INDEX_LOCA].length = length;
				break;
			case TAG_CMAP:
				table_info[INDEX_CMAP].offset = offset;
				table_info[INDEX_CMAP].length = length;
				break;
			case TAG_NAME:
				table_info[INDEX_NAME].offset = offset;
				table_info[INDEX_NAME].length = length;
				break;
			case TAG_GLYF:
				table_info[INDEX_GLYF].offset = offset;
				table_info[INDEX_GLYF].length = length;
				break;
		}
	}
	
	_fnt->_cmap.glyf_table_offset = table_info[INDEX_GLYF].offset;
	
	/* process entries in the correct order... */
	err += read_head(_fnt, table_info[INDEX_HEAD]);
	err += read_maxp(_fnt, table_info[INDEX_MAXP]);
	err += read_loca(_fnt, table_info[INDEX_LOCA]);
	err += read_cmap(_fnt, table_info[INDEX_CMAP]);
	err += read_cvt_(_fnt, table_info[INDEX_CVT_]);
	err += read_prep(_fnt, table_info[INDEX_PREP]);
	err += read_fpgm(_fnt, table_info[INDEX_FPGM]);
	err += read_name(_fnt, table_info[INDEX_NAME]);
	
	/* if we fail to parse the font, we will just exit for now...
	 * this is only temporary, I'm just too lazy to deal with cleanup. */
	if (err) XERR("failed to parse font!", EXIT_FAILURE);
	
	return 0;
}
