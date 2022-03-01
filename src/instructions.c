#include "truetype_virtual.h"
#include "specific.h"

/**************************************************************************
 * INSTRUCTION MACROS (mostly for aesthetics, some will hate this...)
 *************************************************************************/

#define TOUCH(PNT) {\
	if (FRD_VEC.y != 0)\
		TOUCHY( PNT );\
	\
	if (FRD_VEC.x != 0)\
		TOUCHX( PNT );\
}

#define TOUCHX(PNT)   ( PNT.flags |= POINT_MASK_XTOUCHED )
#define TOUCHY(PNT)   ( PNT.flags |= POINT_MASK_YTOUCHED )
#define UNTOUCHX(PNT) ( PNT.flags &= ~POINT_MASK_XTOUCHED )
#define UNTOUCHY(PNT) ( PNT.flags &= ~POINT_MASK_YTOUCHED )

/* 
 * masks and functions for setting / getting information from the
 * machine's round state variables.
 */

#define RSTATE_MASK_THRESHOLD ( 0x0FU )
#define RSTATE_MASK_PERIOD    ( 0xC0U )
#define RSTATE_MASK_PHASE     ( 0x30U )

#define RSTATE_THRESHOLD  ( (UINT8) ((UINT8) ROUND_STATE & RSTATE_MASK_THRESHOLD) )
#define RSTATE_PERIOD     ( (UINT8) ((UINT8) ROUND_STATE & RSTATE_MASK_PERIOD) >> 6 )
#define RSTATE_PHASE      ( (UINT8) ((UINT8) ROUND_STATE & RSTATE_MASK_PHASE) >> 4 )

#define ROUND_STATE       ( GRAPHICS_STATE.round.state)
#define ROUND_GRID_PERIOD ( GRAPHICS_STATE.round.grid_period)
#define ROUND_THRESHOLD   ( GRAPHICS_STATE.round.threshold)
#define ROUND_PERIOD      ( GRAPHICS_STATE.round.period)
#define ROUND_PHASE       ( GRAPHICS_STATE.round.phase)

/* 
 * macros for accessing data through the zone pointer registers...
 */

#define ZONEP0(N)  ( this->zone[REG_ZP0].current_points[N] )
#define ZONEP1(N)  ( this->zone[REG_ZP1].current_points[N] )
#define ZONEP2(N)  ( this->zone[REG_ZP2].current_points[N] )

#define OZONEP0(N) ( this->zone[REG_ZP0].original_points[N] )
#define OZONEP1(N) ( this->zone[REG_ZP1].original_points[N] )
#define OZONEP2(N) ( this->zone[REG_ZP2].original_points[N] )

#define ZONER0     ( this->zone[REG_ZP0] )
#define ZONER1     ( this->zone[REG_ZP1] )
#define ZONER2     ( this->zone[REG_ZP2] )

#define POINTS_IN_ZONE(N)    ( this->zone[N].num_points )

/* 
 * access to registers, graphics state variables, and limits.
 */

#define DPRJ_VEC  ( GRAPHICS_STATE.dual_projection_vector )
#define PRJ_VEC   ( GRAPHICS_STATE.projection_vector )
#define FRD_VEC   ( GRAPHICS_STATE.freedom_vector )

#define FDPRJ_VEC ( GRAPHICS_STATE.floating_dual_projection_vector )
#define FPRJ_VEC  ( GRAPHICS_STATE.floating_projection_vector )
#define FFRD_VEC  ( GRAPHICS_STATE.floating_freedom_vector )

/* 
 * PROJECTIVE_FACTOR holds the value of 1.0 / (PRJ_VEC . FRD_VEC)
 * this is useful for projecting a value along the projection vector
 * (a coordinate) to the freedom vector with a single multiplication.
 */
#define PROJECTIVE_FACTOR ( GRAPHICS_STATE.projective_factor )

#define GRAPHICS_STATE       ( this->graphics_state )
#define MAX_INSTRUCTION_DEFS ( this->max.instruction_defs )
#define MAX_FUNCTION_DEFS    ( this->max.function_defs )
#define MAX_CONTROL_ENTRIES  ( this->max.control_values )
#define MAX_STORAGE          ( this->max.storage )
#define MAX_DEPTH            ( this->max.stack_depth )
#define MAX_STREAM           ( this->stream_length )

#define REG_SP    ( this->reg[SP] )
#define REG_EP    ( this->reg[EP] )
#define REG_LP    ( this->reg[LP] )
#define REG_RP    ( this->reg[RP] )
#define REG_CSP   ( this->reg[CSP] )
#define REG_RP0   ( this->reg[RP0] )
#define REG_RP1   ( this->reg[RP1] )
#define REG_RP2   ( this->reg[RP2] )
#define REG_ZP0   ( this->reg[ZP0] )
#define REG_ZP1   ( this->reg[ZP1] )
#define REG_ZP2   ( this->reg[ZP2] )

#define POINT_SIZE    ( this->point_size )
#define PIXELS_PER_EM ( this->ppem )

#define BRANCH_DEPTH  ( this->branch_depth )

/* 
 * note: since we only allow two zones (for now), and since no contours
 * are in the twilight zone (really) these will be hard coded to zone 1.
 */

#define CONTOUR_START(N)     ( this->zone[1].contours[N] + 1 )
#define CONTOUR_END(N)       ( this->zone[1].contours[N + 1U] )
#define CONTOUR_SIZE(N)      ( (UINT32) CONTOUR_END(N) - CONTOUR_START(N) + 1 )

/* 
 * gets the N-th element from the top of the stack as signed (SSTACK) or
 * unsigned (USTACK / STACK) INT32s.
 */

#define STACK(N)  ( this->stack[REG_SP - (N) - 1U] )
#define SSTACK(N) ( *((INT32*) &STACK(N)) )
#define USTACK(N) ( *((UINT32*) &STACK(N)) )

/* 
 * miscelaneous (should be self-explanatory)...
 */
#define CALL_STACK(N)    ( this->call_stack[REG_CSP - (N) - 1U] )
#define STREAM(N)        ( this->stream[REG_EP + (N) - 1U] )
#define FUNCTION(N)      ( this->functions[N] )
#define USERDEF(N)       ( this->userdefs[N] )
#define MEMORY(N)        ( this->memory[N] )
#define CONTROL_VALUE(N) ( this->cvt[N + 1] )

#define PIXELSTOFU(X) ( ((X) * this->fupem / 64) / this->ppem )
#define FUTOPIXELS(X) ( ((X) * this->ppem * 64) / this->fupem )

/* 
 * update_round_state(this) updates the round state of the truetype virtual
 * machine. it is called after the packed round state setting has chagned.
 */
static int update_round_state(struct vm* this) {
	if (ROUND_STATE == ~0) return 0;
	
	switch (RSTATE_PERIOD) {
		case 3: /* a value of 3 here is technically reserved */
		case 0: ROUND_PERIOD = F26D6MUL(ROUND_GRID_PERIOD, 32);  break;
		case 1: ROUND_PERIOD = ROUND_GRID_PERIOD;                break;
		case 2: ROUND_PERIOD = F26D6MUL(ROUND_GRID_PERIOD, 128); break;
	}
	
	switch (RSTATE_PHASE) {
		case 0: ROUND_PHASE = 0;                                 break;
		case 1: ROUND_PHASE = F26D6MUL(ROUND_PERIOD, 16);        break;
		case 2: ROUND_PHASE = F26D6MUL(ROUND_PERIOD, 32);        break;
		case 3: ROUND_PHASE = F26D6MUL(ROUND_GRID_PERIOD, 48);   break;
	}
	
	ROUND_THRESHOLD = (RSTATE_THRESHOLD == 0) ? ROUND_PERIOD - 1 :
		F26D6MUL(ROUND_PERIOD, 8 * RSTATE_THRESHOLD - 32);
	
	return 0;
}

/* 
 * round_by_state(this, _x) rounds the 26.6 floating point number _x
 * according to the current round state setting.
 */
static F26D6 round_by_state(struct vm* this, F26D6 _x) {
	char s_old = SIGN(_x);
	char s_new = 0;
	
	/* a value of ~0 (0xFFFFFFFF) signifies no rounding... */
	if (ROUND_STATE == ~0) return _x;
	
	_x = (_x - ROUND_PHASE + ROUND_THRESHOLD);
	_x = _x - (_x % ROUND_PERIOD + ROUND_PERIOD) % ROUND_PERIOD;
	_x += ROUND_PHASE;
	
	s_new = SIGN(_x);
	
	if (s_new != s_old)
		_x = (s_old == 1) ? ROUND_PHASE : -ROUND_PHASE;
	
	return _x;
}

static F26D6 apply_compensation(F26D6 _n, int _type) {
	INT8 sign = SIGN(_n);	/* original sign of value */
	
	/* grey distances do not change... */
	if (_type == 0)
		return _n;
	
	/* black distances shrink... */
	else if (_type == 1)
		_n -= sign * ENGINE_COMPENSATION;
	
	/* white distances grow... */
	else if (_type == 2)
		_n += sign * ENGINE_COMPENSATION;
	
	/* if compensation brought our value past zero, we will set it to
	 * zero instead of dealing with a sign change. */
	if (SIGN(_n) != sign) return 0;
	
	return _n;
}

static int update_floating_vectors(struct vm* this) {
	FDPRJ_VEC.x = (float) DPRJ_VEC.x / 16384;
	FDPRJ_VEC.y = (float) DPRJ_VEC.y / 16384;
	FPRJ_VEC.x = (float) PRJ_VEC.x / 16384;
	FPRJ_VEC.y = (float) PRJ_VEC.y / 16384;
	FFRD_VEC.x = (float) FRD_VEC.x / 16384;
	FFRD_VEC.y = (float) FRD_VEC.y / 16384;
	
	PROJECTIVE_FACTOR = 1.0 / dot2f(FPRJ_VEC, FFRD_VEC);
	
	return 0;
}

static F26D6 get_delta(struct vm* this, F26D6 _step) {
	_step = _step - 7;
	_step -= (_step <= 0);
	
	switch (GRAPHICS_STATE.delta_shift) {
		default: /* cap at 6 */
		case 6: _step *= 1 ; break;
		case 5: _step *= 2 ; break;
		case 4: _step *= 4 ; break;
		case 3: _step *= 8 ; break;
		case 2: _step *= 16; break;
		case 1: _step *= 32; break;
		case 0: _step *= 64; break;
	}
	
	return _step;
}

/* 
 * get_udef_index(this, opcode) returns the index in the virtual machine's
 * instruction proceedure list for a given user-definable opcode, if it has
 * one, otherwise -1 is returned.
 */
static int get_udef_index(struct vm* this, BYTE opcode) {
	switch (opcode) {
		case UDEF0: return 0;
		case UDEF1: return 1;
		case UDEF2: return 2;
		case UDEF3: return 3;
		case UDEF4: return 4;
		case UDEF5: return 5;
		case UDEF6: return 6;
		case UDEF7: return 7;
		case UDEF8: return 8;
		case UDEF9: return 9;
		case UDEFA: return 10;
		case UDEFB: return 11;
		case UDEFC: return 12;
		case UDEFD: return 13;
		case UDEFE: return 14;
		case UDEFF: return 15;
		case UDEFG: return 16;
		case UDEFH: return 17;
		case UDEFI: return 18;
		case UDEFJ: return 19;
		case UDEFK: return 20;
		case UDEFL: return 21;
		case UDEFM: return 22;
		case UDEFN: return 23;
		case UDEFO: return 24;
		case UDEFP: return 25;
		case UDEFQ: return 26;
		case UDEFR: return 27;
		case UDEFS: return 28;
		case UDEFT: return 29;
		case UDEFU: return 30;
		case UDEFV: return 31;
		case UDEFW: return 32;
		case UDEFX: return 33;
		case UDEFY: return 34;
		case UDEFZ: return 35;
		default: return -1;
	}
}

/**************************************************************************
 * all the instructions, see the specification for information on any     *
 * specific one.                                                          *
 *************************************************************************/

/* 
 * UDEF
 * 
 * desc: called by the interpreter for instructions that normally have no
 * defined use, and which the user has assigned a proceedure to.
 */
static int op_UDEF(struct vm* this, BYTE opcode) {
	int index = get_udef_index(this, opcode);
	
	/* check this is an actual UDEF... */
	if (index == -1) return -1;
	
	/* call proceedure */
	
	if (REG_CSP + 1 > MAX_CALL_DEPTH) return -1;
	
	/* if the proceedure we are trying to call exists, switch our
	 * stream to the one which contains the proceedure's body. */
	if (USERDEF(index).stream) {
		/* add a new element onto the call stack. */
		REG_CSP++;
		
		CALL_STACK(0).repititions = 1;
		CALL_STACK(0).return_position = REG_EP;
		CALL_STACK(0).stream_length = this->stream_length;
		CALL_STACK(0).stream = this->stream;
		
		this->stream_length = USERDEF(index).length;
		this->stream = USERDEF(index).stream;
		
		/* in this new stream, the execution is at position 1. */
		REG_EP = 1;
	}
	
	/* otherwise, if the proceedure we are trying to call does not
	 * exist then return -1 to indicate that an error has occurred. */
	else return -1;
	
	return 0;
}

/* 
 * MDRP[abcde]
 * 
 * a ) if set, RP0 will be set to reference the point after it is moved.
 * 
 * b ) if set, the distance the point's coordinate will be kept greater
 *     then the current setting of the 'minimum_distance' state variable.
 * 
 * c ) if set, the final distance will be rounded according to the current
 *     setting of ROUND_STATE.
 * 
 * de) distance type (0 = grey, 1 = white, 2 = black)
 * 
 * desc: moves a point along FRD_VEC so that is coordinate relative to the
 * reference point RP0, as projected onto PRJ_VEC (or DPRJ_VEC when
 * referencing the original outline), is the same as it was in the 
 * original, uninstructed outline of the current glyph.
 * 
 * pops: (UINT32) point number
 * 
 */
