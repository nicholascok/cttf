
/* 
 * furound(this, _x) rounds the 26.6 floating point number _x according to
 * the current round state
 */
static F26D6 furound(struct font* this, F26D6 _x) {
	char s_old = SIGN(_x);
	char s_new = 0;
	
	/* prevent modulo zero */
	if (ROUND_PERIOD == 0)
		return _x;
	
	if (GRAPHICS_STATE.round_state == 5)
		return _x;
	
	_x -= ROUND_PHASE;
	_x += ROUND_THRESHOLD;
	
	_x -= (_x % ROUND_PERIOD + ROUND_PERIOD) % ROUND_PERIOD;
	
	_x += ROUND_PHASE;
	
	s_new = SIGN(_x);
	
	if (s_new != s_old) {
		if (s_new < 0)
			_x =  ROUND_PHASE;
		if (s_new > 0)
			_x = -ROUND_PHASE;
	}
	
	return _x;
}

/* 
 * dot2f(_v1, _v2) computes the dot product between the two vectors _v1
 * and _v2.
 */
static float dot2f(struct vec2f _v1, struct vec2f _v2) {
	return _v1.x * _v2.x + _v1.y * _v2.y;
}

/* 
 * fisqrt(_x) computes the inverse of the square root of the floating point
 * value _x. (yes, based on the quake algorithm - to avoid math.h)
 */
static float fisqrt(float _x) {
	uint32_t u = 0x5F400000UL - (*((uint32_t*) &_x) >> 1);
	float f = *((float*) &u);
	
	f *= (1.5 - (_x * 0.5 * f * f));
	f *= (1.5 - (_x * 0.5 * f * f));
	f *= (1.5 - (_x * 0.5 * f * f));
	f *= (1.5 - (_x * 0.5 * f * f));
	
	return f;
}

/* 
 * update_round_state(this) updates the round state of the truetype virtual
 * machine. it is called after the packed round state setting has chagned.
 */
static int update_round_state(struct font* this) {
	switch (RSTATE_PERIOD) {
		case 0: ROUND_PERIOD = F26D6MUL(ROUND_GRID_PERIOD, 32);  break;
		case 1: ROUND_PERIOD = ROUND_GRID_PERIOD;                break;
		case 2: ROUND_PERIOD = F26D6MUL(ROUND_GRID_PERIOD, 128); break;
		default: ROUND_PERIOD = 64; /* technically reserved */
	}
	
	switch (RSTATE_PHASE) {
		case 0: ROUND_PHASE = 0;                               break;
		case 1: ROUND_PHASE = F26D6MUL(ROUND_PERIOD, 16);            break;
		case 2: ROUND_PHASE = F26D6MUL(ROUND_PERIOD, 32);            break;
		case 3: ROUND_PHASE = F26D6MUL(ROUND_GRID_PERIOD, 48); break;
	}
	
	if (RSTATE_THRESHOLD == 0)
		ROUND_THRESHOLD = ROUND_PERIOD - 1;
	else
		ROUND_THRESHOLD = F26D6MUL(ROUND_PERIOD, 8 * RSTATE_THRESHOLD - 32);
	
	return 0;
}

/* 
 * op_UDEF(this, opcode) is called for instructions that normally have no
 * defined use, and that the user has assigned a proceedure to.
 */
static int op_UDEF(struct font* this, BYTE opcode) {
	UINT16 idx;
	
	/* search for opcode */
	for (idx = 0; INSTRUCTION(idx).op != opcode; idx++)
		if (idx >= MAX_INSTRUCTION_DEFS) return -1;
	
	/* call proceedure */
	
	REG_RP = REG_EP;
	
	this->vm.temp_stream = this->vm.stream;
	this->vm.stream = INSTRUCTION(idx).stream;
	
	REG_EP = 1;
	
	return 0;
}

/**************************************************************************
 * all the instructions, see the specification for information on any     *
 * specific one.                                                          *
 *************************************************************************/

static int op_SCANTYPE(struct font* this) {
	
	return 0;
}

static int op_SCANCTRL(struct font* this) {
	
	return 0;
}

static int op_IP(struct font* this) {
	struct vec2f F; /* freedom vec */
	struct vec2f P; /* projection vec */
	
	struct vec2f ;
	
	if (REG_SP < 1) return -1;
	
	F.x = (float) FRD_VEC.x / 16384;
	F.y = (float) FRD_VEC.y / 16384;
	
	P.x = (float) PRJ_VEC.x / 16384;
	P.y = (float) PRJ_VEC.y / 16384;
	
	/*
	ZONEP2(STACK(0)).pos.x
	ZONEP2(STACK(0)).pos.y
	ZONEP0(REG_RP1).pos.x
	ZONEP0(REG_RP1).pos.y
	ZONEP1(REG_RP2).pos.x
	ZONEP1(REG_RP2).pos.y
	*/
	
	REG_SP -= 2;
	
	return 0;
}

static int op_INSTCTRL(struct font* this) {
	
	return 0;
}

static int op_GETVARIATION(struct font* this) {
	
	return 0;
}

static int op_GETINFO(struct font* this) {
	
	return 0;
}

static int op_DELTAP3(struct font* this) {
	
	return 0;
}

static int op_DELTAP2(struct font* this) {
	
	return 0;
}

static int op_DELTAP1(struct font* this) {
	
	return 0;
}

static int op_DELTAC3(struct font* this) {
	
	return 0;
}

static int op_DELTAC2(struct font* this) {
	
	return 0;
}

static int op_DELTAC1(struct font* this) {
	
	return 0;
}

static int op_MDAP_0(struct font* this) {
	
	return 0;
}

static int op_MDAP_1(struct font* this) {
	
	return 0;
}

static int op_IUP_0(struct font* this) {
	
	return 0;
}

static int op_IUP_1(struct font* this) {
	
	return 0;
}

static int op_SHP_0(struct font* this) {
	
	return 0;
}

static int op_SHP_1(struct font* this) {
	
	return 0;
}

static int op_SHC_0(struct font* this) {
	
	return 0;
}

static int op_SHC_1(struct font* this) {
	
	return 0;
}

static int op_SHZ_0(struct font* this) {
	
	return 0;
}

static int op_SHZ_1(struct font* this) {
	
	return 0;
}

static int op_MSIRP_0(struct font* this) {
	
	return 0;
}

static int op_MSIRP_1(struct font* this) {
	
	return 0;
}

static int op_MIAP_0(struct font* this) {
	
	return 0;
}

/* dual */
static int op_SDPVTL_0(struct font* this) {
	struct fuvec L;
	float Lx, Ly, imag;
	
	if (REG_SP < 2) return -1;
	
	L.x = ZONEP1(STACK(1)).pos.x - ZONEP2(STACK(0)).pos.x;
	L.y = ZONEP1(STACK(1)).pos.y - ZONEP2(STACK(0)).pos.y;
	
	if (L.x == 0 && L.y == 0)
		return -1;
	
	Lx = (float) (L.x);
	Ly = (float) (L.y);
	
	imag = 16384.0 * fisqrt(Lx * Lx + Ly * Ly);
	
	DPRJ_VEC.y = (uint16_t) (Ly * imag);
	DPRJ_VEC.x = (uint16_t) (Lx * imag);
	
	REG_SP -= 2;
	
	return 0;
}

