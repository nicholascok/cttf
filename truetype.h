#ifndef __TRUETYPE_H__
#define __TRUETYPE_H__

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>

/* 
 * some basic general purpose functions...
 */
#define SIGN(X) ( ((X) > 0) - ((X) < 0) )

#define ABS(X) ( ((X) < 0) ? (-(X)) : (X) )

#define SWAP32(X0, X1) {\
	(*((UINT32*) (X0))) ^= (*((UINT32*) (X1)));\
	(*((UINT32*) (X1))) ^= (*((UINT32*) (X0)));\
	(*((UINT32*) (X0))) ^= (*((UINT32*) (X1)));\
}


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

#define FORMAT_TRUETYPE   0x74727565
#define FORMAT_TRUETYPE_B 0x00010000
#define FORMAT_OPENTYPE   0x4F54544F
#define FORMAT_POSTSCRIPT 0x74797031

/* 
 * macros for operating with 26.6 fixed point numbers.
 */
#define F26D6_FRAC_MASK  ( (UINT32) (0x0000003F) )
#define F26D6MUL(X0, X1) ( (((INT64) (X0)) * ((INT64) (X1))) >> 6 )
#define F26D6DIV(X0, X1) ( (((INT64) (X0)) << 6) / ((INT64) (X1)) )

/* 
 * since we are not using a printer, we have no engine compensation.
 */
#define ENGINE_COMPENSATION ( 0 )

/* 
 * point flag masks.
 */
#define POINT_MASK_ONCURVE (0x01)
#define POINT_MASK_X_SHORT (0x02)
#define POINT_MASK_Y_SHORT (0x04)
#define POINT_MASK_REPEATS (0x08)
#define POINT_MASK_PX_SAME (0x10)
#define POINT_MASK_PY_SAME (0x20)

#define POINT_MASK_XTOUCHED (0x01)
#define POINT_MASK_YTOUCHED (0x02)

/* 
 * masks and functions for setting / getting information from the
 * machine's round state varaibles.
 */
#define RSTATE_MASK_THRESHOLD ((UINT8) 0x0F)
#define RSTATE_MASK_PERIOD    ((UINT8) 0xC0)
#define RSTATE_MASK_PHASE     ((UINT8) 0x30)

#define RSTATE_THRESHOLD ( (UINT8) ((UINT8) ROUND_STATE & RSTATE_MASK_THRESHOLD) )
#define RSTATE_PERIOD    ( (UINT8) ((UINT8) ROUND_STATE & RSTATE_MASK_PERIOD) >> 6 )
#define RSTATE_PHASE     ( (UINT8) ((UINT8) ROUND_STATE & RSTATE_MASK_PHASE) >> 4 )

#define ROUND_STATE       ( this->vm.round.state)
#define ROUND_GRID_PERIOD ( this->vm.round.grid_period)
#define ROUND_THRESHOLD   ( this->vm.round.threshold)
#define ROUND_PERIOD      ( this->vm.round.period)
#define ROUND_PHASE       ( this->vm.round.phase)

/* 
 * the remaining macros here are aliases for accessing data in the virtual
 * machine, they exist purely for aesthetics.
 */
#define REG_SP    ( this->vm.reg[SP] )
#define REG_EP    ( this->vm.reg[EP] )
#define REG_LP    ( this->vm.reg[LP] )
#define REG_RP    ( this->vm.reg[RP] )

#define REG_RP0   ( this->vm.reg[RP0] )
#define REG_RP1   ( this->vm.reg[RP1] )
#define REG_RP2   ( this->vm.reg[RP2] )

#define REG_ZP0   ( this->vm.reg[ZP0] )
#define REG_ZP1   ( this->vm.reg[ZP1] )
#define REG_ZP2   ( this->vm.reg[ZP2] )

#define ZONEP0(N) ( this->vm.zone[REG_ZP0][N] )
#define ZONEP1(N) ( this->vm.zone[REG_ZP1][N] )
#define ZONEP2(N) ( this->vm.zone[REG_ZP2][N] )

