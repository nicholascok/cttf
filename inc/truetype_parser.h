#ifndef __TRUETYPE_PARSER_H__
#define __TRUETYPE_PARSER_H__

#include "truetype.h"

/* 
 * font file types... (only FORMAT_TRUETYPE / FORMAT_TRUETYPE_B
 * are currently supported)
 */
#define FORMAT_TRUETYPE   0x74727565
#define FORMAT_TRUETYPE_B 0x00010000
#define FORMAT_OPENTYPE   0x4F54544F
#define FORMAT_POSTSCRIPT 0x74797031

/* 
 * table identifiers (tags)...
 */
#define TAG_CMAP 0x636D6170
#define TAG_GLYF 0x676C7966
#define TAG_HEAD 0x68656164
#define TAG_HHEA 0x68686561
#define TAG_HMTX 0x686D7478
#define TAG_LOCA 0x6C6F6361
#define TAG_MAXP 0x6D617870
#define TAG_NAME 0x6E616D65
#define TAG_CVT_ 0x63767420
#define TAG_PREP 0x70726570
#define TAG_FPGM 0x6670676D

/* 
 * table indices - these are used to control the order that tables are
 * processed.
 */
enum {
	INDEX_HEAD,
	INDEX_MAXP,
	INDEX_LOCA,
	INDEX_CMAP,
	INDEX_CVT_,
	INDEX_PREP,
	INDEX_FPGM,
	INDEX_GLYF,
	INDEX_NAME,
	
	INDEX_COUNT
};

/* 
 * holds information contained in a font directory table entry. 
 */
struct table_key {
	UINT32 offset;
	UINT32 length;
};

int read_directory(struct font* _fnt);

#endif /* __TRUETYPE_PARSER_H__ */