/* dual */
static int op_SDPVTL_1(struct font* this) {
	struct fuvec L;
	float Lx, Ly, imag;
	
	if (REG_SP < 2) return -1;
	
	L.x = ZONEP1(STACK(1)).pos.x - ZONEP2(STACK(0)).pos.x;
	L.y = ZONEP1(STACK(1)).pos.y - ZONEP2(STACK(0)).pos.y;
	
	if (L.x == 0 && L.y == 0)
		return -1;
	
	Lx = (float) (-L.y);
	Ly = (float) (L.x);
	
	imag = 16384.0 * fisqrt(Lx * Lx + Ly * Ly);
	
	DPRJ_VEC.y = (uint16_t) (Ly * imag);
	DPRJ_VEC.x = (uint16_t) (Lx * imag);
	
	REG_SP -= 2;
	
	return 0;
}

/* dual */
static int op_GC_0(struct font* this) {
	struct vec2f P;  	/* projection vec */
	struct vec2f pnt;	/* point */
	
	if (REG_SP < 1) return -1;
	
	P.x = (float) PRJ_VEC.x / 16384.0;
	P.y = (float) PRJ_VEC.y / 16384.0;
	
	pnt.y = (float) ZONEP2(STACK(0)).pos.y;
	pnt.x = (float) ZONEP2(STACK(0)).pos.x;
	
	
	SSTACK(0) = (int32_t) dot2f(P, pnt);
	
	return 0;
}

/* dual */
static int op_GC_1(struct font* this) {
	
	return 0;
}

static int op_MIAP_1(struct font* this) {
	
	return 0;
}

static int op_FDEF(struct font* this) {
	BYTE* ptr;	/* pointer to start of proceedure */
	UINT32 len;  	/* length of proceedure */
	
	/* check we have a valid funcion id on the stack... */
	if (REG_SP < 1 || STACK(0) > MAX_FUNCTION_DEFS) {
		/* if not, skip the proceedure body and return -1 to indicate an
		 * error has occurred. */
		for (; STREAM(0) != ENDF; REG_EP++);
		return -1;
	}
	
	/* get the length of our proceedure... */
	for (ptr = &STREAM(0); *ptr != ENDF; ptr++);
	len = ptr - &STREAM(0) + 1;
	
	/* if we are redefining a function, free the old one. */
	if (FUNCTION(STACK(0)).stream)
		free(FUNCTION(STACK(0)).stream);
	
	/* add function */
	FUNCTION(STACK(0)).stream = smalloc(len);
	
	memcpy(FUNCTION(STACK(0)).stream, &STREAM(0), len);
	
	/* skip the proceedure body */
	REG_EP += len - 1;
	
	REG_SP--;
	
	return 0;
}

static int op_IDEF(struct font* this) {
	BYTE* ptr;	/* pointer to start of proceedure */
	UINT16 idx;  	/* index where instruction is to be stored */
	UINT32 len;  	/* length of proceedure */
	
	/* check we have a valid opcode on the stack... */
	if (REG_SP < 1 || STACK(0) + 1 > INSTR_MAX) {
		/* if not, skip the proceedure body and return -1 to indicate an
		 * error has occurred. */
		for (; STREAM(0) != ENDF; REG_EP++);
		return -1;
	}
	
	/* get the length of our proceedure... */
	for (ptr = &STREAM(0); *ptr != ENDF; ptr++);
	len = ptr - &STREAM(0) + 1;
	
	/* check that we are not overriding a built in instruction
	 * (instructions that are undefined start with a '~'). */
	if (command_names[STACK(0)][0] == '~') {
		
		/* find an empty space to store the instruction and its
		 * proceedure... */
		for (idx = 0; INSTRUCTION(idx).stream; idx++) {
			
			/* check index in in rage */
			if (idx >= MAX_INSTRUCTION_DEFS) {
				/* if not, skip the proceedure body and return -1 to
				 * indicate an error has occurred. */
				REG_EP += len - 1;
				return -1;
			}
			
			/* if we come accross the opcode we are trying to insert, break
			 * the loop so the isntruction can be redefined. */
			if (INSTRUCTION(idx).op == STACK(0))
				break;
		}
		
		/* set opcode */
		INSTRUCTION(idx).op = STACK(0);
		
		/* if we are redefining an instruction, free the old one. */
		if (INSTRUCTION(idx).stream)
			free(INSTRUCTION(idx).stream);
		
		/* add instruction */
		INSTRUCTION(idx).stream = smalloc(len);
		if (INSTRUCTION(idx).stream == NULL) XERR(XEMSG_NOMEM, ENOMEM);
		
		memcpy(INSTRUCTION(idx).stream, &STREAM(0), len);
	}
	
	/* skip the proceedure body */
	REG_EP += len - 1;
	
	REG_SP--;
	
	return 0;
}

static int op_ENDF(struct font* this) {
	/* set the execution pointer to the value sotred in the return register
	 * and switch our stream to its original value (the main execution
	 * stream). */
	REG_EP = REG_RP;
	this->vm.stream = this->vm.temp_stream;
	
	return 0;
}

static int op_CALL(struct font* this) {
	if (REG_SP < 1) return -1;
	
	/* store our current address in the return register... */
	REG_RP = REG_EP;
	
	/* save our execution stream... */
	this->vm.temp_stream = this->vm.stream;
	
	/* if the function we are trying to call exists, switch our stream to
	 * the one which contains the function's body. */
	if (FUNCTION(STACK(0)).stream) {
		this->vm.stream = FUNCTION(STACK(0)).stream;
		
		/* in this stream, the execution pointer is at position 1. */
		REG_EP = 1;
	}
	
	/* otherwise ,if the function we are trying to call does not exist then
	 * return -1 to indicate that an error has occurred. */
	else return -1;
	
	REG_SP--;
	
	return 0;
}

static int op_LOOPCALL(struct font* this) {
	static BYTE is_looping = 0;
	static UINT32 funcid;	/* function id */
	static UINT32 calls; 	/* number of calls remaining */
	int err;          	/* error code from function call */
	
	if (is_looping == 0) {
		/* this runs on the initial call to the function... */
		
		if (REG_SP < 2) return -1;
		
		funcid = STACK(0);
		calls = STACK(1);
		
		REG_SP -= 2;
		
		is_looping = 1;
	}
	
	if (calls > 0) {
		/* count down remaining calls */
		calls--;
	} else {
		/* if there are no more calls to process... */
		is_looping = 0;
		return 0;
	}
	
	/* decrement EP register so we do not proceed to the next instruction
	 * until all calls have been exhausted. */
	REG_EP--;
	
	/* perform a call... */
	REG_SP++;
	STACK(0) = funcid;
	
	err = op_CALL(this);
	
	/* if the call succeeds, continue... */
	if (!err) return 0;
	
	/* if the call failed, stop looping and report the error. */
	is_looping = 0;
	REG_EP++;
	
	return err;
}

static int op_IF(struct font* this) {
	if (REG_SP < 1) return -1;
	
	if (STACK(0))
		BRANCH_DEPTH++;
	
	else for (;; REG_EP++) {
		if (STREAM(0) == ELSE) {
			BRANCH_DEPTH++;
			break;
		}
		
		if (STREAM(0) == EIF)
			break;
	}
	
	REG_SP--;
	
	return 0;
}

static int op_ELSE(struct font* this) {
	if (BRANCH_DEPTH > 0)
		for (; STREAM(0) != EIF; REG_EP++);
	
	return 0;
}

static int op_EIF(struct font* this) {
	if (BRANCH_DEPTH > 0)
		BRANCH_DEPTH--;
	
	return 0;
}

static int op_MPS(struct font* this) {
	REG_SP++;
	STACK(0) = this->point_size;
	
	return 0;
}

static int op_MPPEM(struct font* this) {
	float C = PRJ_VEC.x * PRJ_VEC.x + PRJ_VEC.y * PRJ_VEC.y;
	
	REG_SP++;
	STACK(0) = this->point_size * fisqrt(C);
	
	return 0;
}