int __op_MDRP(struct vm* this, BYTE flags) {
	struct vec2f P, oP;	/* current and original point positions */
	struct vec2f R, oR;	/* current and original reference point positions */
	
	float Pc, oPc;	/* current and original point coordinates */
	float Rc, oRc;	/* current and original reference point coordinates */
	
	float dist;	/* distance between our point and the reference */
	           	/*   point rp0 in the original outline */
	float delta;
	
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* compute point coordinates along the projection vector (or dual
	 * projection vector, if regarding the original outline). */
	
	P.x = (float) ZONEP1(STACK(0)).pos.x;
	P.y = (float) ZONEP1(STACK(0)).pos.y;
	Pc = dot2f(FPRJ_VEC, P);
	
	oP.x = (float) OZONEP1(STACK(0)).pos.x;
	oP.y = (float) OZONEP1(STACK(0)).pos.y;
	oPc = dot2f(FDPRJ_VEC, oP);
	
	R.x = (float) ZONEP0(REG_RP0).pos.x;
	R.y = (float) ZONEP0(REG_RP0).pos.y;
	Rc = dot2f(FPRJ_VEC, R);
	
	oR.x = (float) OZONEP0(REG_RP0).pos.x;
	oR.y = (float) OZONEP0(REG_RP0).pos.y;
	oRc = dot2f(FDPRJ_VEC, oR);
	
	/* compute distance from our point to the reference point in the
	 * original outline. */
	dist = oPc - oRc;
	
	/* if the difference between this distance and the value of the
	 * graphics state variable single_width_value is less than the
	 * single_width_cut_in then we will use the single_width_value
	 * instead of the distance in the original outline. */
	if (ABS(ABS(dist) - GRAPHICS_STATE.single_width_value) <
	    GRAPHICS_STATE.single_width_cut_in)
		dist = SIGN(dist) * GRAPHICS_STATE.single_width_value;
	
	/* apply engine compensation to value... */
	dist = apply_compensation(FROUND(dist), (flags & 0x03));
	
	/* if the round flag is set the we will round our distance
	 * according the the current setting of the round state. */
	if (flags & 0x04)
		dist = round_by_state(this, dist);
	
	/* if the minimum distance flag is set then we will ensure that our
	 * distance is at least the value of the graphics state variable
	 * minimum_distance. */
	if (flags & 0x08)
		if (ABS(dist) < GRAPHICS_STATE.minimum_distance)
			dist = SIGN(dist) * GRAPHICS_STATE.minimum_distance;
	
	/* for more information on what is going here see note 1. */
	delta = ((dist + Rc) - Pc) * PROJECTIVE_FACTOR;
	
	ZONEP1(STACK(0)).pos.x += FROUND(FFRD_VEC.x * delta);
	ZONEP1(STACK(0)).pos.y += FROUND(FFRD_VEC.y * delta);
	
	/* MDRP touches the point it moves... */
	TOUCH( ZONEP1(STACK(0)) );
	
	/* if the flag is set, set rp0 to our point. */
	if (flags & 0x10) REG_RP0 = STACK(0);
	
	REG_SP--;
	
	return 0;
}

/* 
 * MIRP[abcde]
 * 
 * a ) if set, RP0 will be set to reference the point after it is moved.
 * 
 * b ) if set, the distance the point's coordinate will be kept greater
 *     then the current setting of the 'minimum_distance' state variable.
 * 
 * c ) if set, a cut-in test will be preformed and the final distance will
 *     be rounded according to the current setting of ROUND_STATE.
 * 
 * de) distance type (0 = grey, 1 = white, 2 = black)
 * 
 * desc: moves a point along FRD_VEC so that is coordinate relative to the
 * reference point RP0, as projected onto PRJ_VEC (or DPRJ_VEC when
 * referencing the original outline), is equal to the value stored in the
 * indexed control value table entry.
 * 
 * pops: (UINT32) index in the control value table
 *       (UINT32) point number
 * 
 */
int __op_MIRP(struct vm* this, BYTE flags) {
	struct vec2f P, oP;	/* current and original point positions */
	struct vec2f R, oR;	/* current and original reference point positions */
	
	float Pc, oPc;	/* current and original point coordinates */
	float Rc, oRc;	/* current and original reference point coordinates */
	
	float cvt; 	/* distance in control value table */
	float dist;	/* distance between our point and the reference */
	           	/*   point rp0 in the original outline */
	float delta;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) >= MAX_CONTROL_ENTRIES) return FVMERR_SEGMENTATION_FAULT;
	
	/* compute point coordinates along the projection vector (or dual
	 * projection vector, if regarding the original outline). */
	
	printf("%u %u\n", STACK(1), this->zone[1].num_points);
	
	P.x = (float) ZONEP1(STACK(1)).pos.x;
	P.y = (float) ZONEP1(STACK(1)).pos.y;
	Pc = dot2f(FPRJ_VEC, P);
	
	oP.x = (float) OZONEP1(STACK(1)).pos.x;
	oP.y = (float) OZONEP1(STACK(1)).pos.y;
	oPc = dot2f(FDPRJ_VEC, oP);
	
	R.x = (float) ZONEP0(REG_RP0).pos.x;
	R.y = (float) ZONEP0(REG_RP0).pos.y;
	Rc = dot2f(FPRJ_VEC, R);
	
	oR.x = (float) OZONEP0(REG_RP0).pos.x;
	oR.y = (float) OZONEP0(REG_RP0).pos.y;
	oRc = dot2f(FDPRJ_VEC, oR);
	
	/* if the difference between the value in the control value table
	 * and the graphics state variable single_width_value is less than
	 * the single_width_cut_in then we will use the single_width_value
	 * instead of the control value table entry. */
	cvt = (float) CONTROL_VALUE(STACK(0));
	
	printf("  %f\n", cvt);
	
	if (ABS(ABS(cvt) - GRAPHICS_STATE.single_width_value) <
	    GRAPHICS_STATE.single_width_cut_in)
		cvt = SIGN(cvt) * GRAPHICS_STATE.single_width_value;
	
	/* compute distance from our point to the reference point in the
	 * original outline. */
	dist = oPc - oRc;
	
	/* if the auto_flip flag is set then we will adjust the sign of
	 * the entry in the control value table to match the sign of the
	 * distance from our point to the reference point along the
	 * projection vector in the original outline. */
	if (GRAPHICS_STATE.flags.auto_flip) {
		if (SIGN(dist) != SIGN(cvt))
			cvt = -cvt;
	}
	
	/* if the round flag is set we will perform the control value cut
	 * in test and round the value, otherwise, the value will recieve
	 * engine compensation only. */
	if (flags & 0x04) {
		/* according to FreeType source code, though undocumetned,
		 * the control value cut in test is only performed if both
		 * points are in the same zone. */
		if (REG_ZP1 == REG_ZP0) {
			/* if the difference between the distance between
			 * our points in the original outline and the value
			 * in the control value table differ by more than
			 * the control_value_cut_in then we will ignore the
			 * control value table entry and use the original
			 * distance. */
			if (ABS(cvt - dist) > GRAPHICS_STATE.control_value_cut_in)
				cvt = dist;
		}
		
		/* apply engine compensation and round... */
		cvt = apply_compensation(FROUND(cvt), (flags & 0x03));
		cvt = round_by_state(this, FROUND(cvt));
	} else {
		/* apply only engine compensation (no rounding) */
		cvt = apply_compensation(FROUND(cvt), (flags & 0x03));
	}
	
	/* if the minimum distance flag is set then we will ensure that our
	 * distance is at least the value of the graphics state variable
	 * minimum_distance. */
	if (flags & 0x08)
		if (ABS(cvt) < GRAPHICS_STATE.minimum_distance)
			cvt = SIGN(cvt) * GRAPHICS_STATE.minimum_distance;
	
	/* for more information on what is going here see note 1. */
	delta = ((cvt + Rc) - Pc) * PROJECTIVE_FACTOR;
	
	ZONEP1(STACK(1)).pos.x += FROUND(FFRD_VEC.x * delta);
	ZONEP1(STACK(1)).pos.y += FROUND(FFRD_VEC.y * delta);
	
	/* MIRP touches the point it moves... */
	TOUCH( ZONEP1(STACK(1)) );
	
	/* if the flag is set, set rp0 to our point. */
	if (flags & 0x10) REG_RP0 = STACK(1);
	
	REG_SP -= 2;
	
	return 0;
}

/* ! */
static int op_SCANTYPE(struct vm* this) {
	REG_SP--;
	printf("SCANTYPE: NOT IMPLEMENTED!\n");
	exit(0);
	
	return 0;
}

/* ! */
static int op_SCANCTRL(struct vm* this) {
	REG_SP--;
	printf("SCANCTRL: NOT IMPLEMENTED!\n");
	exit(0);
	
	return 0;
}

/* ! */
static int op_INSTCTRL(struct vm* this) {
	REG_SP -= 2;
	printf("INSTCTRL: NOT IMPLEMENTED!\n");
	exit(0);
	
	return 0;
}

static int op_GETINFO(struct vm* this) {
	UINT32 result = 0;
	
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	if (STACK(0) & 0x00000001)
		result |= ENGINE_VERSION;
	
	if (STACK(0) & 0x00000002)
		if (this->is_rotated)
			result |= 0x00000100;
	
	if (STACK(0) & 0x00000004)
		if (this->is_stretched)
			result |= 0x00000200;
	
	return 0;
}

static int DELTAP(struct vm* this, BYTE offset) {
	UINT16 ppem;
	UINT32 N, i;
	
	float delta;
	
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* get number of exceptions listed... */
	N = STACK(0);
	REG_SP--;
	
	if (REG_SP < 2 * N) return FVMERR_STACK_UNDERFLOW;
	
	/* loop through exceptions... */
	for (i = 0; i < N; i++) {
		/* calculate ppem */
		ppem = GRAPHICS_STATE.delta_base + offset;
		ppem += ((STACK(1) & 0xF0) >> 4);
		
		delta = (float) get_delta(this, (STACK(1) & 0x0F));
		
		/* if this is the same as our current ppem then the
		 * exception will be applied. */
		if (ppem == PIXELS_PER_EM) {
			/* shift our point along the projection vector. */
			ZONEP1(STACK(0)).pos.x += FROUND(FFRD_VEC.x * delta);
			ZONEP1(STACK(0)).pos.y += FROUND(FFRD_VEC.y * delta);
		}
		
		REG_SP -= 2;
	}
	
	return 0;
}

static int op_DELTAP3(struct vm* this) {
	return DELTAP(this, 32);
}

static int op_DELTAP2(struct vm* this) {
	return DELTAP(this, 16);
}

static int op_DELTAP1(struct vm* this) {
	return DELTAP(this, 0);
}

static int DELTAC(struct vm* this, BYTE offset) {
	UINT16 ppem;
	UINT32 N, i;
	
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) >= MAX_CONTROL_ENTRIES) return FVMERR_SEGMENTATION_FAULT;
	
	/* get number of exceptions listed... */
	N = STACK(0);
	REG_SP--;
	
	if (REG_SP < 2 * N) return FVMERR_STACK_UNDERFLOW;
	
	/* loop through exceptions... */
	for (i = 0; i < N; i++) {
		ppem = GRAPHICS_STATE.delta_base + offset;
		ppem += ((STACK(1) & 0xF0) >> 4);
		
		/* if this is the same as our current ppem then the
		 * exception will be applied. */
		if (ppem == PIXELS_PER_EM)
			CONTROL_VALUE(STACK(0) + 1) +=
				(float) get_delta(this, (STACK(1) & 0x0F));
		
		REG_SP -= 2;
	}
	
	return 0;
}

static int op_DELTAC3(struct vm* this) {
	return DELTAC(this, 32);
}

static int op_DELTAC2(struct vm* this) {
	return DELTAC(this, 16);
}

static int op_DELTAC1(struct vm* this) {
	return DELTAC(this, 0);
}

static int op_MD_0(struct vm* this) {
	/* note: according to FreeType soruce code, the specification has
	 * MD[0] and MD[1] reversed, and this function should instead
	 * measure from the original outline. */
	
	struct vec2f oP1, oP2;	/* points */
	float oP1c, oP2c;     	/* point coordiantes */
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	/* compute point coordinates along the dual projection vector. */
	
	oP1.x = (float) ZONEP1(STACK(0)).pos.x;
	oP1.y = (float) ZONEP1(STACK(0)).pos.y;
	oP1c = dot2f(FDPRJ_VEC, oP1);
	
	oP2.x = (float) ZONEP0(STACK(1)).pos.x;
	oP2.y = (float) ZONEP0(STACK(1)).pos.y;
	oP2c = dot2f(FDPRJ_VEC, oP2);
	
	STACK(1) = FROUND(oP2c - oP1c);
	
	REG_SP--;
	
	return 0;
}

static int op_MD_1(struct vm* this) {
	/* note: according to FreeType soruce code, the specification has
	 * MD[0] and MD[1] reversed, and this function should instead
	 * measure from the grid-fitted outline. */
	
	struct vec2f P1, P2;	/* points */
	float P1c, P2c;     	/* point coordiantes */
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	/* compute point coordinates along the projection vector. */
	
	P1.x = (float) ZONEP1(STACK(0)).pos.x;
	P1.y = (float) ZONEP1(STACK(0)).pos.y;
	P1c = dot2f(FDPRJ_VEC, P1);
	
	P2.x = (float) ZONEP0(STACK(1)).pos.x;
	P2.y = (float) ZONEP0(STACK(1)).pos.y;
	P2c = dot2f(FDPRJ_VEC, P2);
	
	STACK(1) = FROUND(P2c - P1c);
	
	REG_SP--;
	
	return 0;
}