#define ORZONE(N) ( this->zone[N] )

#define STACK(N)  ( this->vm.stack[REG_SP - (N) - 1] )
#define SSTACK(N) ( *((INT32*) &STACK(N)) )
#define USTACK(N) ( *((UINT32*) &STACK(N)) )

#define GRAPHICS_STATE       ( this->vm.graphics_state )
#define MAX_INSTRUCTION_DEFS ( this->max.instruction_defs )
#define MAX_FUNCTION_DEFS    ( this->max.function_defs )
#define MAX_CONTROL_VALUES   ( this->max.cvt_entries )
#define MAX_STORAGE          ( this->max.storage )

#define DPRJ_VEC  ( GRAPHICS_STATE.dual_projection_vector )
#define PRJ_VEC   ( GRAPHICS_STATE.projection_vector )
#define FRD_VEC   ( GRAPHICS_STATE.freedom_vector )

#define CONTROL_VALUE(N) ( this->vm.cvt[N] )
#define STREAM(N) ( this->vm.stream[REG_EP + (N) - 1] )
#define MEMORY(N) ( this->vm.memory[N] )

#define FUNCTION(N)      ( this->vm.functions[N] )
#define INSTRUCTION(N)   ( this->vm.instructions[N] )

#define BRANCH_DEPTH ( this->vm.branch_depth )

#define POINT_SIZE ( this->point_size )

#define PIXELSTOFU(X) ( ((X) * this->fupem) / this->ppem / 64 )

#define FUTOPIXELS(X) ( ((X) * this->ppem) / this->fupem * 64 )

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
 * registers...
 */
enum {
	EP, /* execution pointer */
	SP, /* stack pointer     */
	LP, /* loop register     */
	RP, /* return pointer    */
	
	ZP0, /* zone pointer     */
	ZP1, /* registers        */
	ZP2, /* ...              */
	
	RP0, /* reference point  */
	RP1, /* registers        */
	RP2, /* ...              */
	
	REG_MAX
};

/* 
 * instruction set...
 */