static int op_WCVTP(struct font* this) {
	if (REG_SP < 2) return -1;
	
	CONTROL_VALUE(STACK(1)) = STACK(0);
	REG_SP -= 2;
	
	return 0;
}

static int op_WCVTF(struct font* this) {
	if (REG_SP < 2) return -1;
	
	CONTROL_VALUE(STACK(1)) = FUTOPIXELS(STACK(0));
	REG_SP -= 2;
	
	return 0;
}

static int op_SVTCA_0(struct font* this) {
	PRJ_VEC.y = 16384;
	PRJ_VEC.x = 0;
	FRD_VEC.y = 16384;
	FRD_VEC.x = 0;
	
	return 0;
}

static int op_SVTCA_1(struct font* this) {
	PRJ_VEC.x = 16384;
	PRJ_VEC.y = 0;
	FRD_VEC.x = 16384;
	FRD_VEC.y = 0;
	
	return 0;
}

static int op_SPVTCA_0(struct font* this) {
	PRJ_VEC.y = 16384;
	PRJ_VEC.x = 0;
	
	return 0;
}

static int op_SPVTCA_1(struct font* this) {
	PRJ_VEC.x = 16384;
	PRJ_VEC.y = 0;
	
	return 0;
}

static int op_SFVTCA_0(struct font* this) {
	FRD_VEC.y = 16384;
	FRD_VEC.x = 0;
	
	return 0;
}

static int op_SFVTCA_1(struct font* this) {
	FRD_VEC.x = 16384;
	FRD_VEC.y = 0;
	
	return 0;
}

/* todo: use fixed-point? */
static int op_SPVTL_0(struct font* this) {
	struct fuvec L;
	float Lx, Ly, imag;
	
	if (REG_SP < 2) return -1;
	
	L.x = ZONEP1(STACK(1)).pos.x - ZONEP2(STACK(0)).pos.x;
	L.y = ZONEP1(STACK(1)).pos.y - ZONEP2(STACK(0)).pos.y;
	
	if (L.x == 0 && L.y == 0)
		return -1;
	
	Lx = (float) (L.x);
	Ly = (float) (L.y);
	
	imag = 16384.0 * fisqrt(Lx * Lx + Ly * Ly);
	
	PRJ_VEC.y = (uint16_t) (Ly * imag);
	PRJ_VEC.x = (uint16_t) (Lx * imag);
	
	REG_SP -= 2;
	
	return 0;
}

/* todo: use fixed-point? */
static int op_SPVTL_1(struct font* this) {
	struct fuvec L;
	float Lx, Ly, imag;
	
	if (REG_SP < 2) return -1;
	
	L.x = ZONEP1(STACK(1)).pos.x - ZONEP2(STACK(0)).pos.x;
	L.y = ZONEP1(STACK(1)).pos.y - ZONEP2(STACK(0)).pos.y;
	
	if (L.x == 0 && L.y == 0)
		return -1;
	
	Lx = (float) (-L.y);
	Ly = (float) (L.x);
	
	imag = 16384.0 * fisqrt(Lx * Lx + Ly * Ly);
	
	PRJ_VEC.y = (uint16_t) (Ly * imag);
	PRJ_VEC.x = (uint16_t) (Lx * imag);
	
	REG_SP -= 2;
	
	return 0;
}

/* todo: use fixed-point? */
static int op_SFVTL_0(struct font* this) {
	struct fuvec L;
	float Lx, Ly, imag;
	
	if (REG_SP < 2) return -1;
	
	L.x = ZONEP1(STACK(1)).pos.x - ZONEP2(STACK(0)).pos.x;
	L.y = ZONEP1(STACK(1)).pos.y - ZONEP2(STACK(0)).pos.y;
	
	if (L.x == 0 && L.y == 0)
		return -1;
	
	Lx = (float) (L.x);
	Ly = (float) (L.y);
	
	imag = 16384.0 * fisqrt(Lx * Lx + Ly * Ly);
	
	FRD_VEC.y = (uint16_t) (Ly * imag);
	FRD_VEC.x = (uint16_t) (Lx * imag);
	
	REG_SP -= 2;
	
	return 0;
}

