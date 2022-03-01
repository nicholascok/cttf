#ifndef __TRUETYPE_VIRTUAL_H__
#define __TRUETYPE_VIRTUAL_H__

#include "truetype.h"

void ppix(int x, int y);
void pswp();

/**************************************************************************
 * CONSTANTS
 *************************************************************************/

/* 
 * size of the virtual machine's call stack.
 */
#define MAX_CALL_DEPTH 32

/* 
 * likely 0, unless we plan on printing...
 */
#define ENGINE_COMPENSATION ( 0 )

/* 
 * engine version, just set to 3 (windows 3.1) because why not...
 */
#define ENGINE_VERSION ( 3 )

/* 
 * virtual machine error messages...
 */
enum {
	FVMERR_OK                   = 0x00,
	FVMERR_STACK_OVERFLOW       = 0x01,
	FVMERR_STACK_UNDERFLOW      = 0x02,
	FVMERR_EXECUTION_OVERFLOW   = 0x03,
	FVMERR_EXECUTION_UNDERFLOW  = 0x04,
	FVMERR_CALL_DEPTH_EXCEEDED  = 0x05,
	FVMERR_SEGMENTATION_FAULT   = 0x06,
	FVMERR_DIVIDE_BY_ZERO       = 0x07,
	FVMERR_INVALID_ZONE         = 0x08
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
	MDAP         = 0x2E, /* (M)ove (D)irect (A)bsolute (P)oint */
	IUP          = 0x30, /* (I)nterpolate (U)ntouched (P)oints */
	SHP          = 0x32, /* (SH)ift (P)oint using reference point */
	SHC          = 0x34, /* (SH)ift (C)ontour using reference point */
	SHZ          = 0x36, /* (SH)ift (Z)one using reference point */
	SHPIX        = 0x38, /* (SH)ift (P)oints by (PIX)el amount */
	IP           = 0x39, /* (I)nterpolate (P)oints */
	MSIRP        = 0x3A, /* (M)ove (S)tack (I)ndirect (R)elative (P)oint */
	ALIGNRP      = 0x3C, /* (ALIGN) to (R)eference (P)oint */
	RTDG         = 0x3D, /* (R)ound (T)o (D)ouble (G)rid */
	MIAP         = 0x3E, /* (M)ove (I)ndirect (R)elative (P)oint */
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
	DELTAP1      = 0x5D, /* (DELTA) (P)oint 1 */
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
	DELTAP2      = 0x71, /* (DELTA) (P)oint 2 */
	DELTAP3      = 0x72, /* (DELTA) (P)oint 3 */
	DELTAC1      = 0x73, /* (DELTA) (C)ontour 1 */
	DELTAC2      = 0x74, /* (DELTA) (C)ontour 2 */
	DELTAC3      = 0x75, /* (DELTA) (C)ontour 3 */
	SROUND       = 0x76, /* (S)uper (ROUND) */
	S45ROUND     = 0x77, /* (S)uper (45) degree (R)ound */
	JROT         = 0x78, /* (J)ump (R)elative (O)n (T)rue */
	JROF         = 0x79, /* (J)ump (R)elative (O)n (F)alse */
	ROFF         = 0x7A, /* (R)ound (OFF)*/
	UDEF1        = 0x7B,
	RUTG         = 0x7C, /* (R)ound (U)p (T)o (G)rid */
	RDTG         = 0x7D, /* (R)ound (D)own (T)o (G)rid */
	SANGW        = 0x7E, /* anachronistic */
	AA           = 0x7F, /* anachronistic */
	FLIPPT       = 0x80, /* (FLIP) (P)oin(T) */
	FLIPRGON     = 0x81, /* (FLIP) (R)an(G)e (ON) */
	FLIPRGOFF    = 0x82, /* (FLIP) (R)an(G)e (OFF) */
	UDEF2        = 0x83,
	UDEF3        = 0x84,
	SCANCTRL     = 0x85, /* (SCAN) (C)on(TR)o(L) */
	SDPVTL       = 0x86, /* (S)et (D)ual (P)rojection (V)ector (T)o (L)ine */
	GETINFO      = 0x88, /* (GET) (INFO) */
	IDEF         = 0x89, /* (I)nstruction (DEF)inition */
	ROLL         = 0x8A, /* (ROLL) elements at top of stack */
	MAX          = 0x8B, /* (MAX)imum */
	MIN          = 0x8C, /* (MIN)imum */
	SCANTYPE     = 0x8D, /* (SCAN) (TYPE) */
	INSTCTRL     = 0x8E, /* (INST)ruct (C)on(TR)o(L) */
	UDEF4        = 0x8F,
	UDEF5        = 0x90,
	GETVARIATION = 0x91,
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
	MDRP         = 0xC0, /* (M)ove (D)irect (R)elative (P)oint */
	MIRP         = 0xE0, /* (M)ove (I)ndirect (R)elative (P)oint */
	
	INSTR_MAX    = 0x100
};

/* 
 * registers...
 */
enum {
	EP, /* execution pointer   */
	SP, /* stack pointer       */
	LP, /* loop register       */
	
	ZP0, /* zone pointer       */
	ZP1, /* registers          */
	ZP2, /* ...                */
	