static int op_IUP_0(struct vm* this) {
	INT32 omin, omax; 	/* original coordinate range for bounds */
	INT32 cmin, cmax; 	/* current coordinate range for bounds */
	INT32 lBx, olBx;  	/* bounding point coordintes */
	INT32 rBx, orBx;  	/* bounding point coordintes */
	INT32 start;      	/* starting point for contour */
	INT32 pnt;        	/* point index calculated from contour offset */
	INT32 lB;         	/* left bounding point for range */
	INT32 rB;         	/* right bounding point for range */
	
	UINT16 i, j;
	
	/* this instruction cannot be executed in the twilight zone. */
	if (REG_ZP2 == 0) return FVMERR_INVALID_ZONE;
	
	/* loop through each contour in the glyph */
	for (i = 0; i < ZONER0.num_contours; i++) {
		/* this is here to prevent a floating point exception (due
		 * to modulo zero arithmetic) in case a contour is either
		 * malformed, or has been misread. */
		if (CONTOUR_SIZE(i) == 0) continue;
		
		start = rB = lB = -1;
		
		/* we will start by finding a touched point to use for a
		 * left bound... */
		for (j = 0; j < CONTOUR_SIZE(i); j++) {
			pnt = CONTOUR_START(i) + j;
			
			if (IS_TOUCHED_X(ZONEP2(pnt))) {
				start = lB = j;
				break;
			}
		}
		
		/* if all points are untouched, then we are done. */
		if (start == -1) continue;
		
		do /* process untouched points */ {
			
			/* now we start at the point following our left
			 * bound and search for another point to use as a
			 * right bound. note that this is guarenteed since
			 * at worst we end up back at our left bound
			 * again. */
			j = (lB + 1) % CONTOUR_SIZE(i);
			
			for (;; j++, j %= CONTOUR_SIZE(i)) {
				pnt = CONTOUR_START(i) + j;
				
				if (IS_TOUCHED_X(ZONEP2(pnt))) {
					rB = j;
					break;
				}
			}
			
			lBx = ZONEP2(CONTOUR_START(i) + lB).pos.x;
			olBx = OZONEP2(CONTOUR_START(i) + lB).pos.x;
			
			rBx = ZONEP2(CONTOUR_START(i) + rB).pos.x;
			orBx = OZONEP2(CONTOUR_START(i) + rB).pos.x;
			
			/* with both of our bounding points we will find
			 * coordinate bounds along our axis. */
			cmin = MIN(lBx, rBx);
			cmax = MAX(lBx, rBx);
			omin = MIN(olBx, orBx);
			omax = MAX(olBx, orBx);
			
			/* now we will process untouch points within our
			 * established bounds. */
			j = (lB + 1) % CONTOUR_SIZE(i);
			
			for (; j != rB; j++, j %= CONTOUR_SIZE(i)) {
				pnt = CONTOUR_START(i) + j;
				
				/* if this point was not physically between
				 * our bounding points in the original
				 * outline, then we will shift the point by
				 * the same amount the closest bounding
				 * point has been shifted. */
				if (OZONEP2(pnt).pos.x <= omin)
					ZONEP2(pnt).pos.x += (cmin - omin);
				
				else if (OZONEP2(pnt).pos.x >= omax)
					ZONEP2(pnt).pos.x += (cmax - omax);
				
				/* if this point was physically between our
				 * bounding points in the original outline
				 * then we will interpolate its coordinate
				 * to maintain its relative distance to the
				 * bounds, similar to a call to IP[]. */
				else {
					ZONEP2(pnt).pos.x = cmin + (
						(cmax - cmin) *
						(ZONEP2(pnt).pos.x - omin)
					) / (omax - omin);
				}
			}
			
			lB = rB;
			
		} while (lB != start);
	}
	
	return 0;
}

static int op_IUP_1(struct vm* this) {
	INT32 omin, omax; 	/* original coordinate range for bounds */
	INT32 cmin, cmax; 	/* current coordinate range for bounds */
	INT32 lBy, olBy;  	/* bounding point coordintes */
	INT32 rBy, orBy;  	/* bounding point coordintes */
	INT32 start;      	/* starting point for contour */
	INT32 pnt;        	/* point index calculated from contour offset */
	INT32 lB;         	/* left bounding point for range */
	INT32 rB;         	/* right bounding point for range */
	
	UINT16 i, j;
	
	/* this instruction cannot be executed in the twilight zone. */
	if (REG_ZP2 == 0) return FVMERR_INVALID_ZONE;
	
	/* loop through each contour in the glyph */
	for (i = 0; i < ZONER0.num_contours; i++) {
		/* this is here to prevent a floating point exception (due
		 * to modulo zero arithmetic) in case a contour is either
		 * malformed, or has been misread. */
		if (CONTOUR_SIZE(i) == 0) continue;
		
		start = rB = lB = -1;
		
		/* we will start by finding a touched point to use for a
		 * left bound... */
		for (j = 0; j < CONTOUR_SIZE(i); j++) {
			pnt = CONTOUR_START(i) + j;
			
			if (IS_TOUCHED_X(ZONEP2(pnt))) {
				start = lB = j;
				break;
			}
		}
		
		/* if all points are untouched, then we are done. */
		if (start == -1) continue;
		
		do /* process untouched points */ {
			
			/* now we start at the point following our left
			 * bound and search for another point to use as a
			 * right bound. note that this is guarenteed since
			 * at worst we end up back at our left bound
			 * again. */
			j = (lB + 1) % CONTOUR_SIZE(i);
			
			for (;; j++, j %= CONTOUR_SIZE(i)) {
				pnt = CONTOUR_START(i) + j;
				
				if (IS_TOUCHED_X(ZONEP2(pnt))) {
					rB = j;
					break;
				}
			}
			
			lBy = ZONEP2(CONTOUR_START(i) + lB).pos.y;
			olBy = OZONEP2(CONTOUR_START(i) + lB).pos.y;
			
			rBy = ZONEP2(CONTOUR_START(i) + rB).pos.y;
			orBy = OZONEP2(CONTOUR_START(i) + rB).pos.y;
			
			/* with both of our bounding points we will find
			 * coordinate bounds along our axis. */
			cmin = MIN(lBy, rBy);
			cmax = MAX(lBy, rBy);
			omin = MIN(olBy, orBy);
			omax = MAX(olBy, orBy);
			
			/* now we will process untouch points within our
			 * established bounds. */
			j = (lB + 1) % CONTOUR_SIZE(i);
			
			for (; j != rB; j++, j %= CONTOUR_SIZE(i)) {
				pnt = CONTOUR_START(i) + j;
				
				/* if this point was not physically between
				 * our bounding points in the original
				 * outline, then we will shift the point by
				 * the same amount the closest bounding
				 * point has been shifted. */
				if (OZONEP2(pnt).pos.y <= omin)
					ZONEP2(pnt).pos.y += (cmin - omin);
				
				else if (OZONEP2(pnt).pos.y >= omax)
					ZONEP2(pnt).pos.y += (cmax - omax);
				
				/* if this point was physically between our
				 * bounding points in the original outline
				 * then we will interpolate its coordinate
				 * to maintain its relative distance to the
				 * bounds, similar to a call to IP[]. */
				else {
					ZONEP2(pnt).pos.y = cmin + (
						(cmax - cmin) *
						(ZONEP2(pnt).pos.y - omin)
					) / (omax - omin);
				}
			}
			
			lB = rB;
			
		} while (lB != start);
	}
	
	return 0;
}

static int op_MSIRP_0(struct vm* this) {
	struct vec2f R;	/* reference point */
	struct vec2f P;	/* point */
	
	float Pc;	/* point coordinate */
	float Rc;	/* reference point coordinate */
	
	float delta;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	/* compute point coordinates along the projection vector. */
	
	P.y = (float) ZONEP1(STACK(1)).pos.y;
	P.x = (float) ZONEP1(STACK(1)).pos.x;
	Pc = dot2f(FPRJ_VEC, P);
	
	R.y = (float) ZONEP0(REG_RP0).pos.y;
	R.x = (float) ZONEP0(REG_RP0).pos.x;
	Rc = dot2f(FPRJ_VEC, R);
	
	/* for more information on what is going here see note 1. */
	delta = ( (float) STACK(0) - (Pc - Rc) ) * PROJECTIVE_FACTOR;
	
	ZONEP2(STACK(1)).pos.x += FROUND(FFRD_VEC.x * delta);
	ZONEP2(STACK(1)).pos.y += FROUND(FFRD_VEC.y * delta);
	
	/* MSIRP touches the point it moves... */
	TOUCH( ZONEP1(STACK(1)) );
	
	/* set rp1 = rp0 and rp2 = our point */
	REG_RP1 = REG_RP0;
	REG_RP2 = STACK(1);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_MSIRP_1(struct vm* this) {
	struct vec2f R;	/* reference point */
	struct vec2f P;	/* point */
	
	float Pc;	/* point coordinate */
	float Rc;	/* reference point coordinate */
	
	float delta;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	/* compute point coordinates along the projection vector. */
	
	P.y = (float) ZONEP1(STACK(1)).pos.y;
	P.x = (float) ZONEP1(STACK(1)).pos.x;
	Pc = dot2f(FPRJ_VEC, P);
	
	R.y = (float) ZONEP0(REG_RP0).pos.y;
	R.x = (float) ZONEP0(REG_RP0).pos.x;
	Rc = dot2f(FPRJ_VEC, R);
	
	/* for more information on what is going here see note 1. */
	delta = ( (float) STACK(0) - (Pc - Rc) ) * PROJECTIVE_FACTOR;
	
	ZONEP2(STACK(1)).pos.x += FROUND(FFRD_VEC.x * delta);
	ZONEP2(STACK(1)).pos.y += FROUND(FFRD_VEC.y * delta);
	
	/* MSIRP touches the point it moves... */
	TOUCH( ZONEP1(STACK(1)) );
	
	/* set rp1 = rp0 and rp0 = rp2 = our point */
	REG_RP1 = REG_RP0;
	REG_RP0 = REG_RP2 = STACK(1);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_IP(struct vm* this) {
	struct vec2f R1, oR1;	/* coordinates of reference point 1 */
	struct vec2f R2, oR2;	/* coordinates of reference point 2 */
	struct vec2f P, oP;  	/* coordinates of point */
	
	float R1c, oR1c;	/* projected coordinates of reference point 1 */
	float R2c, oR2c;	/* projected coordinates of reference point 2 */
	float Pc, oPc;  	/* projected coordinates of point */
	
	float delta;
	
	UINT32 i;
	
	if (REG_SP < REG_LP) return FVMERR_STACK_UNDERFLOW;
	
	/* compute reference point coordinates along the projection vector
	 * (or dual  projection vector, if regarding the original outline). */
	
	oR1.x = (float) OZONEP0(REG_RP1).pos.x;
	oR1.y = (float) OZONEP0(REG_RP1).pos.y;
	oR1c = dot2f(FDPRJ_VEC, oR1);
	
	oR2.x = (float) OZONEP1(REG_RP2).pos.x;
	oR2.y = (float) OZONEP1(REG_RP2).pos.y;
	oR2c = dot2f(FDPRJ_VEC, oR2);
	
	R1.x = (float) ZONEP0(REG_RP1).pos.x;
	R1.y = (float) ZONEP0(REG_RP1).pos.y;
	R1c = dot2f(FPRJ_VEC, R1);
	
	R2.x = (float) ZONEP1(REG_RP2).pos.x;
	R2.y = (float) ZONEP1(REG_RP2).pos.y;
	R2c = dot2f(FPRJ_VEC, R2);
	
	/* loop over points */
	for (i = 0; i < REG_LP; i++) {
		/* compute point coordinates along the projection vector (or dual
		 * projection vector, if regarding the original outline). */
		
		oP.x = (float) OZONEP2(STACK(0)).pos.x;
		oP.y = (float) OZONEP2(STACK(0)).pos.y;
		oPc = dot2f(FDPRJ_VEC, oP);
		
		P.x = (float) ZONEP2(STACK(0)).pos.x;
		P.y = (float) ZONEP2(STACK(0)).pos.y;
		Pc = dot2f(FPRJ_VEC, P);
		
		printf("op%f p%f or1%f r1%f or2%f r2%f\n", oPc, Pc, oR1c, R1c, oR2c, R2c);
		
		delta = (R2c - R1c) * (oPc - oR1c) / (oR2c - oR1c);
		
		/* prevent a floating point exception... */
		if (oR2c == oR2c) delta = oPc - oR1c;
		
		delta = (delta + R1c - Pc) * PROJECTIVE_FACTOR;
		
		ZONEP2(STACK(0)).pos.x += FROUND(FFRD_VEC.x * delta);
		ZONEP2(STACK(0)).pos.y += FROUND(FFRD_VEC.y * delta);
		
		/* IP touches points it affects... */
		TOUCH( ZONEP2(STACK(0)) );
		
		REG_SP--;
	}
	
	/* the LP register is reset after being used! */
	REG_LP = 1;
	
	return 0;
}

static int op_SHPIX(struct vm* this) {
	float delta;
	UINT32 i;
	
	if (REG_SP < REG_LP + 1) return FVMERR_STACK_UNDERFLOW;
	
	delta = (float) STACK(0);
	REG_SP--;
	
	/* loop over points */
	for (i = 0; i < REG_LP; i++) {
		ZONEP2(STACK(0)).pos.x += FROUND(FFRD_VEC.x * delta);
		ZONEP2(STACK(0)).pos.y += FROUND(FFRD_VEC.y * delta);
		
		/* SHPIX touches points it affects... */
		TOUCH( ZONEP2(STACK(0)) );
		
		REG_SP--;
	}
	
	return 0;
}

static int op_SHP_0(struct vm* this) {
	struct vec2f R, oR;	/* positions of reference point */
	float Rc, oRc;     	/* projected coordinates of reference point */
	float delta;
	
	UINT32 i;
	
	if (REG_SP < REG_LP) return FVMERR_STACK_UNDERFLOW;
	
	/* compute point coordinates along the projection vector (or dual
	 * projection vector, if regarding the original outline). */
	
	R.y = (float) ZONEP1(REG_RP2).pos.y;
	R.x = (float) ZONEP1(REG_RP2).pos.x;
	Rc = dot2f(FPRJ_VEC, R);
	
	oR.y = (float) OZONEP1(REG_RP2).pos.y;
	oR.x = (float) OZONEP1(REG_RP2).pos.x;
	oRc = dot2f(FDPRJ_VEC, oR);
	
	delta = (Rc - oRc) * PROJECTIVE_FACTOR;
	
	for (i = 0; i < REG_LP; i++) {
		ZONEP2(STACK(0)).pos.x += FROUND(FFRD_VEC.x * delta);
		ZONEP2(STACK(0)).pos.y += FROUND(FFRD_VEC.y * delta);
		
		/* SHP touches points it affects... */
		TOUCH( ZONEP2(STACK(0)) );
		
		REG_SP--;
	}
	
	/* the LP register is reset after being used! */
	REG_LP = 1;
	
	return 0;
}

static int op_SHP_1(struct vm* this) {
	int return_value;
	
	SWAP32( &REG_RP2, &REG_RP1 );
	SWAP32( &REG_ZP0, &REG_ZP1 );
	
	return_value = op_SHP_0(this);
	
	SWAP32( &REG_RP2, &REG_RP1 );
	SWAP32( &REG_ZP0, &REG_ZP1 );
	
	return return_value;
}

static int op_SHC_0(struct vm* this) {
	/* note: though undocumented, FreeType source code says that the
	 * twilight zone has a single (virtual) contour containing all
	 * points within it. */
	
	struct vec2f R, oR;	/* positions of reference point */
	float Rc, oRc;     	/* projected coordinates of reference point */
	float delta;
	
	UINT32 point;
	UINT32 i;
	
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) >= ZONER2.num_contours) return FVMERR_SEGMENTATION_FAULT;
	
	/* compute point coordinates along the projection vector (or dual
	 * projection vector, if regarding the original outline). */
	
	R.y = (float) ZONEP1(REG_RP2).pos.y;
	R.x = (float) ZONEP1(REG_RP2).pos.x;
	Rc = dot2f(FPRJ_VEC, R);
	
	oR.y = (float) OZONEP1(REG_RP2).pos.y;
	oR.x = (float) OZONEP1(REG_RP2).pos.x;
	oRc = dot2f(FDPRJ_VEC, oR);
	
	delta = (Rc - oRc) * PROJECTIVE_FACTOR;
	
	for (i = 0; i < CONTOUR_SIZE(STACK(0)); i++) {
		point = CONTOUR_START(STACK(0)) + i;
		
		if (point != REG_RP2) {
			ZONEP2(point).pos.x += FROUND(FFRD_VEC.x * delta);
			ZONEP2(point).pos.y += FROUND(FFRD_VEC.y * delta);
			
			/* SHC touches points it affects... */
			TOUCH( ZONEP2(point) );
		}
	}
	
	REG_SP--;
	
	return 0;
}