/* todo: use fixed-point? */
static int op_SFVTL_1(struct font* this) {
	struct fuvec L;
	float Lx, Ly, imag;
	
	if (REG_SP < 2) return -1;
	
	L.x = ZONEP1(STACK(1)).pos.x - ZONEP2(STACK(0)).pos.x;
	L.y = ZONEP1(STACK(1)).pos.y - ZONEP2(STACK(0)).pos.y;
	
	if (L.x == 0 && L.y == 0)
		return -1;
	
	Lx = (float) (-L.y);
	Ly = (float) (L.x);
	
	imag = 16384.0 * fisqrt(Lx * Lx + Ly * Ly);
	
	FRD_VEC.y = (uint16_t) (Ly * imag);
	FRD_VEC.x = (uint16_t) (Lx * imag);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_S45ROUND(struct font* this) {
	if (REG_SP < 1) return -1;
	
	ROUND_STATE = STACK(0);
	GRAPHICS_STATE.round_state = ~0;
	update_round_state(this);
	ROUND_GRID_PERIOD = 45;
	
	return 0;
}

static int op_SROUND(struct font* this) {
	if (REG_SP < 1) return -1;
	
	ROUND_STATE = STACK(0);
	GRAPHICS_STATE.round_state = ~0;
	update_round_state(this);
	ROUND_GRID_PERIOD = 64;
	
	return 0;
}

static int op_SHPIX(struct font* this) {
	UINT32 C;	/* amount to shift */
	UINT32 i;
	
	if (REG_SP < REG_LP + 1)
		return -1;
	
	C = PIXELSTOFU(STACK(0));
	REG_SP--;
	
	for (i = 0; i < REG_LP; i++) {
		ZONEP2(STACK(i)).pos.x += (F26D6MUL(FRD_VEC.x >> 8, C << 6) >> 6);
		ZONEP2(STACK(i)).pos.y += (F26D6MUL(FRD_VEC.y >> 8, C << 6) >> 6);
	}
	
	REG_SP -= REG_LP;
	
	return 0;
}

static int op_PUSHB_0(struct font* this) {
	REG_SP++;
	
	USTACK(0) = STREAM(1);
	
	REG_EP++;
	
	return 0;
}

static int op_PUSHB_1(struct font* this) {
	REG_SP += 2;
	
	USTACK(0) = STREAM(2);
	USTACK(1) = STREAM(1);
	
	REG_EP += 2;
	
	return 0;
}

static int op_PUSHB_2(struct font* this) {
	REG_SP += 3;
	
	USTACK(0) = STREAM(3);
	USTACK(1) = STREAM(2);
	USTACK(2) = STREAM(1);
	
	REG_EP += 3;
	
	return 0;
}

static int op_PUSHB_3(struct font* this) {
	REG_SP += 4;
	
	USTACK(0) = STREAM(4);
	USTACK(1) = STREAM(3);
	USTACK(2) = STREAM(2);
	USTACK(3) = STREAM(1);
	
	REG_EP += 4;
	
	return 0;
}

static int op_PUSHB_4(struct font* this) {
	REG_SP += 5;
	
	USTACK(0) = STREAM(5);
	USTACK(1) = STREAM(4);
	USTACK(2) = STREAM(3);
	USTACK(3) = STREAM(2);
	USTACK(4) = STREAM(1);
	
	REG_EP += 5;
	
	return 0;
}

static int op_PUSHB_5(struct font* this) {
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

static int op_PUSHB_6(struct font* this) {
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

static int op_PUSHB_7(struct font* this) {
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

static int op_PUSHW_0(struct font* this) {
	REG_SP++;
	
	USTACK(0) = STREAM(1) << 8;
	USTACK(0) |= STREAM(2);
	
	REG_EP += 2;
	
	return 0;
}

static int op_PUSHW_1(struct font* this) {
	REG_SP += 2;
	
	USTACK(0) = STREAM(3) << 8;
	USTACK(0) |= STREAM(4);
	
	USTACK(1) = STREAM(1) << 8;
	USTACK(1) |= STREAM(2);
	
	REG_EP += 4;
	
	return 0;
}

static int op_PUSHW_2(struct font* this) {
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

static int op_PUSHW_3(struct font* this) {
	REG_SP += 4;
	
	USTACK(0) = STREAM(7) << 8;
	USTACK(0) |= STREAM(8);
	
	USTACK(1) = STREAM(5) << 8;
	USTACK(1) |= STREAM(6);
	
	USTACK(2) = STREAM(3) << 8;
	USTACK(2) |= STREAM(4);
	
	USTACK(3) = STREAM(1) << 8;
	USTACK(4) |= STREAM(2);
	
	REG_EP += 8;
	
	return 0;
}

static int op_PUSHW_4(struct font* this) {
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

static int op_PUSHW_5(struct font* this) {
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

static int op_PUSHW_6(struct font* this) {
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

static int op_PUSHW_7(struct font* this) {
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

static int op_SRP_0(struct font* this) {
	if (REG_SP < 1) return -1;
	
	REG_RP0 = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SRP_1(struct font* this) {
	if (REG_SP < 1) return -1;
	
	REG_RP1 = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SRP_2(struct font* this) {
	if (REG_SP < 1) return -1;
	
	REG_RP2 = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SZP_0(struct font* this) {
	if (REG_SP < 1) return -1;
	
	REG_ZP0 = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SZP_1(struct font* this) {
	if (REG_SP < 1) return -1;
	
	REG_ZP1 = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SZP_2(struct font* this) {
	if (REG_SP < 1) return -1;
	
	REG_ZP2 = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_ROUND_0(struct font* this) {
	if (REG_SP < 1) return -1;
	
	/* grey distances recieve no engine compensation... */
	SSTACK(0) = furound(this, SSTACK(0));
	
	return 0;
}

static int op_ROUND_1(struct font* this) {
	if (REG_SP < 1) return -1;
	
	/* black distances casue disatnces to shrink... */
	SSTACK(0) = furound(this, SSTACK(0) - SIGN(SSTACK(0)) * ENGINE_COMPENSATION);
	
	return 0;
}

static int op_ROUND_2(struct font* this) {
	if (REG_SP < 1) return -1;
	
	/* white distances casue disatnces to grow... */
	SSTACK(0) = furound(this, SSTACK(0) + SIGN(SSTACK(0)) * ENGINE_COMPENSATION);
	
	return 0;
}

static int op_ROUND_3(struct font* this) {
	if (REG_SP < 1) return -1;
	
	/* ???????? ... ?????? .... reserved maybe??? */
	SSTACK(0) = furound(this, SSTACK(0));
	
	return 0;
}

static int op_NROUND_0(struct font* this) {
	/* grey distances recieve no engine compensation, so we just do
	 * literally nothing. */
	
	return 0;
}

static int op_NROUND_1(struct font* this) {
	if (REG_SP < 1) return -1;
	
	/* black distances casue disatnces to shrink... */
	SSTACK(0) = SSTACK(0) - SIGN(SSTACK(0)) * ENGINE_COMPENSATION;
	
	return 0;
}

static int op_NROUND_2(struct font* this) {
	if (REG_SP < 1) return -1;
	
	/* white distances casue disatnces to grow... */
	SSTACK(0) = SSTACK(0) + SIGN(SSTACK(0)) * ENGINE_COMPENSATION;
	
	return 0;
}

static int op_NROUND_3(struct font* this) {
	/* there are three distance types (white, grey, black)...
	 * THEN WHY ARE THERE FOUR VARIENTS ???? */
	
	return 0;
}

static int op_UTP(struct font* this) {
	if (REG_SP < 1) return -1;
	
	if (FRD_VEC.y != 0)
		ZONEP0(STACK(0)).status &= ~POINT_MASK_YTOUCHED;
	
	if (FRD_VEC.x != 0)
		ZONEP0(STACK(0)).status &= ~POINT_MASK_XTOUCHED;
	
	REG_SP--;
	
	return 0;
}

static int op_SSWCI(struct font* this) {
	if (REG_SP < 1) return -1;
	
	GRAPHICS_STATE.single_width_cut_in = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SSW(struct font* this) {
	if (REG_SP < 1) return -1;
	
	GRAPHICS_STATE.single_width_value = STACK(0);
	REG_SP--;
	
	return 0;
}

/* todo: use fixed-point? */
static int op_SPVFS(struct font* this) {
	float Px, Py, imag;
	
	if (REG_SP < 2) return -1;
	
	if (STACK(0) == 0 && STACK(1) == 0)
		return -1;
	
	Px = (float) STACK(1);
	Py = (float) STACK(0);
	imag = 16384.0 * fisqrt(Px * Px + Py * Py);
	
	PRJ_VEC.y = (uint16_t) (Py * imag);
	PRJ_VEC.x = (uint16_t) (Px * imag);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_SMD(struct font* this) {
	if (REG_SP < 1) return -1;
	
	GRAPHICS_STATE.minimum_distance = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SLOOP(struct font* this) {
	if (REG_SP < 1) return -1;
	
	REG_LP = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SFVTPV(struct font* this) {
	FRD_VEC.y = PRJ_VEC.y;
	FRD_VEC.x = PRJ_VEC.x;
	
	return 0;
}

/* todo: use fixed-point? */
static int op_SFVFS(struct font* this) {
	float Fx, Fy, imag;
	
	if (REG_SP < 2) return -1;
	
	if (STACK(0) == 0 && STACK(1) == 0)
		return -1;
	
	Fx = (float) STACK(1);
	Fy = (float) STACK(0);
	imag = 16384.0 * fisqrt(Fx * Fx + Fy * Fy);
	
	FRD_VEC.y = (int16_t) (Fy * imag);
	FRD_VEC.x = (int16_t) (Fx * imag);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_SDS(struct font* this) {
	if (REG_SP < 1) return -1;
	
	GRAPHICS_STATE.delta_shift = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SDB(struct font* this) {
	if (REG_SP < 1) return -1;
	
	GRAPHICS_STATE.delta_base = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SCVTCI(struct font* this) {
	if (REG_SP < 1) return -1;
	
	GRAPHICS_STATE.cut_in = STACK(0);
	REG_SP--;
	
	return 0;
}

/* todo: use fixed-point? */
static int op_SCFS(struct font* this) {
	struct vec2f F;  	/* freedom vec */
	struct vec2f P;  	/* projection vec */
	struct vec2f pnt;	/* point */
	
	float C;
	
	if (REG_SP < 2) return -1;
	
	F.x = (float) FRD_VEC.x / 16384.0;
	F.y = (float) FRD_VEC.y / 16384.0;
	
	P.x = (float) PRJ_VEC.x / 16384.0;
	P.y = (float) PRJ_VEC.y / 16384.0;
	
	pnt.y = (float) ZONEP2(STACK(1)).pos.y;
	pnt.x = (float) ZONEP2(STACK(1)).pos.x;
	
	C = ( ( (float) STACK(0) ) - dot2f(P, pnt) ) / dot2f(P, F);
	
	ZONEP2(STACK(1)).pos.x += (int32_t) (F.x * C);
	ZONEP2(STACK(1)).pos.y += (int32_t) (F.y * C);
	
	REG_SP -= 2;
	
	return 0;
}

static int op_SANGW(struct font* this) {
	REG_SP -= (REG_SP > 0);
	
	return 0;
}

static int op_RUTG(struct font* this) {
	
	ROUND_STATE = 0x40;
	GRAPHICS_STATE.round_state = 4;
	update_round_state(this);
	
	return 0;
}

static int op_RTHG(struct font* this) {
	
	ROUND_STATE = 0x68;
	GRAPHICS_STATE.round_state = 0;
	update_round_state(this);
	
	return 0;
}

static int op_RTG(struct font* this) {
	
	ROUND_STATE = 0x48;
	GRAPHICS_STATE.round_state = 1;
	update_round_state(this);
	
	return 0;
}

static int op_RTDG(struct font* this) {
	
	ROUND_STATE = 0x08;
	GRAPHICS_STATE.round_state = 2;
	update_round_state(this);
	
	return 0;
}

static int op_ROFF(struct font* this) {
	GRAPHICS_STATE.round_state = 5;
	
	return 0;
}

static int op_RDTG(struct font* this) {
	
	ROUND_STATE = 0x44;
	GRAPHICS_STATE.round_state = 3;
	update_round_state(this);
	
	return 0;
}

static int op_RCVT(struct font* this) {
	if (REG_SP < 1) return -1;
	
	if (STACK(0) < MAX_CONTROL_VALUES)
		STACK(0) = CONTROL_VALUE(STACK(0));
	
	else return -1;
	
	return 0;
}

static int op_JROT(struct font* this) {
	if (REG_SP < 2) return -1;
	
	if (STACK(0))
		REG_EP += STACK(1) - 1;
	
	REG_SP -= 2;
	
	return 0;
}

static int op_JROF(struct font* this) {
	if (REG_SP < 2) return -1;
	
	if (!STACK(0))
		REG_EP += STACK(1) - 1;
	
	REG_SP -= 2;
	
	return 0;
}

static int op_JMPR(struct font* this) {
	if (REG_SP < 1) return -1;
	
	REG_EP += STACK(0) - 1;
	REG_SP--;
	
	return 0;
}

static int op_ISECT(struct font* this) {
	if (REG_SP < 5) return -1;
	
	ZONEP2(STACK(0)).pos.x = (
		(ZONEP0(STACK(4)).pos.x + ZONEP0(STACK(3)).pos.x) / 2 +
		(ZONEP1(STACK(2)).pos.x + ZONEP1(STACK(1)).pos.x) / 2
	) / 2;
	
	
	ZONEP2(STACK(0)).pos.y = (
		(ZONEP0(STACK(4)).pos.y + ZONEP0(STACK(3)).pos.y) / 2 +
		(ZONEP1(STACK(2)).pos.y + ZONEP1(STACK(1)).pos.y) / 2
	) / 2;
	
	REG_SP -= 5;
	
	return 0;
}

static int op_GPV(struct font* this) {
	REG_SP += 2;
	STACK(0) = PRJ_VEC.y;
	STACK(1) = PRJ_VEC.x;
	
	return 0;
}

static int op_GFV(struct font* this) {
	REG_SP += 2;
	STACK(0) = FRD_VEC.y;
	STACK(1) = FRD_VEC.x;
	
	return 0;
}

static int op_FLIPRGON(struct font* this) {
	UINT32 i;
	
	if (REG_SP < 2) return -1;
	
	for (i = STACK(1); i < STACK(0); i++)
		ZONEP0(i).flags |= POINT_MASK_ONCURVE;
	
	return 0;
}

static int op_FLIPRGOFF(struct font* this) {
	UINT32 i;
	
	if (REG_SP < 2) return -1;
	
	for (i = STACK(1); i < STACK(0); i++)
		ZONEP0(i).flags &= ~POINT_MASK_ONCURVE;
	
	return 0;
}

static int op_FLIPPT(struct font* this) {
	if (REG_SP < 1) return -1;
	
	if (ZONEP0(STACK(0)).flags & POINT_MASK_ONCURVE)
		ZONEP0(STACK(0)).flags &= ~POINT_MASK_ONCURVE;
	else
		ZONEP0(STACK(0)).flags |= POINT_MASK_ONCURVE;
	
	return 0;
}

static int op_FLIPON(struct font* this) {
	GRAPHICS_STATE.flags.auto_flip = 1;
	
	return 0;
}

static int op_FLIPOFF(struct font* this) {
	GRAPHICS_STATE.flags.auto_flip = 0;
	
	return 0;
}

static int op_DEBUG(struct font* this) {
	REG_SP -= (REG_SP > 0);
	
	return 0;
}

/* todo: use fixed-point? */
static int op_ALIGNRP(struct font* this) {
	struct vec2f F; /* freedom vec */
	struct vec2f P; /* projection vec */
	
	struct fuvec epsilon;
	struct vec2f dp;
	
	float C;
	uint32_t i;
	
	if (REG_SP < REG_LP) return -1;
	
	F.x = (float) FRD_VEC.x;
	F.y = (float) FRD_VEC.y;
	
	P.x = (float) PRJ_VEC.x;
	P.y = (float) PRJ_VEC.y;
	
	for (i = 0; i < REG_LP; i++) {
		dp.x = (float) (ZONEP0(REG_RP0).pos.x - ZONEP1(STACK(i)).pos.x);
		dp.y = (float) (ZONEP0(REG_RP0).pos.y - ZONEP1(STACK(i)).pos.y);
		
		C = dot2f(P, dp) / dot2f(P, F);
		
		epsilon.x = (int32_t) (F.x * C);
		epsilon.y = (int32_t) (F.y * C);
		
		ZONEP1(STACK(i)).pos.x += epsilon.x;
		ZONEP1(STACK(i)).pos.y += epsilon.y;
	}
	
	REG_SP -= REG_LP;
	
	return 0;
}

/* todo: use fixed-point? */
static int op_ALIGNPTS(struct font* this) {
	struct vec2f F; /* freedom vec */
	struct vec2f P; /* projection vec */
	
	struct fuvec epsilon;
	struct vec2f dp;
	
	float C;
	
	if (REG_SP < 2) return -1;
	
	F.x = (float) FRD_VEC.x;
	F.y = (float) FRD_VEC.y;
	
	P.x = (float) PRJ_VEC.x;
	P.y = (float) PRJ_VEC.y;
	
	dp.x = (float) (ZONEP0(STACK(0)).pos.x - ZONEP1(STACK(1)).pos.x);
	dp.y = (float) (ZONEP0(STACK(0)).pos.y - ZONEP1(STACK(1)).pos.y);
	
	C = dot2f(P, dp) / ( 2.0 * dot2f(P, F) );
	
	epsilon.x = (int32_t) (F.x * C);
	epsilon.y = (int32_t) (F.y * C);
	
	ZONEP0(STACK(0)).pos.x -= epsilon.x;
	ZONEP0(STACK(0)).pos.y -= epsilon.y;
	
	ZONEP1(STACK(1)).pos.x += epsilon.x;
	ZONEP1(STACK(1)).pos.y += epsilon.y;
	
	REG_SP -= 2;
	
	return 0;
}

static int op_AA(struct font* this) {
	REG_SP -= (REG_SP > 0);
	
	return 0;
}

/* todo: don't assume stream actually has shit within its allocation */
static int op_NPUSHW(struct font* this) {
	BYTE N = STREAM(1);
	BYTE i;
	
	/* make room for N elements on the stack */
	REG_SP += N;
	
	/* push elements */
	for (i = 0; i < N; i++) {
		USTACK(N - i - 1) = STREAM(2 + 2 * i) << 8;
		USTACK(N - i - 1) |= STREAM(2 + 2 * i + 1);
	}
	
	/* advance execution */
	REG_EP += 2 * i + 1;
	
	return 0;
}

/* todo: don't assume stream actually has shit within its allocation */
static int op_NPUSHB(struct font* this) {
	BYTE N = STREAM(1);
	BYTE i;
	
	/* make room for N elements on the stack */
	REG_SP += N;
	
	/* push elements */
	for (i = 0; i < N; i++)
		USTACK(N - i - 1) = STREAM(2 + i);
	
	/* advance execution */
	REG_EP += i + 1;
	
	return 0;
}

static int op_WS(struct font* this) {
	if (REG_SP < 2) return -1;
	
	/* make sure we are within addressable memory */
	if (STACK(1) < MAX_STORAGE)
		MEMORY(STACK(1)) = STACK(0);
	
	else return -1;
	
	REG_SP -= 2;
	
	return 0;
}

static int op_RS(struct font* this) {
	if (REG_SP < 1) return -1;
	
	/* make sure we are within addressable memory */
	if (STACK(0) < MAX_STORAGE)
		STACK(0) = MEMORY(STACK(0));
	
	else return -1;
	
	return 0;
}

static int op_SZPS(struct font* this) {
	if (REG_SP < 1) return -1;
	
	/* make sure argument makes sense (only 3 zp regs) */
	if (STACK(0) > 2) return -1;
	
	REG_ZP0 = STACK(0);
	REG_ZP1 = STACK(0);
	REG_ZP2 = STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_NEG(struct font* this) {
	if (REG_SP < 1) return -1;
	
	SSTACK(0) = -SSTACK(0);
	
	return 0;
}

static int op_MIN(struct font* this) {
	if (REG_SP < 2) return -1;
	
	if (SSTACK(0) < SSTACK(1))
		STACK(1) = STACK(0);
	
	REG_SP--;
	
	return 0;
}

static int op_MAX(struct font* this) {
	if (REG_SP < 2) return -1;
	
	if (SSTACK(0) > SSTACK(1))
		STACK(1) = STACK(0);
	
	REG_SP--;
	
	return 0;
}

static int op_ABS(struct font* this) {
	if (REG_SP < 1) return -1;
	
	SSTACK(0) = ABS(SSTACK(0));
	
	return 0;
}

static int op_CEILING(struct font* this) {
	if (REG_SP < 1) return -1;
	
	if (STACK(0) & F26D6_FRAC_MASK)
		SSTACK(0) += 64;
	
	STACK(0) &= ~F26D6_FRAC_MASK;
	
	return 0;
}

static int op_FLOOR(struct font* this) {
	if (REG_SP < 1) return -1;
	
	STACK(0) &= ~F26D6_FRAC_MASK;
	
	return 0;
}

static int op_DIV(struct font* this) {
	if (REG_SP < 2) return -1;
	
	SSTACK(1) = F26D6DIV(SSTACK(1), SSTACK(0));
	REG_SP--;
	
	return 0;
}

static int op_MUL(struct font* this) {
	if (REG_SP < 2) return -1;
	
	SSTACK(1) = F26D6MUL(SSTACK(1), SSTACK(0));
	REG_SP--;
	
	return 0;
}

static int op_SUB(struct font* this) {
	if (REG_SP < 2) return -1;
	
	SSTACK(1) = SSTACK(1) - SSTACK(0);
	REG_SP--;
	
	return 0;
}

static int op_ADD(struct font* this) {
	if (REG_SP < 2) return -1;
	
	SSTACK(1) = SSTACK(1) + SSTACK(0);
	REG_SP--;
	
	return 0;
}

static int op_NOT(struct font* this) {
	if (REG_SP < 1) return -1;
	
	STACK(0) = !!!STACK(0);
	
	return 0;
}

static int op_OR(struct font* this) {
	if (REG_SP < 2) return -1;
	
	STACK(1) = STACK(1) || STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_AND(struct font* this) {
	if (REG_SP < 2) return -1;
	
	STACK(1) = STACK(1) && STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_EVEN(struct font* this) {
	F26D6 val;
	
	if (REG_SP < 1) return -1;
	
	/* round val according to round state and then truncate to the nearest
	 * integer before checking if it is even or not. */
	val = furound(this, STACK(0));
	val -= (val % 64 + 64) % 64;
	val >>= 6;
	
	STACK(0) = !(val % 2);
	
	return 0;
}

static int op_ODD(struct font* this) {
	F26D6 val;
	
	if (REG_SP < 1) return -1;
	
	/* round val according to round state and then truncate to the nearest
	 * integer before checking if it is even or not. */
	val = furound(this, STACK(0));
	val -= (val % 64 + 64) % 64;
	val >>= 6;
	
	STACK(0) = val % 2;
	
	return 0;
}

static int op_NEQ(struct font* this) {
	if (REG_SP < 2) return -1;
	
	STACK(1) = STACK(1) != STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_EQ(struct font* this) {
	if (REG_SP < 2) return -1;
	
	STACK(1) = STACK(1) == STACK(0);
	REG_SP--;
	
	return 0;
}

static int op_GTEQ(struct font* this) {
	if (REG_SP < 2) return -1;
	
	STACK(1) = SSTACK(1) >= SSTACK(0);
	REG_SP--;
	
	return 0;
}

static int op_GT(struct font* this) {
	if (REG_SP < 2) return -1;
	
	STACK(1) = SSTACK(1) > SSTACK(0);
	REG_SP--;
	
	return 0;
}

static int op_LTEQ(struct font* this) {
	if (REG_SP < 2) return -1;
	
	STACK(1) = SSTACK(1) <= SSTACK(0);
	REG_SP--;
	
	return 0;
}

static int op_LT(struct font* this) {
	if (REG_SP < 2) return -1;
	
	STACK(1) = SSTACK(1) < SSTACK(0);
	REG_SP--;
	
	return 0;
}

static int op_SWAP(struct font* this) {
	if (REG_SP < 2) return -1;
	
	SWAP32( &STACK(1), &STACK(0) );
	
	return 0;
}

static int op_ROLL(struct font* this) {
	if (REG_SP < 3) return -1;
	
	SWAP32( &STACK(0), &STACK(2) );
	SWAP32( &STACK(1), &STACK(2) );
	
	return 0;
}

static int op_MINDEX(struct font* this) {
	UINT32 element;
	UINT32 i;
	
	if (REG_SP < 1) return -1;
	
	/* make sure we are not outside the stack */
	if (USTACK(0) + 1 > REG_SP) return -1;
	
	/* save element */
	element = STACK(STACK(0));
	
	/* remove element from stack */
	for (i = STACK(0); i > 0; i--)
		STACK(i) = STACK(i - 1);
	
	/* push element to top of stack */
	STACK(1) = element;
	REG_SP--;
	
	return 0;
}

static int op_CINDEX(struct font* this) {
	if (REG_SP < 1) return -1;
	
	/* make sure we are not addressing outside the stack */
	if (USTACK(0) + 1 > REG_SP) return -1;
	
	/* copy element to top of stack */
	STACK(0) = STACK(STACK(0));
	
	return 0;
}

static int op_POP(struct font* this) {
	REG_SP -= (REG_SP > 0);
	
	return 0;
}

static int op_DUP(struct font* this) {
	if (REG_SP > 0) {
		REG_SP++;
		STACK(0) = STACK(1);
	}
	
	return 0;
}

static int op_DEPTH(struct font* this) {
	REG_SP++;
	STACK(0) = REG_SP - 1;
	
	return 0;
}

static int op_CLEAR(struct font* this) {
	REG_SP = 0;
	
	return 0;
}

int (*instructions[INSTR_MAX])(struct font*) = {
/*	[SVTCA + 0    ] */ op_SVTCA_0,
/*	[SVTCA + 1    ] */ op_SVTCA_1,
/*	[SPVTCA + 0   ] */ op_SPVTCA_0,
/*	[SPVTCA + 1   ] */ op_SPVTCA_1,
/*	[SFVTCA + 0   ] */ op_SFVTCA_0,
/*	[SFVTCA + 1   ] */ op_SFVTCA_1,
/*	[SPVTL + 0    ] */ op_SPVTL_0,
/*	[SPVTL + 1    ] */ op_SPVTL_1,
/*	[SFVTL + 0    ] */ op_SFVTL_0,
/*	[SFVTL + 1    ] */ op_SFVTL_1,
/*	[SPVFS        ] */ op_SPVFS,
/*	[SFVFS        ] */ op_SFVFS,
/*	[GPV          ] */ op_GPV,
/*	[GFV          ] */ op_GFV,
/*	[SFVTPV       ] */ op_SFVTPV,
/*	[ISECT        ] */ op_ISECT,
/*	[SRP + 0      ] */ op_SRP_0,
/*	[SRP + 1      ] */ op_SRP_1,
/*	[SRP + 2      ] */ op_SRP_2,
/*	[SZP + 0      ] */ op_SZP_0,
/*	[SZP + 1      ] */ op_SZP_1,
/*	[SZP + 2      ] */ op_SZP_2,
/*	[SZPS         ] */ op_SZPS,
/*	[SLOOP        ] */ op_SLOOP,
/*	[RTG          ] */ op_RTG,
/*	[RTHG         ] */ op_RTHG,
/*	[SMD          ] */ op_SMD,
/*	[ELSE         ] */ op_ELSE,
/*	[JMPR         ] */ op_JMPR,
/*	[SCVTCI       ] */ op_SCVTCI,
/*	[SSWCI        ] */ op_SSWCI,
/*	[SSW          ] */ op_SSW,
/*	[DUP          ] */ op_DUP,
/*	[POP          ] */ op_POP,
/*	[CLEAR        ] */ op_CLEAR,
/*	[SWAP         ] */ op_SWAP,
/*	[DEPTH        ] */ op_DEPTH,
/*	[CINDEX       ] */ op_CINDEX,
/*	[MINDEX       ] */ op_MINDEX,
/*	[ALIGNPTS     ] */ op_ALIGNPTS,
/*	[UDEF0        ] */ NULL,
/*	[UTP          ] */ op_UTP,
/*	[LOOPCALL     ] */ op_LOOPCALL,
/*	[CALL         ] */ op_CALL,
/*	[FDEF         ] */ op_FDEF,
/*	[ENDF         ] */ op_ENDF,
/*	[MDAP + 0     ] */ op_MDAP_0,
/*	[MDAP + 1     ] */ op_MDAP_1,
/*	[IUP + 0      ] */ op_IUP_0,
/*	[IUP + 1      ] */ op_IUP_1,
/*	[SHP + 0      ] */ op_SHP_0,
/*	[SHP + 1      ] */ op_SHP_1,
/*	[SHC + 0      ] */ op_SHC_0,
/*	[SHC + 1      ] */ op_SHC_1,
/*	[SHZ + 0      ] */ op_SHZ_0,
/*	[SHZ + 1      ] */ op_SHZ_1,
/*	[SHPIX        ] */ op_SHPIX,
/*	[IP           ] */ op_IP,
/*	[MSIRP + 0    ] */ op_MSIRP_0,
/*	[MSIRP + 1    ] */ op_MSIRP_1,
/*	[ALIGNRP      ] */ op_ALIGNRP,
/*	[RTDG         ] */ op_RTDG,
/*	[MIAP + 0     ] */ op_MIAP_0,
/*	[MIAP + 1     ] */ op_MIAP_1,
/*	[NPUSHB       ] */ op_NPUSHB,
/*	[NPUSHW       ] */ op_NPUSHW,
/*	[WS           ] */ op_WS,
/*	[RS           ] */ op_RS,
/*	[WCVTP        ] */ op_WCVTP,
/*	[RCVT         ] */ op_RCVT,
/*	[GC + 0       ] */ op_GC_0,
/*	[GC + 1       ] */ op_GC_1,
/*	[SCFS         ] */ op_SCFS,
/*	[MD + 0       ] */ NULL,
/*	[MD + 1       ] */ NULL,
/*	[MPPEM        ] */ op_MPPEM,
/*	[MPS          ] */ op_MPS,
/*	[FLIPON       ] */ op_FLIPON,
/*	[FLIPOFF      ] */ op_FLIPOFF,
/*	[DEBUG        ] */ op_DEBUG,
/*	[LT           ] */ op_LT,
/*	[LTEQ         ] */ op_LTEQ,
/*	[GT           ] */ op_GT,
/*	[GTEQ         ] */ op_GTEQ,
/*	[EQ           ] */ op_EQ,
/*	[NEQ          ] */ op_NEQ,
/*	[ODD          ] */ op_ODD,
/*	[EVEN         ] */ op_EVEN,
/*	[IF           ] */ op_IF,
/*	[EIF          ] */ op_EIF,
/*	[AND          ] */ op_AND,
/*	[OR           ] */ op_OR,
/*	[NOT          ] */ op_NOT,
/*	[DELTAP1      ] */ op_DELTAP1,
/*	[SDB          ] */ op_SDB,
/*	[SDS          ] */ op_SDS,
/*	[ADD          ] */ op_ADD,
/*	[SUB          ] */ op_SUB,
/*	[DIV          ] */ op_DIV,
/*	[MUL          ] */ op_MUL,
/*	[ABS          ] */ op_ABS,
/*	[NEG          ] */ op_NEG,
/*	[FLOOR        ] */ op_FLOOR,
/*	[CEILING      ] */ op_CEILING,
/*	[ROUND + 0    ] */ op_ROUND_0,
/*	[ROUND + 1    ] */ op_ROUND_1,
/*	[ROUND + 2    ] */ op_ROUND_2,
/*	[ROUND + 3    ] */ op_ROUND_3,
/*	[NROUND + 0   ] */ op_NROUND_0,
/*	[NROUND + 1   ] */ op_NROUND_1,
/*	[NROUND + 2   ] */ op_NROUND_2,
/*	[NROUND + 3   ] */ op_NROUND_3,
/*	[WCVTF        ] */ op_WCVTF,
/*	[DELTAP2      ] */ op_DELTAP2,
/*	[DELTAP3      ] */ op_DELTAP3,
/*	[DELTAC1      ] */ op_DELTAC1,
/*	[DELTAC2      ] */ op_DELTAC2,
/*	[DELTAC3      ] */ op_DELTAC3,
/*	[SROUND       ] */ op_SROUND,
/*	[S45ROUND     ] */ op_S45ROUND,
/*	[JROT         ] */ op_JROT,
/*	[JROF         ] */ op_JROF,
/*	[ROFF         ] */ op_ROFF,
/*	[UDEF1        ] */ NULL,
/*	[RUTG         ] */ op_RUTG,
/*	[RDTG         ] */ op_RDTG,
/*	[SANGW        ] */ op_SANGW,
/*	[AA           ] */ op_AA,
/*	[FLIPPT       ] */ op_FLIPPT,
/*	[FLIPRGON     ] */ op_FLIPRGON,
/*	[FLIPRGOFF    ] */ op_FLIPRGOFF,
/*	[UDEF2        ] */ NULL,
/*	[UDEF3        ] */ NULL,
/*	[SCANCTRL     ] */ op_SCANCTRL,
/*	[SDPVTL + 0   ] */ op_SDPVTL_0,
/*	[SDPVTL + 1   ] */ op_SDPVTL_1,
/*	[GETINFO      ] */ op_GETINFO,
/*	[IDEF         ] */ op_IDEF,
/*	[ROLL         ] */ op_ROLL,
/*	[MAX          ] */ op_MAX,
/*	[MIN          ] */ op_MIN,
/*	[SCANTYPE     ] */ op_SCANTYPE,
/*	[INSTCTRL     ] */ op_INSTCTRL,
/*	[UDEF4        ] */ NULL,
/*	[UDEF5        ] */ NULL,
/*	[GETVARIATION ] */ op_GETVARIATION,
/*	[UDEF6        ] */ NULL,
/*	[UDEF7        ] */ NULL,
/*	[UDEF8        ] */ NULL,
/*	[UDEF9        ] */ NULL,
/*	[UDEFA        ] */ NULL,
/*	[UDEFB        ] */ NULL,
/*	[UDEFC        ] */ NULL,
/*	[UDEFD        ] */ NULL,
/*	[UDEFE        ] */ NULL,
/*	[UDEFF        ] */ NULL,
/*	[UDEFG        ] */ NULL,
/*	[UDEFH        ] */ NULL,
/*	[UDEFI        ] */ NULL,
/*	[UDEFJ        ] */ NULL,
/*	[UDEFK        ] */ NULL,
/*	[UDEFL        ] */ NULL,
/*	[UDEFM        ] */ NULL,
/*	[UDEFN        ] */ NULL,
/*	[UDEFO        ] */ NULL,
/*	[UDEFP        ] */ NULL,
/*	[UDEFQ        ] */ NULL,
/*	[UDEFR        ] */ NULL,
/*	[UDEFS        ] */ NULL,
/*	[UDEFT        ] */ NULL,
/*	[UDEFU        ] */ NULL,
/*	[UDEFV        ] */ NULL,
/*	[UDEFW        ] */ NULL,
/*	[UDEFX        ] */ NULL,
/*	[UDEFY        ] */ NULL,
/*	[UDEFZ        ] */ NULL,
/*	[PUSHB + 0    ] */ op_PUSHB_0,
/*	[PUSHB + 1    ] */ op_PUSHB_1,
/*	[PUSHB + 2    ] */ op_PUSHB_2,
/*	[PUSHB + 3    ] */ op_PUSHB_3,
/*	[PUSHB + 4    ] */ op_PUSHB_4,
/*	[PUSHB + 5    ] */ op_PUSHB_5,
/*	[PUSHB + 6    ] */ op_PUSHB_6,
/*	[PUSHB + 7    ] */ op_PUSHB_7,
/*	[PUSHW + 0    ] */ op_PUSHW_0,
/*	[PUSHW + 1    ] */ op_PUSHW_1,
/*	[PUSHW + 2    ] */ op_PUSHW_2,
/*	[PUSHW + 3    ] */ op_PUSHW_3,
/*	[PUSHW + 4    ] */ op_PUSHW_4,
/*	[PUSHW + 5    ] */ op_PUSHW_5,
/*	[PUSHW + 6    ] */ op_PUSHW_6,
/*	[PUSHW + 7    ] */ op_PUSHW_7,
/*	[MDRP +  0    ] */ NULL,
/*	[MDRP +  1    ] */ NULL,
/*	[MDRP +  2    ] */ NULL,
/*	[MDRP +  3    ] */ NULL,
/*	[MDRP +  4    ] */ NULL,
/*	[MDRP +  5    ] */ NULL,
/*	[MDRP +  6    ] */ NULL,
/*	[MDRP +  7    ] */ NULL,
/*	[MDRP +  8    ] */ NULL,
/*	[MDRP +  9    ] */ NULL,
/*	[MDRP + 10    ] */ NULL,
/*	[MDRP + 11    ] */ NULL,
/*	[MDRP + 12    ] */ NULL,
/*	[MDRP + 13    ] */ NULL,
/*	[MDRP + 14    ] */ NULL,
/*	[MDRP + 15    ] */ NULL,
/*	[MDRP + 16    ] */ NULL,
/*	[MDRP + 17    ] */ NULL,
/*	[MDRP + 18    ] */ NULL,
/*	[MDRP + 19    ] */ NULL,
/*	[MDRP + 20    ] */ NULL,
/*	[MDRP + 21    ] */ NULL,
/*	[MDRP + 22    ] */ NULL,
/*	[MDRP + 23    ] */ NULL,
/*	[MDRP + 24    ] */ NULL,
/*	[MDRP + 25    ] */ NULL,
/*	[MDRP + 26    ] */ NULL,
/*	[MDRP + 27    ] */ NULL,
/*	[MDRP + 28    ] */ NULL,
/*	[MDRP + 29    ] */ NULL,
/*	[MDRP + 30    ] */ NULL,
/*	[MDRP + 31    ] */ NULL,
/*	[MIRP +  0    ] */ NULL,
/*	[MIRP +  1    ] */ NULL,
/*	[MIRP +  2    ] */ NULL,
/*	[MIRP +  3    ] */ NULL,
/*	[MIRP +  4    ] */ NULL,
/*	[MIRP +  5    ] */ NULL,
/*	[MIRP +  6    ] */ NULL,
/*	[MIRP +  7    ] */ NULL,
/*	[MIRP +  8    ] */ NULL,
/*	[MIRP +  9    ] */ NULL,
/*	[MIRP + 10    ] */ NULL,
/*	[MIRP + 11    ] */ NULL,
/*	[MIRP + 12    ] */ NULL,
/*	[MIRP + 13    ] */ NULL,
/*	[MIRP + 14    ] */ NULL,
/*	[MIRP + 15    ] */ NULL,
/*	[MIRP + 16    ] */ NULL,
/*	[MIRP + 17    ] */ NULL,
/*	[MIRP + 18    ] */ NULL,
/*	[MIRP + 19    ] */ NULL,
/*	[MIRP + 20    ] */ NULL,
/*	[MIRP + 21    ] */ NULL,
/*	[MIRP + 22    ] */ NULL,
/*	[MIRP + 23    ] */ NULL,
/*	[MIRP + 24    ] */ NULL,
/*	[MIRP + 25    ] */ NULL,
/*	[MIRP + 26    ] */ NULL,
/*	[MIRP + 27    ] */ NULL,
/*	[MIRP + 28    ] */ NULL,
/*	[MIRP + 29    ] */ NULL,
/*	[MIRP + 30    ] */ NULL,
/*	[MIRP + 31    ] */ NULL,
};

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
	"~UDEF0",
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
	"~UDEF1",
	"RUTG",
	"RDTG",
	"SANGW",
	"AA",
	"FLIPPT",
	"FLIPRGON",
	"FLIPRGOFF",
	"~UDEF2",
	"~UDEF3",
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
	"~UDEF4",
	"~UDEF5",
	"GETVARIATION",
	"~UDEF6",
	"~UDEF7",
	"~UDEF8",
	"~UDEF9",
	"~UDEFA",
	"~UDEFB",
	"~UDEFC",
	"~UDEFD",
	"~UDEFE",
	"~UDEFF",
	"~UDEFG",
	"~UDEFH",
	"~UDEFI",
	"~UDEFJ",
	"~UDEFK",
	"~UDEFL",
	"~UDEFM",
	"~UDEFN",
	"~UDEFO",
	"~UDEFP",
	"~UDEFQ",
	"~UDEFR",
	"~UDEFS",
	"~UDEFT",
	"~UDEFU",
	"~UDEFV",
	"~UDEFW",
	"~UDEFX",
	"~UDEFY",
	"~UDEFZ",
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

int get_command_opcode(char* _name) {
	int i;
	
	/* search for name */
	for (i = 0; i < INSTR_MAX; i++)
		if (!strcmp(command_names[i], _name))
			return (BYTE) i;
	
	/* if no match was found */
	return -1;
}