	RP0, /* reference point    */
	RP1, /* registers          */
	RP2, /* ...                */
	
	CSP, /* call stack pointer */
	
	REG_MAX
};

/* 
 * holds graphics state variables. note that some variables - RP0-2, ZP0-2,
 * and ROUND_STATE - are stored elsewhere (see notes above).
 */
struct graphics_state {
	struct vec32fx projection_vector;
	struct vec32fx dual_projection_vector;
	struct vec32fx freedom_vector;
	
	struct vec2f floating_projection_vector;
	struct vec2f floating_dual_projection_vector;
	struct vec2f floating_freedom_vector;
	float projective_factor;
	
	UINT32 control_value_cut_in;
	UINT32 single_width_cut_in;
	UINT32 single_width_value;
	UINT32 minimum_distance;
	UINT32 delta_shift;
	UINT32 delta_base;
	
	struct {
		BYTE  state;
		F26D6 grid_period;
		F26D6 threshold;
		F26D6 period;
		F26D6 phase;
	} round;
	
	struct {
		BYTE instruct_control;
		BYTE scan_control;
		BYTE auto_flip;
	} flags;
};

/* 
 * used to hold information relevant to a zone (see specification)...
 */
struct zone {
	struct point* current_points;
	struct point* original_points;
	UINT16 num_contours;
	UINT16 num_points;
	INT32* contours;
};

/* 
 * used to hold FDEFs, IDEFs, etc.
 */
struct proceedure {
	BYTE* stream;
	UINT32 length;
};

/* 
 * the truetype virtual machine!
 */
struct vm {
	BYTE is_ready;
	BYTE is_stretched;
	BYTE is_rotated;
	
	struct graphics_state graphics_state;
	struct proceedure* functions;
	struct proceedure* userdefs;
	
	struct {
		INT32  repititions;
		INT32  return_position;
		UINT32 stream_length;
		BYTE*  stream;
	}
	
	call_stack[MAX_CALL_DEPTH];
	
	struct zone zone[2];
	UINT32 reg[REG_MAX];
	
	BYTE* stream;
	
	UINT32* memory;
	UINT32* stack;
	INT32* cvt;
	
	UINT32 stream_length;
	UINT32 branch_depth;
	
	UINT32 point_size;
	UINT32 fupem;
	UINT32 ppem;
	
	struct {
		UINT32 instruction_defs;
		UINT32 control_values;
		UINT32 function_defs;
		UINT32 stack_depth;
		UINT32 storage;
	} max;
};

/* 
 * struct vm_state holds data to be loaded into the virtual machine, it is
 * passed to init_vitrual(this, _state) (see below).
 */
struct vm_state {
	UINT32 num_instructions;
	BYTE*  instructions;
	
	struct glyph* glyph;
};

/* 
 * GFVMERRNO is an error code that is set when the virtual machine
 * encounters an error.
 */
extern int GFVMERRNO;

/* 
 * GFVMERRMSGS is an array that is to be indexed by GFVMERRNO (see above)
 * for a detailed description of the error.
 */
extern char* GFVMERRMSGS[];

/* 
 * command_names is an array containing instruction names, indexed by
 * opcode. it is used exclusively for debuggung.
 */
extern char* command_names[INSTR_MAX];

/* 
 * array containing pointers to instruction handlers, indexed by opcode...
 */
extern int (*instructions[INSTR_MAX])(struct vm*);

/* 
 * make_virtual(_fnt) creates a virtual machine and initialises it with the
 * font _fnt. note that this assumes that the 'maxp' table has been read
 * and necessary values are stored in the font's 'font.max' structure.if
 * successful, the function returns 0, otherwise, -1 is returned.
 */
struct vm* make_virtual(struct font* _fnt);

/* 
 * update_virtual(_fnt) updates an existing virtual machine's data in
 * order to be consistant with the provided font.
 */
struct vm* update_virtual(struct font* _fnt);

/* 
 * init_virtual(this, _state) prepares a virtual machine for execution by
 * setting the instruction and zone inforamtion based on _state. if
 * successful, the function returns 0, otherwise, -1 is returned.
 */
int init_virtual(struct vm* this, struct vm_state _state);

/* 
 * exec_virtual(this) runs the virtual machine in its current state. note
 * that this assumes the machine is ready to be executed. if an error
 * occurs during execution, the values of GFVMERRNO and GFVERRMSG are set
 * to indicate the error and the function return -1. 
 */
int exec_virtual(struct vm* this, int d);

/* 
 * free_virtual(this) frees the virtual machine 'this'.
 */
void free_virtual(struct vm* this);

/* 
 * fvm_perror(_str) prints the error message associated with the current
 * value of GFVMERRNO (see above).
 */
void fvm_perror(void);

/* 
 * these are here so they can be called by exec_virtual(this) (see above).
 * most instructions are called by indexing 'instructions' by their opcode,
 * but there would simply be too many varients of MDRP and MIRP, so they
 * are handled seperately.
 */
int __op_MDRP(struct vm* this, BYTE flags);
int __op_MIRP(struct vm* this, BYTE flags);

#endif /* __TRUETYPE_VIRTUAL_H__ */