static int op_SHC_1(struct vm* this) {
	int return_value;
	
	SWAP32( &REG_RP2, &REG_RP1 );
	SWAP32( &REG_ZP0, &REG_ZP1 );
	
	return_value = op_SHC_0(this);
	
	SWAP32( &REG_RP2, &REG_RP1 );
	SWAP32( &REG_ZP0, &REG_ZP1 );
	
	return return_value;
}

static int op_SHZ_0(struct vm* this) {
	struct vec2f R, oR;	/* positions of reference point */
	float Rc, oRc;     	/* projected coordinates of reference point */
	float delta;
	
	UINT32 i;
	
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) > 1) return FVMERR_INVALID_ZONE;
	
	/* compute point coordinates along the projection vector (or dual
	 * projection vector, if regarding the original outline). */
	
	R.y = (float) ZONEP1(REG_RP2).pos.y;
	R.x = (float) ZONEP1(REG_RP2).pos.x;
	Rc = dot2f(FPRJ_VEC, R);
	
	oR.y = (float) OZONEP1(REG_RP2).pos.y;
	oR.x = (float) OZONEP1(REG_RP2).pos.x;
	oRc = dot2f(FDPRJ_VEC, oR);
	
	delta = (Rc - oRc) * PROJECTIVE_FACTOR;
	
	for (i = 0; i < POINTS_IN_ZONE(STACK(0)); i++) {
		ZONEP2(i).pos.x += FROUND(FFRD_VEC.x * delta);
		ZONEP2(i).pos.y += FROUND(FFRD_VEC.y * delta);
		
		/* note: according to FreeType source code, though
		 * undocumented, SHZ does not touch points... */
	}
	
	REG_SP--;
	
	return 0;
}

static int op_SHZ_1(struct vm* this) {
	int return_value;
	
	SWAP32( &REG_RP2, &REG_RP1 );
	SWAP32( &REG_ZP0, &REG_ZP1 );
	
	return_value = op_SHZ_0(this);
	
	SWAP32( &REG_RP2, &REG_RP1 );
	SWAP32( &REG_ZP0, &REG_ZP1 );
	
	return return_value;
}

static int op_MDAP_1(struct vm* this) {
	struct vec2f P;  	/* position of point */
	float rounded_Pc;	/* rounded coordinate */
	float Pc;        	/* coordinate */
	
	float delta;
	
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* compute point coordiante along projection vector. */
	P.x = (float) ZONEP0(STACK(0)).pos.x;
	P.y = (float) ZONEP0(STACK(0)).pos.y;
	Pc = dot2f(FPRJ_VEC, P);
	
	REG_RP0 = REG_RP1 = STACK(0);
	
	/* round coordinate acording to round state... */
	rounded_Pc = round_by_state(this, FROUND(Pc));
	
	delta = (rounded_Pc - Pc) * PROJECTIVE_FACTOR;
	
	ZONEP0(STACK(0)).pos.x += FROUND(FFRD_VEC.x * delta);
	ZONEP0(STACK(0)).pos.y += FROUND(FFRD_VEC.y * delta);
	
	/* MDAP touches the point it is given... */
	TOUCH( ZONEP0(STACK(0)) );
	
	REG_SP--;
	
	return 0;
}

static int op_MDAP_0(struct vm* this) {
	int return_value;
	
	/* this is the same as MDAP[1], but without rounding...
	 * so to save trouble, we will temporarily turn rounding off and
	 * call MDAP[1]. */
	
	/* note that when we set the round state specifically off (to ~0)
	 * we need not call update_round_state, and the state variables
	 * that would normally be set perserve their original values. */
	
	BYTE temp = ROUND_STATE;
	ROUND_STATE = ~0;
	
	return_value = op_MDAP_1(this);
	
	ROUND_STATE = temp;
	
	return return_value;
}

static int op_MIAP_0(struct vm* this) {
	struct vec2f P, oP;	/* positions of point */
	float Pc, oPc;     	/* coordinates of point */
	float cvt;         	/* value from control value table */
	
	float delta;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) >= MAX_CONTROL_ENTRIES) return FVMERR_SEGMENTATION_FAULT;
	
	/* compute point coordiante along projection vector. */
	P.y = (float) ZONEP0(STACK(1)).pos.y;
	P.x = (float) ZONEP0(STACK(1)).pos.x;
	Pc = dot2f(FPRJ_VEC, P);
	
	/* note that for MIAP[0] no rounding nor the control value cut
	 * in test takes place. */
	cvt = (float) CONTROL_VALUE(STACK(0));
	
	if (REG_ZP0 != 0) {
		oP.y = (float) OZONEP0(STACK(1)).pos.y;
		oP.x = (float) OZONEP0(STACK(1)).pos.x;
		oPc = dot2f(FDPRJ_VEC, oP);
		
		delta = (cvt - oPc) * PROJECTIVE_FACTOR;
		
		OZONEP0(STACK(1)).pos.x += FROUND(FFRD_VEC.x * delta);
		OZONEP0(STACK(1)).pos.y += FROUND(FFRD_VEC.y * delta);
	}
	
	/* for more information on what is going here see note 1. */
	delta = (cvt - Pc) * PROJECTIVE_FACTOR;
	
	ZONEP0(STACK(1)).pos.x += FROUND(FFRD_VEC.x * delta);
	ZONEP0(STACK(1)).pos.y += FROUND(FFRD_VEC.y * delta);
	
	/* set rp0 and rp1 to our point */
	REG_RP0 = REG_RP1 = STACK(1);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_MIAP_1(struct vm* this) {
	struct vec2f P, oP;	/* positions of point */
	float Pc, oPc;     	/* coordinates of point */
	float cvt;         	/* value from control value table */
	
	float delta;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) >= MAX_CONTROL_ENTRIES) return FVMERR_SEGMENTATION_FAULT;
	
	/* compute point coordiante along projection vector. */
	P.y = (float) ZONEP0(STACK(1)).pos.y;
	P.x = (float) ZONEP0(STACK(1)).pos.x;
	Pc = dot2f(FPRJ_VEC, P);
	
	/* if the difference between the value in the control value table
	 * and the graphics state variable single_width_value is less than
	 * the single_width_cut_in then we will use the single_width_value
	 * instead of the control value table entry. */
	cvt = CONTROL_VALUE(STACK(0));
	
	/* according to FreeType source code, if we are in the twilight
	 * zone then the cut-in test is not performed. */
	if (REG_ZP0 != 0) {
		if (ABS(cvt - Pc) > GRAPHICS_STATE.control_value_cut_in)
			cvt = Pc;
	}
	
	/* additionally, if this is the case then we wil set the original
	 * position of the point in the twilihgt zone to the value in the
	 * CVT. */
	else {
		oP.y = (float) OZONEP0(STACK(1)).pos.y;
		oP.x = (float) OZONEP0(STACK(1)).pos.x;
		oPc = dot2f(FPRJ_VEC, oP);
		
		delta = (cvt - oPc) * PROJECTIVE_FACTOR;
		
		OZONEP0(STACK(1)).pos.x += FROUND(FFRD_VEC.x * delta);
		OZONEP0(STACK(1)).pos.y += FROUND(FFRD_VEC.y * delta);
	}
	
	/* round value according to current setting of the round state and
	 * shift point. */
	delta = round_by_state(this, FROUND(cvt));
	delta = (delta - Pc) * PROJECTIVE_FACTOR;
	
	ZONEP0(STACK(1)).pos.x += FROUND(FFRD_VEC.x * delta);
	ZONEP0(STACK(1)).pos.y += FROUND(FFRD_VEC.y * delta);
	
	/* set rp0 and rp1 to our point */
	REG_RP0 = REG_RP1 = STACK(1);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_FDEF(struct vm* this) {
	UINT32 length;	/* length of proceedure */
	BYTE* ptr;    	/* pointer to start of proceedure */
	
	/* check we have a valid function id on the stack... */
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) + 1 > MAX_FUNCTION_DEFS) return FVMERR_SEGMENTATION_FAULT;
	
	ptr = &STREAM(1);
	
	/* get the length of our proceedure... */
	for (length = 0; STREAM(0) != ENDF; REG_EP++, length++)
	
	/* if we are redefining a function, free the old one. */
	if (FUNCTION(STACK(0)).stream)
		free(FUNCTION(STACK(0)).stream);
	
	/* add function */
	FUNCTION(STACK(0)).stream = smalloc(sizeof(BYTE), length);
	FUNCTION(STACK(0)).length = length;
	printf("FDEF %i (l %u)\n", STACK(0), FUNCTION(STACK(0)).length);
	
	memcpy(FUNCTION(STACK(0)).stream, ptr, length);
	
	REG_SP--;
	
	return 0;
}

static int op_IDEF(struct vm* this) {
	BYTE* ptr;    	/* pointer to start of proceedure */
	UINT32 length;	/* length of proceedure */
	int index;    	/* index where instruction is to be stored */
	
	/* check we have a valid opcode on the stack... */
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) + 1 > INSTR_MAX) return FVMERR_SEGMENTATION_FAULT;
	
	ptr = &STREAM(1);
	
	/* get the length of our proceedure... */
	for (length = 0; STREAM(0) != ENDF; REG_EP++, length++);
	
	/* get an index to store the instruction... */
	index = get_udef_index(this, STACK(0));
	
	REG_SP--;
	
	/* if a value of -1 was returned for the index then we are trying
	 * to override a built-in instruction. this not erroneous and may
	 * happen when an instruction is being written for backwards
	 * compatibility with older versions of TrueType (which should
	 * always be the case), but a newer one which already has the
	 * instruction is used. in this case we will simlpy return as if
	 * the instruction had succeeded. */
	if (index == -1) return 0;
	
	/* if we are redefining an instruction, free the old
	 * one. */
	if (USERDEF(index).stream)
		free(USERDEF(index).stream);
	
	/* add proceedure */
	USERDEF(index).stream = smalloc(sizeof(BYTE), length);
	memcpy(USERDEF(index).stream, ptr, length);
	
	return 0;
}

static int op_ENDF(struct vm* this) {
	CALL_STACK(0).repititions--;
	
	printf("%d\n", CALL_STACK(0).repititions);
	
	/* check if we still have more calls to complete, and if so then
	 * return to the start of the function. */
	if (CALL_STACK(0).repititions > 0) {
		REG_EP = 0;
		return 0;
	}
	
	/* set the execution pointer to the value sotred in the return
	 * register and switch our stream to its original value (the main
	 * execution stream). */
	REG_EP = CALL_STACK(0).return_position;
	this->stream_length = CALL_STACK(0).stream_length;
	this->stream = CALL_STACK(0).stream;
	
	REG_CSP--;
	
	return 0;
}

static int op_CALL(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (REG_CSP + 1 > MAX_CALL_DEPTH) return FVMERR_CALL_DEPTH_EXCEEDED;
	
	printf("CALL %i\n", STACK(0));
	
	/* if the function we are trying to call exists, switch our stream
	 * to the one which contains the function's body. */
	if (FUNCTION(STACK(0)).stream) {
		/* add a new element onto the call stack. */
		REG_CSP++;
		
		CALL_STACK(0).repititions = 1;
		CALL_STACK(0).return_position = REG_EP;
		CALL_STACK(0).stream_length = this->stream_length;
		CALL_STACK(0).stream = this->stream;
		
		this->stream_length = FUNCTION(STACK(0)).length;
		this->stream = FUNCTION(STACK(0)).stream;
		
		/* in this new stream, the execution is at position 0. */
		REG_EP = 0;
	}
	
	/* otherwise, if the function we are trying to call does not exist
	 * then return to indicate that an error has occurred. */
	else return FVMERR_SEGMENTATION_FAULT;
	
	REG_SP--;
	
	return 0;
}