enum {
	SVTCA        = 0x00, /* (S)et (V)ectors (T)o (C)oordinate (A)xis */
	SPVTCA       = 0x02, /* (S)et (P)rojection (V)ector (T)o (C)oordinate (A)xis */
	SFVTCA       = 0x04, /* (S)et (F)reedom (V)ector (T)o (C)oordinate (A)xis */
	SPVTL        = 0x06, /* (S)et (P)rojection (V)ector (T)o (L)ine */
	SFVTL        = 0x08, /* (S)et (F)reedom (V)ector (T)o (L)ine */
	SPVFS        = 0x0A, /* (S)et (P)rojection (V)ector (F)rom (S)tack */
	SFVFS        = 0x0B, /* (S)et (F)reedom (V)ector (F)rom (S)tack */
	GPV          = 0x0C, /* (G)et (P)rojection (V)ector */
	GFV          = 0x0D, /* (G)et (F)reedom (V)ector */
	SFVTPV       = 0x0E, /* (S)et (F)reedom (V)ector (T)o (P)rojection (V)ector */
	ISECT        = 0x0F, /* move point to (I)nter(SECT)ion of lines */
	SRP          = 0x10, /* (S)et (R)eference (P)oint */
	SZP          = 0x13, /* (S)et (Z)one (P)ointer */
	SZPS         = 0x16, /* (S)et (Z)one (P)ointer register(S) */
	SLOOP        = 0x17, /* (S)et (LOOP) register */
	RTG          = 0x18, /* (R)ound (T)o (G)rid */
	RTHG         = 0x19, /* (R)ound (T)o (H)alf (G)rid */
	SMD          = 0x1A, /* (S)et (M)inimum (D)istance */
	ELSE         = 0x1B, /* (ELSE) */
	JMPR         = 0x1C, /* (J)u(MP) (R)elative */
	SCVTCI       = 0x1D, /* (S)et (C)ontrol (V)alue (T)able (C)ut (I)n */
	SSWCI        = 0x1E, /* (S)et (S)ingle (W)idth (C)ut (I)n */
	SSW          = 0x1F, /* (S)et (S)ingle (W)idth */
	DUP          = 0x20, /* (DUP)licate element at top of stack */
	POP          = 0x21, /* (POP) element at top of stack */
	CLEAR        = 0x22, /* (CLEAR) stack */
	SWAP         = 0x23, /* (SWAP) values at top of stack */
	DEPTH        = 0x24, /* push stack (DEPTH) */
	CINDEX       = 0x25, /* (C)opy (INDEX) */
	MINDEX       = 0x26, /* (M)ove (INDEX) */
	ALIGNPTS     = 0x27, /* (ALIGN) (P)oin(TS) */
	UDEF0        = 0x28,
	UTP          = 0x29, /* (U)n (T)ouch (P)oint */
	LOOPCALL     = 0x2A, /* (LOOP) (CALL) function */
	CALL         = 0x2B, /* (CALL) function */
	FDEF         = 0x2C, /* (F)unction (DEF)inition */
	ENDF         = 0x2D, /* (END) (F)unction definition */
	MDAP         = 0x2E, /*  */
	IUP          = 0x30, /* (I)nterpolate (U)ntouched (P)oints */
	SHP          = 0x32, /* (SH)ift (P)oint using reference point */
	SHC          = 0x34, /* (SH)ift (C)ontour using reference point */
	SHZ          = 0x36, /* (SH)ift (Z)one using reference point */
	SHPIX        = 0x38, /* (SH)ift (P)oints by (PIX)el amount */
	IP           = 0x39, /* (I)nterpolate (P)oints */
	MSIRP        = 0x3A, /*  */
	ALIGNRP      = 0x3C, /* (ALIGN) to (R)eference (P)oint */
	RTDG         = 0x3D, /* (R)ound (T)o (D)ouble (G)rid */
	MIAP         = 0x3E, /*  */
	NPUSHB       = 0x40, /* (PUSH) (N) (B)ytes */
	NPUSHW       = 0x41, /* (PUSH) (B) (W)ords */
	WS           = 0x42, /* (W)rite value to (S)torage */
	RS           = 0x43, /* (R)ead value from (S)torage */
	WCVTP        = 0x44, /* (W)rite (C)ontrol (V)alue (T)able in (P)ixel units */
	RCVT         = 0x45, /* (R)ead (C)ontrol (V)alue (T)able */
	GC           = 0x46, /* (G)et (C)oordinate */
	SCFS         = 0x48, /* (S)et (C)oorinate (F)rom (S)tack */
	MD           = 0x49, /* (M)easure (D)istance */
	MPPEM        = 0x4B, /* (M)easure (P)ixels (P)er (E)m */
	MPS          = 0x4C, /* (M)easure (P)oint (S)ize */
	FLIPON       = 0x4D, /* set auto (FLIP) to (ON) */
	FLIPOFF      = 0x4E, /* set auto (FLIP) to (OFF) */
	DEBUG        = 0x4F,
	LT           = 0x50, /* (L)ess (T)han */
	LTEQ         = 0x51, /* (L)ess (T)han or (EQ)ual to */
	GT           = 0x52, /* (G)reater (T)han */
	GTEQ         = 0x53, /* (G)reater (T)han or (EQ)ual to */
	EQ           = 0x54, /* (EQ)ual */
	NEQ          = 0x55, /* (N)ot (EQ)ual */
	ODD          = 0x56, /* (ODD) */
	EVEN         = 0x57, /* (EVEN) */
	IF           = 0x58, /* (IF) */
	EIF          = 0x59, /* (E)nd (IF) */
	AND          = 0x5A, /* logical (AND) */
	OR           = 0x5B, /* logical (OR) */
	NOT          = 0x5C, /* logical (NOT) */
	DELTAP1      = 0x5D, /*  */
	SDB          = 0x5E, /* (S)et (D)elta (B)ase */
	SDS          = 0x5F, /* (S)et (D)elta (S)hift */
	ADD          = 0x60, /* (ADD) */
	SUB          = 0x61, /* (SUB)tract */
	DIV          = 0x62, /* (DIV)ide */
	MUL          = 0x63, /* (MUL)tiply */
	ABS          = 0x64, /* (ABS)olute value */
	NEG          = 0x65, /* (NEG)ate */
	FLOOR        = 0x66, /* (FLOOR) */
	CEILING      = 0x67, /* (CEILING) */
	ROUND        = 0x68, /* (ROUND) */
	NROUND       = 0x6C, /* (N)o (ROUND) */
	WCVTF        = 0x70, /* (W)rite (C)ontrol (V)alue (T)able in (F)ont units */
	DELTAP2      = 0x71, /*  */
	DELTAP3      = 0x72, /*  */
	DELTAC1      = 0x73, /*  */
	DELTAC2      = 0x74, /*  */
	DELTAC3      = 0x75, /*  */
	SROUND       = 0x76, /* (S)uper (ROUND) */
	S45ROUND     = 0x77, /* (S)uper (45) degree (R)ound */
	JROT         = 0x78, /* (J)ump (R)elative (O)n (T)rue */
	JROF         = 0x79, /* (J)ump (R)elative (O)n (F)alse */
	ROFF         = 0x7A, /* (R)ound (OFF)*/
	UDEF1        = 0x7B,
	RUTG         = 0x7C, /* (R)ound (U)p (T)o (G)rid */
	RDTG         = 0x7D, /* (R)ound (D)own (T)o (G)rid */
	SANGW        = 0x7E, /*  */
	AA           = 0x7F, /* pop[ ARG ] */
	FLIPPT       = 0x80, /* (FLIP) (P)oin(T) */
	FLIPRGON     = 0x81, /* (FLIP) (R)an(G)e (ON) */
	FLIPRGOFF    = 0x82, /* (FLIP) (R)an(G)e (OFF) */
	UDEF2        = 0x83,
	UDEF3        = 0x84,
	SCANCTRL     = 0x85, /*  */
	SDPVTL       = 0x86, /* (S)et (D)ual (P)rojection (V)ector (T)o (L)ine */
	GETINFO      = 0x88, /* (GET) (INFO) */
	IDEF         = 0x89, /* (I)nstruction (DEF)inition */
	ROLL         = 0x8A, /* (ROLL) elements at top of stack */
	MAX          = 0x8B, /* (MAX)imum */
	MIN          = 0x8C, /* (MIN)imum */
	SCANTYPE     = 0x8D, /*  */
	INSTCTRL     = 0x8E, /*  */
	UDEF4        = 0x8F,
	UDEF5        = 0x90,
	GETVARIATION = 0x91, /*  */
	UDEF6        = 0x92,
	UDEF7        = 0x93,
	UDEF8        = 0x94,
	UDEF9        = 0x95,
	UDEFA        = 0x96,
	UDEFB        = 0x97,
	UDEFC        = 0x98,
	UDEFD        = 0x99,
	UDEFE        = 0x9A,
	UDEFF        = 0x9B,
	UDEFG        = 0x9C,
	UDEFH        = 0x9D,
	UDEFI        = 0x9E,
	UDEFJ        = 0x9F,
	UDEFK        = 0xA0,
	UDEFL        = 0xA1,
	UDEFM        = 0xA2,
	UDEFN        = 0xA3,
	UDEFO        = 0xA4,
	UDEFP        = 0xA5,
	UDEFQ        = 0xA6,
	UDEFR        = 0xA7,
	UDEFS        = 0xA8,
	UDEFT        = 0xA9,
	UDEFU        = 0xAA,
	UDEFV        = 0xAB,
	UDEFW        = 0xAC,
	UDEFX        = 0xAD,
	UDEFY        = 0xAE,
	UDEFZ        = 0xAF,
	PUSHB        = 0xB0, /* (PUSH) (B)ytes */
	PUSHW        = 0xB8, /* (PUSH) (W)ords */
	MDRP         = 0xC0, /*  */
	MIRP         = 0xE0, /*  */
	
	INSTR_MAX    = 0x100
};

/* 
 * table indices...
 */
enum {
	INDEX_HEAD,
	INDEX_MAXP,
	INDEX_LOCA,
	INDEX_CMAP,
	INDEX_GLYF,
	INDEX_NAME,
	
	INDEX_COUNT
};

/* 
 * struct fuvec is used by the truetype virtual machine to hold vectors in
 * the graphics state, as well as internally for some operations.
 */
struct fuvec {
	FUNIT x, y;
};

/* 
 * struct vec2f is a floating point vector used by the VM internally for
 * some operations.
 */
struct vec2f {
	float x, y;
};

struct glyph {
	UINT16 id;
	F26D6 point_size;
};

/* 
 * struct graphics_state holds the truetype virtual machine's graphics
 * state varaibles, which themselves hold information used internally by
 * the machine.
 */
struct graphics_state {
	struct fuvec projection_vector;
	struct fuvec dual_projection_vector;
	struct fuvec freedom_vector;
	
	UINT32 round_state;
	UINT32 single_width_cut_in;
	UINT32 single_width_value;
	UINT32 minimum_distance;
	UINT32 delta_shift;
	UINT32 delta_base;
	UINT32 cut_in;
	
	struct {
		BYTE instruct_control;
		BYTE scan_control;
		BYTE auto_flip;
	} flags;
};