/* not proud of this one... */
static int op_LOOPCALL(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	if (REG_CSP + 1 > MAX_CALL_DEPTH) return FVMERR_CALL_DEPTH_EXCEEDED;
	
	printf("CALL %i * %d\n", STACK(0), SSTACK(1));
	
	/* if the function we are trying to call exists, switch our stream
	 * to the one which contains the function's body. */
	if (FUNCTION(STACK(0)).stream) {
		/* add a new element onto the call stack. */
		REG_CSP++;
		
		CALL_STACK(0).repititions = SSTACK(1);
		CALL_STACK(0).return_position = REG_EP;
		CALL_STACK(0).stream_length = this->stream_length;
		CALL_STACK(0).stream = this->stream;
		
		this->stream_length = FUNCTION(STACK(0)).length;
		this->stream = FUNCTION(STACK(0)).stream;
		
		/* in this new stream, the execution is at position 0. */
		REG_EP = 0;
	}
	
	/* otherwise, if the function we are trying to call does not exist
	 * then return to indicate that an error has occurred. */
	else return FVMERR_SEGMENTATION_FAULT;
	
	REG_SP -= 2;
	
	return 0;
}

static int op_IF(struct vm* this) {
	int level = 0; /* keep track of nest level to find correct EIF */
	
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* if the value at the top of the stack is TRUE then we will take
	 * the TRUE path by simply resuming execution. */
	if (STACK(0)) BRANCH_DEPTH++;
	
	/* otherwise, we will need to find where to go (either locate the
	 * associated EIF or take the ELSE path). */
	else for (REG_EP++;; REG_EP++) {
		/* if we come across another IF then we will increment the
		 * current nest level. */
		if (STREAM(0) == IF) level++;
		
		/* if we are are the correct nesting level... */
		else if (level == 0) {
			/* ... and we find an ELSE, then take it. */ 
			if (STREAM(0) == ELSE) {
				BRANCH_DEPTH++;
				break;
			}
			
			/* ... and we find and EIF, then we are done. */
			else if (STREAM(0) == EIF) break;
		}
		
		/* if we are not at the correct nesting level and we
		 * encounter an EIF then we will decrement the current
		 * nesting level. */
		else if (STREAM(0) == EIF) level--;
	}
	
	printf("TAKING '%s' by %X\n\n", STACK(0) ? "TRUE" : "FALSE", STACK(0));
	
	REG_SP--;
	
	return 0;
}

static int op_ELSE(struct vm* this) {
	int level = 0; /* keep track of nest level to find correct EIF */
	
	/* if we are in a branch (have a non-zero branch level) and we
	 * enounter an ELSE statement then we need to find the associated
	 * EIF for this statement. */
	if (BRANCH_DEPTH > 0) {
		for (;; REG_EP++) {
			if (STREAM(0) == IF) level++;
			
			/* if we are are the correct nesting level and we
			 * find an EIF as the *next* instruction, then we
			 * are done. note that the next instruction to be
			 * executed will be the EIF, which will decrement
			 * our branch depth for us. */
			else if (level == 0 && STREAM(1) == EIF) break;
			
			/* if we are not at the correct nesting level and we
			 * encounter an EIF then we will decrement the current
			 * nesting level. */
			else if (STREAM(0) == EIF) level--;
		}
	}
	
	/* if we encounter an ELSE and we are not in a branch, then we will
	 * simply ignore it. */
	
	return 0;
}

static int op_EIF(struct vm* this) {
	BRANCH_DEPTH -= (BRANCH_DEPTH > 0);
	
	return 0;
}

static int op_MPS(struct vm* this) {
	if (REG_SP + 1 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP++;
	STACK(0) = POINT_SIZE;
	
	return 0;
}

static int op_MPPEM(struct vm* this) {
	float C;
	
	if (REG_SP + 1 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	C = PRJ_VEC.x * PRJ_VEC.x + PRJ_VEC.y * PRJ_VEC.y;
	
	REG_SP++;
	STACK(0) = FROUND(POINT_SIZE * fisqrt(C));
	
	return 0;
}

static int op_WCVTP(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	if (STACK(1) >= MAX_CONTROL_ENTRIES) return FVMERR_SEGMENTATION_FAULT;
	
	printf("%u\n", STACK(1));
	
	CONTROL_VALUE(STACK(1) + 1) = SSTACK(0);
	REG_SP -= 2;
	
	return 0;
}

static int op_WCVTF(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	if (STACK(1) >= MAX_CONTROL_ENTRIES) return FVMERR_SEGMENTATION_FAULT;
	
	CONTROL_VALUE(STACK(1) + 1) = FUTOPIXELS(SSTACK(0));
	REG_SP -= 2;
	
	return 0;
}

static int op_RCVT(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) >= MAX_CONTROL_ENTRIES) return FVMERR_SEGMENTATION_FAULT;
	
	printf("RC:%u %u\n", MAX_CONTROL_ENTRIES, STACK(0));
	
	SSTACK(0) = CONTROL_VALUE(STACK(0));
	
	return 0;
}

static int op_ALIGNRP(struct vm* this) {
	struct vec2f P;	/* point */
	float delta;
	
	UINT32 i;
	
	if (REG_SP < REG_LP) return FVMERR_STACK_UNDERFLOW;
	
	for (i = 0; i < REG_LP; i++) {
		P.x = (float) (ZONEP0(REG_RP0).pos.x - ZONEP1(STACK(0)).pos.x);
		P.y = (float) (ZONEP0(REG_RP0).pos.y - ZONEP1(STACK(0)).pos.y);
		
		delta = dot2f(FPRJ_VEC, P) * PROJECTIVE_FACTOR;
		
		ZONEP1(STACK(0)).pos.x += FROUND(FFRD_VEC.x * delta);
		ZONEP1(STACK(0)).pos.y += FROUND(FFRD_VEC.y * delta);
		
		TOUCH( ZONEP1(STACK(0)) );
		
		REG_SP--;
	}
	
	/* the LP register is reset after being used! */
	REG_LP = 1;
	
	return 0;
}

static int op_ALIGNPTS(struct vm* this) {
	struct vec32fx E;	/* delta vector */
	struct vec2f P;  	/* point */
	
	float delta;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	P.x = (float) (ZONEP0(STACK(0)).pos.x - ZONEP1(STACK(1)).pos.x);
	P.y = (float) (ZONEP0(STACK(0)).pos.y - ZONEP1(STACK(1)).pos.y);
	
	delta = dot2f(FPRJ_VEC, P) * 0.5 * PROJECTIVE_FACTOR;
	
	E.x = FROUND(FFRD_VEC.x * delta);
	E.y = FROUND(FFRD_VEC.y * delta);
	
	ZONEP0(STACK(0)).pos.x -= E.x;
	ZONEP0(STACK(0)).pos.y -= E.y;
	
	TOUCH( ZONEP0(STACK(0)) );
	
	ZONEP1(STACK(1)).pos.x += E.x;
	ZONEP1(STACK(1)).pos.y += E.y;
	
	TOUCH( ZONEP1(STACK(1)) );
	
	REG_SP -= 2;
	
	return 0;
}

static int op_SVTCA_0(struct vm* this) {
	PRJ_VEC.y = 16384;
	PRJ_VEC.x = 0;
	FRD_VEC.y = 16384;
	FRD_VEC.x = 0;
	
	DPRJ_VEC.y = PRJ_VEC.y;
	DPRJ_VEC.x = PRJ_VEC.x;
	
	update_floating_vectors(this);
	
	return 0;
}

static int op_SVTCA_1(struct vm* this) {
	PRJ_VEC.x = 16384;
	PRJ_VEC.y = 0;
	FRD_VEC.x = 16384;
	FRD_VEC.y = 0;
	
	DPRJ_VEC.y = PRJ_VEC.y;
	DPRJ_VEC.x = PRJ_VEC.x;
	
	update_floating_vectors(this);
	
	return 0;
}

static int op_SPVTCA_0(struct vm* this) {
	PRJ_VEC.y = 16384;
	PRJ_VEC.x = 0;
	
	DPRJ_VEC.y = PRJ_VEC.y;
	DPRJ_VEC.x = PRJ_VEC.x;
	
	update_floating_vectors(this);
	
	return 0;
}

static int op_SPVTCA_1(struct vm* this) {
	PRJ_VEC.x = 16384;
	PRJ_VEC.y = 0;
	
	DPRJ_VEC.y = PRJ_VEC.y;
	DPRJ_VEC.x = PRJ_VEC.x;
	
	update_floating_vectors(this);
	
	return 0;
}

static int op_SFVTCA_0(struct vm* this) {
	FRD_VEC.y = 16384;
	FRD_VEC.x = 0;
	
	update_floating_vectors(this);
	
	return 0;
}

static int op_SFVTCA_1(struct vm* this) {
	FRD_VEC.x = 16384;
	FRD_VEC.y = 0;
	
	update_floating_vectors(this);
	
	return 0;
}

static int op_SDPVTL_0(struct vm* this) {
	struct vec32fx L;
	float Lx, Ly;
	float imag;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	L.x = ZONEP1(STACK(1)).pos.x - ZONEP2(STACK(0)).pos.x;
	L.y = ZONEP1(STACK(1)).pos.y - ZONEP2(STACK(0)).pos.y;
	
	if (L.x == 0 && L.y == 0) return FVMERR_DIVIDE_BY_ZERO;
	
	Lx = (float) (L.x);
	Ly = (float) (L.y);
	
	imag = fisqrt(Lx * Lx + Ly * Ly) * 16384;
	
	DPRJ_VEC.y = (F2D14) FROUND(Ly * imag);
	DPRJ_VEC.x = (F2D14) FROUND(Lx * imag);
	
	update_floating_vectors(this);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_SDPVTL_1(struct vm* this) {
	struct vec32fx L;
	float Lx, Ly;
	float imag;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	L.x = ZONEP1(STACK(1)).pos.x - ZONEP2(STACK(0)).pos.x;
	L.y = ZONEP1(STACK(1)).pos.y - ZONEP2(STACK(0)).pos.y;
	
	if (L.x == 0 && L.y == 0) return FVMERR_DIVIDE_BY_ZERO;
	
	Lx = (float) (-L.y);
	Ly = (float) (L.x);
	
	imag = fisqrt(Lx * Lx + Ly * Ly) * 16384;
	
	DPRJ_VEC.y = (F2D14) FROUND(Ly * imag);
	DPRJ_VEC.x = (F2D14) FROUND(Lx * imag);
	
	update_floating_vectors(this);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_SPVTL_0(struct vm* this) {
	struct vec32fx L;
	float Lx, Ly;
	float imag;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	L.x = ZONEP1(STACK(1)).pos.x - ZONEP2(STACK(0)).pos.x;
	L.y = ZONEP1(STACK(1)).pos.y - ZONEP2(STACK(0)).pos.y;
	
	if (L.x == 0 && L.y == 0) return FVMERR_DIVIDE_BY_ZERO;
	
	Lx = (float) (L.x);
	Ly = (float) (L.y);
	
	imag = fisqrt(Lx * Lx + Ly * Ly) * 16384;
	
	PRJ_VEC.y = (F2D14) FROUND(Ly * imag);
	PRJ_VEC.x = (F2D14) FROUND(Lx * imag);
	
	DPRJ_VEC.y = PRJ_VEC.y;
	DPRJ_VEC.x = PRJ_VEC.x;
	
	update_floating_vectors(this);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_SPVTL_1(struct vm* this) {
	struct vec32fx L;
	float Lx, Ly;
	float imag;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	L.x = ZONEP1(STACK(1)).pos.x - ZONEP2(STACK(0)).pos.x;
	L.y = ZONEP1(STACK(1)).pos.y - ZONEP2(STACK(0)).pos.y;
	
	if (L.x == 0 && L.y == 0) return FVMERR_DIVIDE_BY_ZERO;
	
	Lx = (float) (-L.y);
	Ly = (float) (L.x);
	
	imag = fisqrt(Lx * Lx + Ly * Ly) * 16384;
	
	PRJ_VEC.y = (F2D14) FROUND(Ly * imag);
	PRJ_VEC.x = (F2D14) FROUND(Lx * imag);
	
	DPRJ_VEC.y = PRJ_VEC.y;
	DPRJ_VEC.x = PRJ_VEC.x;
	
	update_floating_vectors(this);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_SFVTL_0(struct vm* this) {
	struct vec32fx L;
	float Lx, Ly;
	float imag;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	L.x = ZONEP1(STACK(1)).pos.x - ZONEP2(STACK(0)).pos.x;
	L.y = ZONEP1(STACK(1)).pos.y - ZONEP2(STACK(0)).pos.y;
	
	if (L.x == 0 && L.y == 0) return FVMERR_DIVIDE_BY_ZERO;
	
	Lx = (float) (L.x);
	Ly = (float) (L.y);
	
	imag = fisqrt(Lx * Lx + Ly * Ly) * 16384;
	
	FRD_VEC.y = (F2D14) FROUND(Ly * imag);
	FRD_VEC.x = (F2D14) FROUND(Lx * imag);
	
	update_floating_vectors(this);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_SFVTL_1(struct vm* this) {
	struct vec32fx L;
	float Lx, Ly;
	float imag;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	L.x = ZONEP1(STACK(1)).pos.x - ZONEP2(STACK(0)).pos.x;
	L.y = ZONEP1(STACK(1)).pos.y - ZONEP2(STACK(0)).pos.y;
	
	if (L.x == 0 && L.y == 0) return FVMERR_DIVIDE_BY_ZERO;
	
	Lx = (float) (-L.y);
	Ly = (float) (L.x);
	
	imag = fisqrt(Lx * Lx + Ly * Ly) * 16384;
	
	FRD_VEC.y = (F2D14) FROUND(Ly * imag);
	FRD_VEC.x = (F2D14) FROUND(Lx * imag);
	
	update_floating_vectors(this);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_SFVTPV(struct vm* this) {
	FRD_VEC.y = PRJ_VEC.y;
	FRD_VEC.x = PRJ_VEC.x;
	
	update_floating_vectors(this);
	
	return 0;
}

static int op_SPVFS(struct vm* this) {
	float Px, Py;
	float imag;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) == 0 && STACK(1) == 0) return FVMERR_DIVIDE_BY_ZERO;
	
	Px = (float) (INT16) SSTACK(1);
	Py = (float) (INT16) SSTACK(0);
	
	imag = fisqrt(Px * Px + Py * Py) * 16384;
	
	PRJ_VEC.y = FROUND(Py * imag);
	PRJ_VEC.x = FROUND(Px * imag);
	
	DPRJ_VEC.y = PRJ_VEC.y;
	DPRJ_VEC.x = PRJ_VEC.x;
	
	update_floating_vectors(this);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_SFVFS(struct vm* this) {
	float Fx, Fy;
	float imag;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) == 0 && STACK(1) == 0) return FVMERR_DIVIDE_BY_ZERO;
	
	Fx = (float) (INT16) STACK(1);
	Fy = (float) (INT16) STACK(0);
	
	imag = fisqrt(Fx * Fx + Fy * Fy) * 16384;
	
	FRD_VEC.y = FROUND(Fy * imag);
	FRD_VEC.x = FROUND(Fx * imag);
	
	update_floating_vectors(this);
	
	return 0;
}

static int op_S45ROUND(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	ROUND_STATE = STACK(0);
	ROUND_GRID_PERIOD = 45;
	update_round_state(this);
	
	return 0;
}

static int op_SROUND(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	ROUND_STATE = STACK(0);
	ROUND_GRID_PERIOD = 64;
	update_round_state(this);
	
	return 0;
}

static int op_PUSHB_0(struct vm* this) {
	if (REG_EP + 1 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 1 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP++;
	
	USTACK(0) = STREAM(1);
	
	REG_EP++;
	
	return 0;
}

static int op_PUSHB_1(struct vm* this) {
	if (REG_EP + 2 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 2 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 2;
	
	USTACK(0) = STREAM(2);
	USTACK(1) = STREAM(1);
	
	REG_EP += 2;
	
	return 0;
}

static int op_PUSHB_2(struct vm* this) {
	if (REG_EP + 3 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 3 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 3;
	
	USTACK(0) = STREAM(3);
	USTACK(1) = STREAM(2);
	USTACK(2) = STREAM(1);
	
	REG_EP += 3;
	
	return 0;
}

static int op_PUSHB_3(struct vm* this) {
	if (REG_EP + 4 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 4 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 4;
	
	USTACK(0) = STREAM(4);
	USTACK(1) = STREAM(3);
	USTACK(2) = STREAM(2);
	USTACK(3) = STREAM(1);
	
	REG_EP += 4;
	
	return 0;
}

static int op_PUSHB_4(struct vm* this) {
	if (REG_EP + 5 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 5 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 5;
	
	USTACK(0) = STREAM(5);
	USTACK(1) = STREAM(4);
	USTACK(2) = STREAM(3);
	USTACK(3) = STREAM(2);
	USTACK(4) = STREAM(1);
	
	REG_EP += 5;
	
	return 0;
}

static int op_PUSHB_5(struct vm* this) {
	if (REG_EP + 6 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 6 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 6;
	
	USTACK(0) = STREAM(6);
	USTACK(1) = STREAM(5);
	USTACK(2) = STREAM(4);
	USTACK(3) = STREAM(3);
	USTACK(4) = STREAM(2);
	USTACK(5) = STREAM(1);
	
	REG_EP += 6;
	
	return 0;
}

static int op_PUSHB_6(struct vm* this) {
	if (REG_EP + 7 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 7 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 7;
	
	USTACK(0) = STREAM(7);
	USTACK(1) = STREAM(6);
	USTACK(2) = STREAM(5);
	USTACK(3) = STREAM(4);
	USTACK(4) = STREAM(3);
	USTACK(5) = STREAM(2);
	USTACK(6) = STREAM(1);
	
	REG_EP += 7;
	
	return 0;
}

static int op_PUSHB_7(struct vm* this) {
	if (REG_EP + 8 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 8 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 8;
	
	USTACK(0) = STREAM(8);
	USTACK(1) = STREAM(7);
	USTACK(2) = STREAM(6);
	USTACK(3) = STREAM(5);
	USTACK(4) = STREAM(4);
	USTACK(5) = STREAM(3);
	USTACK(6) = STREAM(2);
	USTACK(7) = STREAM(1);
	
	REG_EP += 8;
	
	return 0;
}

static int op_PUSHW_0(struct vm* this) {
	if (REG_EP + 2 * 1 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 1 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP++;
	
	USTACK(0) = STREAM(1) << 8;
	USTACK(0) |= STREAM(2);
	
	REG_EP += 2;
	
	return 0;
}

static int op_PUSHW_1(struct vm* this) {
	if (REG_EP + 2 * 2 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 2 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 2;
	
	USTACK(0) = STREAM(3) << 8;
	USTACK(0) |= STREAM(4);
	
	USTACK(1) = STREAM(1) << 8;
	USTACK(1) |= STREAM(2);
	
	REG_EP += 4;
	
	return 0;
}

static int op_PUSHW_2(struct vm* this) {
	if (REG_EP + 2 * 3 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 3 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 3;
	
	USTACK(0) = STREAM(5) << 8;
	USTACK(0) |= STREAM(6);
	
	USTACK(1) = STREAM(3) << 8;
	USTACK(1) |= STREAM(4);
	
	USTACK(2) = STREAM(1) << 8;
	USTACK(2) |= STREAM(2);
	
	REG_EP += 6;
	
	return 0;
}

static int op_PUSHW_3(struct vm* this) {
	if (REG_EP + 2 * 4 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 4 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 4;
	
	USTACK(0) = STREAM(7) << 8;
	USTACK(0) |= STREAM(8);
	
	USTACK(1) = STREAM(5) << 8;
	USTACK(1) |= STREAM(6);
	
	USTACK(2) = STREAM(3) << 8;
	USTACK(2) |= STREAM(4);
	
	USTACK(3) = STREAM(1) << 8;
	USTACK(3) |= STREAM(2);
	
	REG_EP += 8;
	
	return 0;
}

static int op_PUSHW_4(struct vm* this) {
	if (REG_EP + 2 * 5 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 5 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 5;
	
	USTACK(0) = STREAM(9) << 8;
	USTACK(0) |= STREAM(10);
	
	USTACK(1) = STREAM(7) << 8;
	USTACK(1) |= STREAM(8);
	
	USTACK(2) = STREAM(5) << 8;
	USTACK(2) |= STREAM(6);
	
	USTACK(3) = STREAM(3) << 8;
	USTACK(3) |= STREAM(4);
	
	USTACK(4) = STREAM(1) << 8;
	USTACK(4) |= STREAM(2);
	
	REG_EP += 10;
	
	return 0;
}

static int op_PUSHW_5(struct vm* this) {
	if (REG_EP + 2 * 6 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 6 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 6;
	
	USTACK(0) = STREAM(11) << 8;
	USTACK(0) |= STREAM(12);
	
	USTACK(1) = STREAM(9) << 8;
	USTACK(1) |= STREAM(10);
	
	USTACK(2) = STREAM(7) << 8;
	USTACK(2) |= STREAM(8);
	
	USTACK(3) = STREAM(5) << 8;
	USTACK(3) |= STREAM(6);
	
	USTACK(4) = STREAM(3) << 8;
	USTACK(4) |= STREAM(4);
	
	USTACK(5) = STREAM(1) << 8;
	USTACK(5) |= STREAM(2);
	
	REG_EP += 12;
	
	return 0;
}

static int op_PUSHW_6(struct vm* this) {
	if (REG_EP + 2 * 7 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 7 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 7;
	
	USTACK(0) = STREAM(13) << 8;
	USTACK(0) |= STREAM(14);
	
	USTACK(1) = STREAM(11) << 8;
	USTACK(1) |= STREAM(12);
	
	USTACK(2) = STREAM(9) << 8;
	USTACK(2) |= STREAM(10);
	
	USTACK(3) = STREAM(7) << 8;
	USTACK(3) |= STREAM(8);
	
	USTACK(4) = STREAM(5) << 8;
	USTACK(4) |= STREAM(6);
	
	USTACK(5) = STREAM(3) << 8;
	USTACK(5) |= STREAM(4);
	
	USTACK(6) = STREAM(1) << 8;
	USTACK(6) |= STREAM(2);
	
	REG_EP += 14;
	
	return 0;
}

static int op_PUSHW_7(struct vm* this) {
	if (REG_EP + 2 * 8 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + 8 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 8;
	
	USTACK(0) = STREAM(15) << 8;
	USTACK(0) |= STREAM(16);
	
	USTACK(1) = STREAM(13) << 8;
	USTACK(1) |= STREAM(14);
	
	USTACK(2) = STREAM(11) << 8;
	USTACK(2) |= STREAM(12);
	
	USTACK(3) = STREAM(9) << 8;
	USTACK(3) |= STREAM(10);
	
	USTACK(4) = STREAM(7) << 8;
	USTACK(4) |= STREAM(8);
	
	USTACK(5) = STREAM(5) << 8;
	USTACK(5) |= STREAM(6);
	
	USTACK(6) = STREAM(3) << 8;
	USTACK(6) |= STREAM(4);
	
	USTACK(7) = STREAM(1) << 8;
	USTACK(7) |= STREAM(2);
	
	REG_EP += 16;
	
	return 0;
}

static int op_SRP_0(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	REG_RP0 = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SRP_1(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	REG_RP1 = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SRP_2(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	REG_RP2 = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SZP_0(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) > 1) return FVMERR_INVALID_ZONE;
	
	REG_ZP0 = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SZP_1(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) > 1) return FVMERR_INVALID_ZONE;
	
	REG_ZP1 = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SZP_2(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) > 1) return FVMERR_INVALID_ZONE;
	
	REG_ZP2 = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SZPS(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) > 1) return FVMERR_INVALID_ZONE;
	
	REG_ZP0 = STACK(0);
	REG_ZP1 = STACK(0);
	REG_ZP2 = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_ROUND_0(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* grey distances recieve no engine compensation... */
	SSTACK(0) = round_by_state(this, SSTACK(0));
	
	return 0;
}

static int op_ROUND_1(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* black distances shrink... */
	SSTACK(0) = round_by_state(this, apply_compensation(SSTACK(0), 1));
	
	return 0;
}

static int op_ROUND_2(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* white distances grow... */
	SSTACK(0) = round_by_state(this, apply_compensation(SSTACK(0), 2));
	
	return 0;
}

static int op_ROUND_3(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* there are only three distance types... so this is reserved...
	 * perhaps... ??? */
	SSTACK(0) = round_by_state(this, SSTACK(0));
	
	return 0;
}

static int op_NROUND_0(struct vm* this) {
	/* grey distances recieve no engine compensation, so we just do
	 * literally nothing. */
	
	return 0;
}

static int op_NROUND_1(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* black distances shrink... */
	SSTACK(0) = apply_compensation(SSTACK(0), 1);
	
	return 0;
}

static int op_NROUND_2(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* white distances grow... */
	SSTACK(0) = apply_compensation(SSTACK(0), 2);
	
	return 0;
}

static int op_NROUND_3(struct vm* this) {
	/* there are three distance types (white, grey, black)...
	 * so this is reserved? I guess its so distance can be
	 * expressed in two bits, leaving an unsed option... */
	
	return 0;
}

static int op_UTP(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	if (FRD_VEC.y != 0)
		UNTOUCHY( ZONEP0(STACK(0)) );
	
	if (FRD_VEC.x != 0)
		UNTOUCHX( ZONEP0(STACK(0)) );
	
	REG_SP--;
	
	return 0;
}

static int op_SSWCI(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	GRAPHICS_STATE.single_width_cut_in = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SSW(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	GRAPHICS_STATE.single_width_value = FUTOPIXELS(STACK(0));
	REG_SP--;
	
	return 0;
}

static int op_SMD(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	GRAPHICS_STATE.minimum_distance = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SLOOP(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	REG_LP = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SDS(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	GRAPHICS_STATE.delta_shift = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SDB(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	GRAPHICS_STATE.delta_base = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SCVTCI(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	GRAPHICS_STATE.control_value_cut_in = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_GC_0(struct vm* this) {
	struct vec2f P;	/* position of point */
	
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	P.y = (float) ZONEP2(STACK(0)).pos.y;
	P.x = (float) ZONEP2(STACK(0)).pos.x;
	
	SSTACK(0) = FROUND(dot2f(FPRJ_VEC, P));
	
	return 0;
}

static int op_GC_1(struct vm* this) {
	struct vec2f oP;	/* original position of point */
	
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	oP.y = (float) OZONEP2(STACK(0)).pos.y;
	oP.x = (float) OZONEP2(STACK(0)).pos.x;
	
	SSTACK(0) = FROUND(dot2f(FDPRJ_VEC, oP));
	
	return 0;
}

static int op_SCFS(struct vm* this) {
	struct vec2f P;	/* point */
	float C;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	P.y = (float) ZONEP2(STACK(1)).pos.y;
	P.x = (float) ZONEP2(STACK(1)).pos.x;
	
	/* to set the projective coordinate of our point we will calculate
	 * a delta by subtracting where we want it to be from where it is.
	 * we calaculte where it is by taking the dot product between the
	 * coordinate of the point and the projection vector. this works on
	 * a simplification of the scalar projection formula since the
	 * projection vector is normalised.
	 * 
	 * let Z be the scalar projection of a vector A on a vector B.
	 * then,
	 * 
	 * Z = (A . B) / (B . B) = (A . B) / ||B||^2
	 * 
	 * but since B is normalised, its magnitude, ||B|| = 1. thus,
	 * 
	 * Z = (A . B) / 1^2 = (A . B) / 1 = A . B
	 * 
	 */
	C = (float) SSTACK(0) - dot2f(FPRJ_VEC, P);
	
	/* now, since points move along the freedom vector and in order to
	 * maintain distance along the proejction vector, we need to
	 * project our delta vector (C * PRJ_VEC) onto the freedom vector.
	 * this can be done as follows:
	 * 
	 * let C denote our coordinate, X denote the origin, P denote the
	 * projection vector, and F denote the freedom vector, T denote
	 * the angle between the freedom and projection vector, A the
	 * adjacent to this angle, and O the opposite to it. then we
	 * have the following:
	 * 
	 *                               
	 *         |- C --|   /          
	 *         :      :  /           
	 *         :      : /            
	 *         : A    :/             
	 * P ------:------X------------- 
	 *         :   ( /               
	 *        O:  T /                
	 *         :   /                 
	 *         :  /H                 
	 *         : /                   
	 *         :/                    
	 *         /                     
	 *       F                       
	 *                               
	 * 
	 * note that A = C.
	 * 
	 * using primary trigonometric ratios we can calculate H, which
	 * will be the distance along the freedom vector that preserves
	 * our coordinate along the projection vector:
	 * 
	 * cos(T) = A / H = C / H
	 *   -> H = C / cos(T)
	 * 
	 * then, given that for any vectors A and B, and an angle between
	 * them T,
	 * 
	 * A . B = ||A|| ||B|| cos(T)
	 *   -> cos(T) = (A . B) / (||A|| ||B||)
	 * 
	 * we arrive at the following:
	 * 
	 * H = C / cos(T) = C / [(P . F) / (||P|| ||F||)]
	 *   = C / [(P . F) / (1 * 1)]
	 *   = C / (P . F)
	 * 
	 * note that P . F is pre-computed and stored as PROJECTIVE_FACTOR.
	 * 
	 */
	C *= PROJECTIVE_FACTOR;
	
	/* now we add C times the freedom vector to our original point. */
	ZONEP2(STACK(1)).pos.x += FROUND(FFRD_VEC.x * C);
	ZONEP2(STACK(1)).pos.y += FROUND(FFRD_VEC.y * C);
	
	TOUCH( ZONEP2(STACK(1)) );
	
	/* FreeType source code says though it is undocumented, this
	 * command changes the original position of points in the twilight
	 * zone. */
	if (REG_ZP2 == 0)
		OZONEP2(STACK(1)).pos = ZONEP2(STACK(1)).pos;
	
	REG_SP -= 2;
	
	return 0;
}

static int op_SANGW(struct vm* this) {
	/* anachronistic */
	REG_SP -= (REG_SP > 0);
	
	return 0;
}

static int op_RUTG(struct vm* this) {
	ROUND_STATE = 0x40;
	update_round_state(this);
	
	return 0;
}

static int op_RTHG(struct vm* this) {
	ROUND_STATE = 0x68;
	update_round_state(this);
	
	return 0;
}

static int op_RTG(struct vm* this) {
	ROUND_STATE = 0x48;
	update_round_state(this);
	
	return 0;
}

static int op_RTDG(struct vm* this) {
	ROUND_STATE = 0x08;
	update_round_state(this);
	
	return 0;
}

static int op_ROFF(struct vm* this) {
	ROUND_STATE = ~0;
	
	return 0;
}

static int op_RDTG(struct vm* this) {
	ROUND_STATE = 0x44;
	update_round_state(this);
	
	return 0;
}

static int op_JROT(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	if (REG_EP + SSTACK(1) - 1 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_EP + SSTACK(1) < 1) return FVMERR_EXECUTION_UNDERFLOW;
	
	if (STACK(0)) REG_EP += SSTACK(1) - 1;
	
	REG_SP -= 2;
	
	return 0;
}

static int op_JROF(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	if (REG_EP + SSTACK(1) - 1 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_EP + SSTACK(1) < 1) return FVMERR_EXECUTION_UNDERFLOW;
	
	if (!STACK(0)) REG_EP += SSTACK(1) - 1;
	
	REG_SP -= 2;
	
	return 0;
}

static int op_JMPR(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (REG_EP + SSTACK(0) - 1 > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_EP + SSTACK(0) < 1) return FVMERR_EXECUTION_UNDERFLOW;
	
	REG_EP += SSTACK(0) - 1;
	REG_SP--;
	
	return 0;
}

static int op_ISECT(struct vm* this) {
	if (REG_SP < 5) return FVMERR_STACK_UNDERFLOW;
	
	/* intersection of lines A = A1 -> A0 and B = B1 -> B0 at point P
	 * is calculated in the following way:
	 * 
	 * P = ( (A0 + A1) / 2 + (B0 + B1) / 2 ) / 2;
	 * 
	 */
	
	ZONEP2(STACK(4)).pos.x = (
		(ZONEP0(STACK(0)).pos.x + ZONEP0(STACK(1)).pos.x) / 2 +
		(ZONEP1(STACK(2)).pos.x + ZONEP1(STACK(3)).pos.x) / 2
	) / 2;
	
	ZONEP2(STACK(4)).pos.y = (
		(ZONEP0(STACK(0)).pos.y + ZONEP0(STACK(1)).pos.y) / 2 +
		(ZONEP1(STACK(2)).pos.y + ZONEP1(STACK(3)).pos.y) / 2
	) / 2;
	
	TOUCHX( ZONEP2(STACK(4)) );
	TOUCHY( ZONEP2(STACK(4)) );
	
	REG_SP -= 5;
	
	return 0;
}

static int op_GPV(struct vm* this) {
	if (REG_SP + 2 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 2;
	SSTACK(0) = PRJ_VEC.y;
	SSTACK(1) = PRJ_VEC.x;
	
	return 0;
}

static int op_GFV(struct vm* this) {
	if (REG_SP + 2 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += 2;
	SSTACK(0) = FRD_VEC.y;
	SSTACK(1) = FRD_VEC.x;
	
	return 0;
}

static int op_FLIPRGON(struct vm* this) {
	UINT32 i;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	/* set points numbered STACK(1) to STACK(0) to on curve. */
	for (i = STACK(1); i < STACK(0); i++)
		ZONEP0(i).flags |= POINT_MASK_ONCURVE;
	
	return 0;
}

static int op_FLIPRGOFF(struct vm* this) {
	UINT32 i;
	
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	/* set points numbered STACK(1) to STACK(0) to off curve. */
	for (i = STACK(1); i < STACK(0); i++)
		ZONEP0(i).flags &= ~POINT_MASK_ONCURVE;
	
	return 0;
}

static int op_FLIPPT(struct vm* this) {
	UINT32 i;
	
	if (REG_SP < REG_LP) return FVMERR_STACK_UNDERFLOW;
	
	/* read REG_LP point numbers from the stack... */
	for (i = 0; i < REG_LP; i++) {
		/* if the point is an on curve one, make it an off curve
		 * one, or if it is off curve make it on curve. */
		if (ZONEP0(STACK(0)).flags & POINT_MASK_ONCURVE) {
			ZONEP0(STACK(0)).flags &= ~POINT_MASK_ONCURVE;
		} else {
			ZONEP0(STACK(0)).flags |= POINT_MASK_ONCURVE;
		}
		
		REG_SP--;
	}
	
	/* the LP register is reset after being used! */
	REG_LP = 1;
	
	return 0;
}

static int op_FLIPON(struct vm* this) {
	GRAPHICS_STATE.flags.auto_flip = 1;
	
	return 0;
}

static int op_FLIPOFF(struct vm* this) {
	GRAPHICS_STATE.flags.auto_flip = 0;
	
	return 0;
}

static int op_DEBUG(struct vm* this) {
	/* pop, nothing else */
	REG_SP -= (REG_SP > 0);
	
	return 0;
}

static int op_AA(struct vm* this) {
	/* anachronistic */
	REG_SP -= (REG_SP > 0);
	
	return 0;
}

static int op_NPUSHW(struct vm* this) {
	BYTE N = STREAM(1);
	BYTE i;
	
	REG_EP++;
	
	/* make sure there is room on the stack and in the instruction
	 * stream for the data. */
	if (REG_EP + N > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + N > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += N;
	
	/* push elements */
	for (i = 0; i < N; i++) {
		STACK(N - i - 1) = STREAM(2 * i + 1) << 8;
		STACK(N - i - 1) |= STREAM(2 * i + 2);
	}
	
	/* advance execution */
	REG_EP += 2 * N;
	
	return 0;
}

static int op_NPUSHB(struct vm* this) {
	BYTE N = STREAM(1);
	BYTE i;
	
	REG_EP++;
	
	/* make sure there is room on the stack and in the instruction
	 * stream for the data. */
	if (REG_EP + N > MAX_STREAM) return FVMERR_EXECUTION_OVERFLOW;
	if (REG_SP + N > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP += N;
	
	/* push elements */
	for (i = 0; i < N; i++)
		STACK(N - i - 1) = STREAM(i + 1);
	
	/* advance execution */
	REG_EP += N;
	
	return 0;
}

static int op_WS(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	if (STACK(1) >= MAX_STORAGE) return FVMERR_SEGMENTATION_FAULT;
	
	MEMORY(STACK(1)) = STACK(0);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_RS(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) >= MAX_STORAGE) return FVMERR_SEGMENTATION_FAULT;
	
	STACK(0) = MEMORY(STACK(0));
	
	return 0;
}

static int op_NEG(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	SSTACK(0) = -SSTACK(0);
	
	return 0;
}

static int op_MIN(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	if (SSTACK(0) < SSTACK(1))
		STACK(1) = STACK(0);
	
	REG_SP--;
	
	return 0;
}

static int op_MAX(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	if (SSTACK(0) > SSTACK(1))
		STACK(1) = STACK(0);
	
	REG_SP--;
	
	return 0;
}

static int op_ABS(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	SSTACK(0) = ABS(SSTACK(0));
	
	return 0;
}

static int op_FLOOR(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* the last six bits of a fixed 26.6 number represent its
	 * fractional part, so to floor we just set these bits to zero. 
	 * note this works for negative numbers only if they are
	 * represented using twos complement, which we will assume,
	 * even though the C standard permits otherwise. */
	STACK(0) &= ~F26D6_FRAC_MASK;
	
	return 0;
}

static int op_CEILING(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* what is essentially being done here is add 0.999 and floor
	 * (but with 26.6 fixed point). this is so integral values are
	 * unaffected. */
	SSTACK(0) += 63;
	STACK(0) &= ~F26D6_FRAC_MASK;
	
	return 0;
}

static int op_DIV(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	SSTACK(1) = F26D6DIV(SSTACK(1), SSTACK(0));
	REG_SP--;
	
	return 0;
}

static int op_MUL(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	SSTACK(1) = F26D6MUL(SSTACK(1), SSTACK(0));
	REG_SP--;
	
	return 0;
}

static int op_SUB(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	SSTACK(1) = SSTACK(1) - SSTACK(0);
	REG_SP--;
	
	return 0;
}

static int op_ADD(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	SSTACK(1) = SSTACK(1) + SSTACK(0);
	REG_SP--;
	
	return 0;
}

static int op_NOT(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* a triple! */
	STACK(0) = !!!STACK(0);
	
	return 0;
}

static int op_OR(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	STACK(1) = STACK(1) || STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_AND(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	STACK(1) = STACK(1) && STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_EVEN(struct vm* this) {
	F26D6 val;
	
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* round value according to round state and then truncate to the
	 * nearest integer before checking if it is even or not. */
	val = round_by_state(this, STACK(0));
	STACK(0) = !( (val >> 6) % 2 );
	
	return 0;
}

static int op_ODD(struct vm* this) {
	F26D6 val;
	
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	
	/* round value according to round state and then truncate to the
	 * nearest integer before checking if it is odd or not. */
	val = round_by_state(this, STACK(0));
	STACK(0) = (val >> 6) % 2;
	
	return 0;
}

static int op_NEQ(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	STACK(1) = STACK(1) != STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_EQ(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	STACK(1) = STACK(1) == STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_GTEQ(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	STACK(1) = SSTACK(1) >= SSTACK(0);
	REG_SP--;
	
	return 0;
}

static int op_GT(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	STACK(1) = SSTACK(1) > SSTACK(0);
	REG_SP--;
	
	return 0;
}

static int op_LTEQ(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	STACK(1) = SSTACK(1) <= SSTACK(0);
	REG_SP--;
	
	return 0;
}

static int op_LT(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	STACK(1) = SSTACK(1) < SSTACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SWAP(struct vm* this) {
	if (REG_SP < 2) return FVMERR_STACK_UNDERFLOW;
	
	SWAP32( &STACK(1), &STACK(0) );
	
	return 0;
}

static int op_ROLL(struct vm* this) {
	if (REG_SP < 3) return FVMERR_STACK_UNDERFLOW;
	
	SWAP32( &STACK(0), &STACK(2) );
	SWAP32( &STACK(1), &STACK(2) );
	
	return 0;
}

static int op_MINDEX(struct vm* this) {
	UINT32 element;
	UINT32 i;
	
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) + 1 > REG_SP) return FVMERR_STACK_OVERFLOW;
	
	/* save element */
	element = STACK(STACK(0));
	
	/* remove from stack */
	for (i = STACK(0); i > 0; i--)
		STACK(i) = STACK(i - 1);
	
	/* push to top of stack */
	STACK(1) = element;
	REG_SP--;
	
	return 0;
}

static int op_CINDEX(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (STACK(0) + 1 > REG_SP) return FVMERR_STACK_OVERFLOW;
	
	STACK(0) = STACK(STACK(0));
	
	return 0;
}

static int op_POP(struct vm* this) {
	REG_SP -= (REG_SP > 0);
	
	return 0;
}

static int op_DUP(struct vm* this) {
	if (REG_SP < 1) return FVMERR_STACK_UNDERFLOW;
	if (REG_SP + 1 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP++;
	STACK(0) = STACK(1);
	
	return 0;
}

static int op_DEPTH(struct vm* this) {
	if (REG_SP + 1 > MAX_DEPTH) return FVMERR_STACK_OVERFLOW;
	
	REG_SP++;
	STACK(0) = REG_SP - 1;
	
	return 0;
}

static int op_CLEAR(struct vm* this) {
	REG_SP = 0;
	
	return 0;
}

char* command_names[INSTR_MAX] = {
	"SVTCA[0]",
	"SVTCA[1]",
	"SPVTCA[0]",
	"SPVTCA[1]",
	"SFVTCA[0]",
	"SFVTCA[1]",
	"SPVTL[0]",
	"SPVTL[1]",
	"SFVTL[0]",
	"SFVTL[1]",
	"SPVFS",
	"SFVFS",
	"GPV",
	"GFV",
	"SFVTPV",
	"ISECT",
	"SRP[0]",
	"SRP[1]",
	"SRP[2]",
	"SZP[0]",
	"SZP[1]",
	"SZP[2]",
	"SZPS",
	"SLOOP",
	"RTG",
	"RTHG",
	"SMD",
	"ELSE",
	"JMPR",
	"SCVTCI",
	"SSWCI",
	"SSW",
	"DUP",
	"POP",
	"CLEAR",
	"SWAP",
	"DEPTH",
	"CINDEX",
	"MINDEX",
	"ALIGNPTS",
	"UDEF0",
	"UTP",
	"LOOPCALL",
	"CALL",
	"FDEF",
	"ENDF",
	"MDAP[0]",
	"MDAP[1]",
	"IUP[0]",
	"IUP[1]",
	"SHP[0]",
	"SHP[1]",
	"SHC[0]",
	"SHC[1]",
	"SHZ[0]",
	"SHZ[1]",
	"SHPIX",
	"IP",
	"MSIRP[0]",
	"MSIRP[1]",
	"ALIGNRP",
	"RTDG",
	"MIAP[0]",
	"MIAP[1]",
	"NPUSHB",
	"NPUSHW",
	"WS",
	"RS",
	"WCVTP",
	"RCVT",
	"GC[0]",
	"GC[1]",
	"SCFS",
	"MD[0]",
	"MD[1]",
	"MPPEM",
	"MPS",
	"FLIPON",
	"FLIPOFF",
	"DEBUG",
	"LT",
	"LTEQ",
	"GT",
	"GTEQ",
	"EQ",
	"NEQ",
	"ODD",
	"EVEN",
	"IF",
	"EIF",
	"AND",
	"OR",
	"NOT",
	"DELTAP1",
	"SDB",
	"SDS",
	"ADD",
	"SUB",
	"DIV",
	"MUL",
	"ABS",
	"NEG",
	"FLOOR",
	"CEILING",
	"ROUND[0]",
	"ROUND[1]",
	"ROUND[2]",
	"ROUND[3]",
	"NROUND[0]",
	"NROUND[1]",
	"NROUND[2]",
	"NROUND[3]",
	"WCVTF",
	"DELTAP2",
	"DELTAP3",
	"DELTAC1",
	"DELTAC2",
	"DELTAC3",
	"SROUND",
	"S45ROUND",
	"JROT",
	"JROF",
	"ROFF",
	"UDEF1",
	"RUTG",
	"RDTG",
	"SANGW",
	"AA",
	"FLIPPT",
	"FLIPRGON",
	"FLIPRGOFF",
	"UDEF2",
	"UDEF3",
	"SCANCTRL",
	"SDPVTL[0]",
	"SDPVTL[1]",
	"GETINFO",
	"IDEF",
	"ROLL",
	"MAX",
	"MIN",
	"SCANTYPE",
	"INSTCTRL",
	"UDEF4",
	"UDEF5",
	"GETVARIATION",
	"UDEF6",
	"UDEF7",
	"UDEF8",
	"UDEF9",
	"UDEFA",
	"UDEFB",
	"UDEFC",
	"UDEFD",
	"UDEFE",
	"UDEFF",
	"UDEFG",
	"UDEFH",
	"UDEFI",
	"UDEFJ",
	"UDEFK",
	"UDEFL",
	"UDEFM",
	"UDEFN",
	"UDEFO",
	"UDEFP",
	"UDEFQ",
	"UDEFR",
	"UDEFS",
	"UDEFT",
	"UDEFU",
	"UDEFV",
	"UDEFW",
	"UDEFX",
	"UDEFY",
	"UDEFZ",
	"PUSHB[0]",
	"PUSHB[1]",
	"PUSHB[2]",
	"PUSHB[3]",
	"PUSHB[4]",
	"PUSHB[5]",
	"PUSHB[6]",
	"PUSHB[7]",
	"PUSHW[0]",
	"PUSHW[1]",
	"PUSHW[2]",
	"PUSHW[3]",
	"PUSHW[4]",
	"PUSHW[5]",
	"PUSHW[6]",
	"PUSHW[7]",
	"MDRP[0]",
	"MDRP[1]",
	"MDRP[2]",
	"MDRP[3]",
	"MDRP[4]",
	"MDRP[5]",
	"MDRP[6]",
	"MDRP[7]",
	"MDRP[8]",
	"MDRP[9]",
	"MDRP[10]",
	"MDRP[11]",
	"MDRP[12]",
	"MDRP[13]",
	"MDRP[14]",
	"MDRP[15]",
	"MDRP[16]",
	"MDRP[17]",
	"MDRP[18]",
	"MDRP[19]",
	"MDRP[20]",
	"MDRP[21]",
	"MDRP[22]",
	"MDRP[23]",
	"MDRP[24]",
	"MDRP[25]",
	"MDRP[26]",
	"MDRP[27]",
	"MDRP[28]",
	"MDRP[29]",
	"MDRP[30]",
	"MDRP[31]",
	"MIRP[0]",
	"MIRP[1]",
	"MIRP[2]",
	"MIRP[3]",
	"MIRP[4]",
	"MIRP[5]",
	"MIRP[6]",
	"MIRP[7]",
	"MIRP[8]",
	"MIRP[9]",
	"MIRP[10]",
	"MIRP[11]",
	"MIRP[12]",
	"MIRP[13]",
	"MIRP[14]",
	"MIRP[15]",
	"MIRP[16]",
	"MIRP[17]",
	"MIRP[18]",
	"MIRP[19]",
	"MIRP[20]",
	"MIRP[21]",
	"MIRP[22]",
	"MIRP[23]",
	"MIRP[24]",
	"MIRP[25]",
	"MIRP[26]",
	"MIRP[27]",
	"MIRP[28]",
	"MIRP[29]",
	"MIRP[30]",
	"MIRP[31]",
};

int (*instructions[INSTR_MAX])(struct vm*) = {
	op_SVTCA_0,
	op_SVTCA_1,
	op_SPVTCA_0,
	op_SPVTCA_1,
	op_SFVTCA_0,
	op_SFVTCA_1,
	op_SPVTL_0,
	op_SPVTL_1,
	op_SFVTL_0,
	op_SFVTL_1,
	op_SPVFS,
	op_SFVFS,
	op_GPV,
	op_GFV,
	op_SFVTPV,
	op_ISECT,
	op_SRP_0,
	op_SRP_1,
	op_SRP_2,
	op_SZP_0,
	op_SZP_1,
	op_SZP_2,
	op_SZPS,
	op_SLOOP,
	op_RTG,
	op_RTHG,
	op_SMD,
	op_ELSE,
	op_JMPR,
	op_SCVTCI,
	op_SSWCI,
	op_SSW,
	op_DUP,
	op_POP,
	op_CLEAR,
	op_SWAP,
	op_DEPTH,
	op_CINDEX,
	op_MINDEX,
	op_ALIGNPTS,
	NULL, /* UDEF0 */
	op_UTP,
	op_LOOPCALL,
	op_CALL,
	op_FDEF,
	op_ENDF,
	op_MDAP_0,
	op_MDAP_1,
	op_IUP_0,
	op_IUP_1,
	op_SHP_0,
	op_SHP_1,
	op_SHC_0,
	op_SHC_1,
	op_SHZ_0,
	op_SHZ_1,
	op_SHPIX,
	op_IP,
	op_MSIRP_0,
	op_MSIRP_1,
	op_ALIGNRP,
	op_RTDG,
	op_MIAP_0,
	op_MIAP_1,
	op_NPUSHB,
	op_NPUSHW,
	op_WS,
	op_RS,
	op_WCVTP,
	op_RCVT,
	op_GC_0,
	op_GC_1,
	op_SCFS,
	op_MD_0,
	op_MD_1,
	op_MPPEM,
	op_MPS,
	op_FLIPON,
	op_FLIPOFF,
	op_DEBUG,
	op_LT,
	op_LTEQ,
	op_GT,
	op_GTEQ,
	op_EQ,
	op_NEQ,
	op_ODD,
	op_EVEN,
	op_IF,
	op_EIF,
	op_AND,
	op_OR,
	op_NOT,
	op_DELTAP1,
	op_SDB,
	op_SDS,
	op_ADD,
	op_SUB,
	op_DIV,
	op_MUL,
	op_ABS,
	op_NEG,
	op_FLOOR,
	op_CEILING,
	op_ROUND_0,
	op_ROUND_1,
	op_ROUND_2,
	op_ROUND_3,
	op_NROUND_0,
	op_NROUND_1,
	op_NROUND_2,
	op_NROUND_3,
	op_WCVTF,
	op_DELTAP2,
	op_DELTAP3,
	op_DELTAC1,
	op_DELTAC2,
	op_DELTAC3,
	op_SROUND,
	op_S45ROUND,
	op_JROT,
	op_JROF,
	op_ROFF,
	NULL, /* UDEF1 */
	op_RUTG,
	op_RDTG,
	op_SANGW,
	op_AA,
	op_FLIPPT,
	op_FLIPRGON,
	op_FLIPRGOFF,
	NULL, /* UDEF2 */
	NULL, /* UDEF3 */
	op_SCANCTRL,
	op_SDPVTL_0,
	op_SDPVTL_1,
	op_GETINFO,
	op_IDEF,
	op_ROLL,
	op_MAX,
	op_MIN,
	op_SCANTYPE,
	op_INSTCTRL,
	NULL, /* UDEF4 */
	NULL, /* UDEF5 */
	NULL, /* GETVARIATION */
	NULL, /* UDEF6 */
	NULL, /* UDEF7 */
	NULL, /* UDEF8 */
	NULL, /* UDEF9 */
	NULL, /* UDEFA */
	NULL, /* UDEFB */
	NULL, /* UDEFC */
	NULL, /* UDEFD */
	NULL, /* UDEFE */
	NULL, /* UDEFF */
	NULL, /* UDEFG */
	NULL, /* UDEFH */
	NULL, /* UDEFI */
	NULL, /* UDEFJ */
	NULL, /* UDEFK */
	NULL, /* UDEFL */
	NULL, /* UDEFM */
	NULL, /* UDEFN */
	NULL, /* UDEFO */
	NULL, /* UDEFP */
	NULL, /* UDEFQ */
	NULL, /* UDEFR */
	NULL, /* UDEFS */
	NULL, /* UDEFT */
	NULL, /* UDEFU */
	NULL, /* UDEFV */
	NULL, /* UDEFW */
	NULL, /* UDEFX */
	NULL, /* UDEFY */
	NULL, /* UDEFZ */
	op_PUSHB_0,
	op_PUSHB_1,
	op_PUSHB_2,
	op_PUSHB_3,
	op_PUSHB_4,
	op_PUSHB_5,
	op_PUSHB_6,
	op_PUSHB_7,
	op_PUSHW_0,
	op_PUSHW_1,
	op_PUSHW_2,
	op_PUSHW_3,
	op_PUSHW_4,
	op_PUSHW_5,
	op_PUSHW_6,
	op_PUSHW_7,
	NULL, /* MDRP + 0  */
	NULL, /* MDRP + 1  */
	NULL, /* MDRP + 2  */
	NULL, /* MDRP + 3  */
	NULL, /* MDRP + 4  */
	NULL, /* MDRP + 5  */
	NULL, /* MDRP + 6  */
	NULL, /* MDRP + 7  */
	NULL, /* MDRP + 8  */
	NULL, /* MDRP + 9  */
	NULL, /* MDRP + 10 */
	NULL, /* MDRP + 11 */
	NULL, /* MDRP + 12 */
	NULL, /* MDRP + 13 */
	NULL, /* MDRP + 14 */
	NULL, /* MDRP + 15 */
	NULL, /* MDRP + 16 */
	NULL, /* MDRP + 17 */
	NULL, /* MDRP + 18 */
	NULL, /* MDRP + 19 */
	NULL, /* MDRP + 20 */
	NULL, /* MDRP + 21 */
	NULL, /* MDRP + 22 */
	NULL, /* MDRP + 23 */
	NULL, /* MDRP + 24 */
	NULL, /* MDRP + 25 */
	NULL, /* MDRP + 26 */
	NULL, /* MDRP + 27 */
	NULL, /* MDRP + 28 */
	NULL, /* MDRP + 29 */
	NULL, /* MDRP + 30 */
	NULL, /* MDRP + 31 */
	NULL, /* MIRP + 0  */
	NULL, /* MIRP + 1  */
	NULL, /* MIRP + 2  */
	NULL, /* MIRP + 3  */
	NULL, /* MIRP + 4  */
	NULL, /* MIRP + 5  */
	NULL, /* MIRP + 6  */
	NULL, /* MIRP + 7  */
	NULL, /* MIRP + 8  */
	NULL, /* MIRP + 9  */
	NULL, /* MIRP + 10 */
	NULL, /* MIRP + 11 */
	NULL, /* MIRP + 12 */
	NULL, /* MIRP + 13 */
	NULL, /* MIRP + 14 */
	NULL, /* MIRP + 15 */
	NULL, /* MIRP + 16 */
	NULL, /* MIRP + 17 */
	NULL, /* MIRP + 18 */
	NULL, /* MIRP + 19 */
	NULL, /* MIRP + 20 */
	NULL, /* MIRP + 21 */
	NULL, /* MIRP + 22 */
	NULL, /* MIRP + 23 */
	NULL, /* MIRP + 24 */
	NULL, /* MIRP + 25 */
	NULL, /* MIRP + 26 */
	NULL, /* MIRP + 27 */
	NULL, /* MIRP + 28 */
	NULL, /* MIRP + 29 */
	NULL, /* MIRP + 30 */
	NULL, /* MIRP + 31 */
};