/* 
 * struct proceedure is used to hold instructions for FDEFs and IDEFs.
 */
struct proceedure {
	BYTE* stream;
	UINT16 op;
};

/* 
 * struct point is used internally as a representation for control points.
 */
struct point {
	struct fuvec pos;
	BYTE flags;
	BYTE status;
};

/* 
 * the truetype virtual machine!
 */
struct vm {
	struct graphics_state graphics_state;
	struct proceedure* instructions;
	struct proceedure* functions;
	
	UINT32 branch_depth;
	UINT32 stream_length;
	
	BYTE* temp_stream;
	BYTE* stream;
	
	struct point* zone[2];
	UINT32 reg[REG_MAX];
	
	UINT32* memory;
	UINT32* stack;
	UINT32* cvt;
	
	struct {
		BYTE  state;
		F26D6 grid_period;
		F26D6 threshold;
		F26D6 period;
		F26D6 phase;
	} round;
};

/* 
 * struct font holds information relevant to an imported truetype
 * font file.
 */
struct font {
	char name[256];
	F26D6 point_size;
	UINT16 flags;
	UINT16 fupem;
	UINT16 ppem;
	
	int fd;
	
	struct {
		off_t glyf_start;
		INT16 loca;
		
		union {
			UINT16* f0;
			UINT32* f1;
		} locate;
		
		UINT16 segcount;
		UINT16* start_codes;
		UINT16* end_codes;
		UINT16* offsets;
		UINT16* deltas;
	} cmap;
	
	struct {
		UINT16 glyphs;
		UINT16 points;
		UINT16 contours;
		UINT16 twilight;
		UINT16 storage;
		UINT16 cvt_entries;
		UINT16 function_defs;
		UINT16 instruction_defs;
		UINT16 instructions;
	} max;
	
	struct point* zone;
	UINT32* memory;
	UINT32* cvt;
	
	struct vm vm;
};

/* 
 * a lookup for command names...
 */
extern char* command_names[INSTR_MAX];

/* 
 * a lookup for instruction handlers...
 */
extern int (*instructions[INSTR_MAX])(struct font*);

/* 
 * the dpi of the user's primary display.
 */
extern int MONITOR_DPI;

/* 
 * load_font(_fp) imports the truetype font located at _fp and returns a
 * structure to contain it.
 */
struct font* load_font(char* _fp);

/* 
 * request_glyph(_fnt, _gid) requests the unicode glyph _gid from the font
 * _fnt.
 */
int request_glyph(struct font* _fnt, UINT16 _code);

/* 
 * get_command_opcode(_name) returns the opcode of the isntruction with
 * name _name.
 */
int get_command_opcode(char* _name);

#endif /* __TRUETYPE_H__ */
