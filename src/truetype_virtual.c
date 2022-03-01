#include "error.h"
#include "specific.h"

#include "truetype_virtual.h"

/* TO BE REMOVED */
#include "linuxfb.h"
#include "lfb2d.h"

int GFVMERRNO = 0;

char* GFVMERRMSGS[] = {
	"OK (0x00)",
	"STACK OVERFLOW (0x01) : most likely the stack has reached its capacity",
	"STACK UNDERFLOW (0x02) : attempted to pop from an empty stack",
	"EXECUTION OVERFLOW (0x03) : attempted to access data outside the current execution stream",
	"EXECUTION UNDERFLOW (0x04) : most likey a jump took execution out of bounds",
	"CALL DEPTH EXCEEDED (0x05) : call stack capacity has been exceeded",
	"SEGMENTATION FAULT (0x06) : attempted to access out-of-bounds memory (CVT, STORAGE, or otherwise)",
	"DIVIDE BY ZERO (0x07) : attempted to divide by zero, most likey a vector has been set with both conponents zero",
	"INVALID ZONE (0x08) : attempted to reference a non-existant zone (index > 1)"
};

void fvm_perror(void) {
	fprintf(stderr, "TTFVM ERROR : %s\n", GFVMERRMSGS[GFVMERRNO]);
}

static void __debug_draw_glyph_to_fb(rgbx32 _c, int _npnts, struct point* _zone, int _xoff) {
	int  i;
	
	for (i = 0; i < _npnts - 4; i++)
		fb_draw_point(_c, _zone[i].pos.x / 2 + _xoff, -_zone[i].pos.y / 2 + 512);
}

void ppix(int x, int y) {
	sbuf[y][x] = WHITEX32;
}

void pswp() {
	fb_swap();
}

struct vm* make_virtual(struct font* _fnt) {
	struct vm* virtm = smalloc(sizeof(struct vm), 1);
	
	/* allocate zones */
	
	/* allocate room to store the original points of a glyph outline.
	 * note that we will not allocate memory to store points in the
	 * working area of zone 1. this memory will be provided by the
	 * glyph when init_virtual(this, _state) is called (see below).
	 * this is also true for zone 1 contours. */
	virtm->zone[1].original_points = smalloc(sizeof(struct point), 
		_fnt->max.points);
	
	/* the twilight zone has a set number of points and a single
	 * virtual contour. these will never change, so we will set them
	 * here. */
	virtm->zone[0].num_points = _fnt->max.twilight;
	virtm->zone[0].contours = smalloc(sizeof(INT32), 2);
	virtm->zone[0].num_contours = 1;
	virtm->zone[0].contours[1] = _fnt->max.twilight - 1;
	virtm->zone[0].contours[0] = -1;
		
	virtm->zone[0].current_points = scalloc(sizeof(struct point),
		_fnt->max.twilight);
	virtm->zone[0].original_points = scalloc(sizeof(struct point),
		_fnt->max.twilight);
	
	/* alloacte space for IDEFs and FDEFs */
	virtm->functions = scalloc(sizeof(struct proceedure), _fnt->max.function_defs);
	virtm->userdefs = scalloc(sizeof(struct proceedure), _fnt->max.instruction_defs);
	
	/* allocate more things... */
	virtm->stack = smalloc(sizeof(UINT32), _fnt->max.stack_depth);
	virtm->memory = _fnt->storage;
	virtm->cvt = _fnt->cvt;
	virtm->stream = NULL;
	
	/* copy data from font */
	virtm->point_size = _fnt->point_size;
	virtm->fupem = _fnt->fupem;
	virtm->ppem = _fnt->ppem;
	
	virtm->max.instruction_defs = _fnt->max.instruction_defs;
	virtm->max.control_values = _fnt->max.control_values;
	virtm->max.function_defs = _fnt->max.function_defs;
	virtm->max.stack_depth = _fnt->max.stack_depth;
	virtm->max.storage = _fnt->max.storage;
	
	/* the virtual machine has not been initialised (obviously). */
	virtm->is_ready = 0;
	
	return virtm;
}

int init_virtual(struct vm* this, struct vm_state _state) {
	/* make sure we aren't screwing up... */
	if (this == NULL || _state.instructions == NULL) return -1;
	
	/* copy the instructions into the machine... */
	this->stream_length = _state.num_instructions;
	this->stream = _state.instructions;
	
	/* if a glyph was provided ,copy its data into the machine,
	 * otherwise, set num_points and num_contours to zero (we are
	 * probably running the 'prep' or font program). */
	if (_state.glyph) {
		/* check glyph is OK... */
		if (_state.glyph->contours == NULL ||
		    _state.glyph->points == NULL) return -1;
		
		this->zone[1].num_contours = _state.glyph->num_contours;
		this->zone[1].contours = _state.glyph->contours;
		
		this->zone[1].num_points = _state.glyph->num_points;
		this->zone[1].current_points = _state.glyph->points;
		
		/* keep a copy of the glyph's original outline. */
		memcpy(this->zone[1].original_points,
			this->zone[1].current_points, sizeof(struct point) *
			_state.glyph->num_points);
	} else {
		this->zone[1].num_contours = 0;
		this->zone[1].contours = NULL;
		
		this->zone[1].num_points = 0;
		this->zone[1].current_points = NULL;
	}
	
	/* initialise registers */
	this->reg[EP ] = 1;
	this->reg[LP ] = 1;
	this->reg[SP ] = 0;
	this->reg[CSP] = 0;
	this->reg[RP0] = 0;
	this->reg[RP1] = 0;
	this->reg[RP2] = 0;
	this->reg[ZP0] = 1;
	this->reg[ZP1] = 1;
	this->reg[ZP2] = 1;
	
	/* initialise graphics state */
	this->graphics_state.control_value_cut_in = 68;
	this->graphics_state.single_width_cut_in = 0;
	this->graphics_state.single_width_value = 0;
	this->graphics_state.minimum_distance = 64;
	this->graphics_state.delta_shift = 3;
	this->graphics_state.delta_base = 9;
	
	this->graphics_state.flags.instruct_control = 0;
	this->graphics_state.flags.scan_control = 0;
	this->graphics_state.flags.auto_flip = 1;
	
	this->graphics_state.floating_projection_vector.x = 1.0;
	this->graphics_state.floating_projection_vector.y = 0;
	this->graphics_state.projection_vector.x = 16384;
	this->graphics_state.projection_vector.y = 0;
	
	this->graphics_state.floating_dual_projection_vector.x = 1.0;
	this->graphics_state.floating_dual_projection_vector.y = 0;
	this->graphics_state.dual_projection_vector.x = 16384;
	this->graphics_state.dual_projection_vector.y = 0;
	
	this->graphics_state.floating_freedom_vector.x = 1.0;
	this->graphics_state.floating_freedom_vector.y = 0;
	this->graphics_state.freedom_vector.x = 16384;
	this->graphics_state.freedom_vector.y = 0;
	
	this->graphics_state.round.state = 0x48;
	this->graphics_state.round.grid_period = 64;
	this->graphics_state.round.threshold = 32;
	this->graphics_state.round.period = 64;
	this->graphics_state.round.phase = 0;
	
	/* we are ready! */
	this->is_ready = 1;
	
	return 0;
}

int exec_virtual(struct vm* vm, int d) {
	static int init = 0;
	
	int err;
	UINT32 i;
	int hh, kk;
	
	if (!init) {
		fb_init("/dev/fb0");
		init = 1;
	}
		printf("\nEXECUTING %d:\n\n", vm->stream_length);
	
	if (d) {
		
		__debug_draw_glyph_to_fb(cyan, vm->zone[1].num_points, vm->zone[1].current_points, 128);
		__debug_draw_glyph_to_fb(red, vm->zone[1].num_points, vm->zone[1].original_points, 384);
		
		for (hh = 0; hh < 800; hh += 32) {
			for (kk = 28; kk < 1000; kk += 32)
				fb_draw_pnt(WHITEX32, kk, hh);
		}
		
			fb_swap();
		
		
		getchar();
		
		for (i = 0; i <  vm->stream_length; i++) {
			printf("0x%02X: %s\n", vm->stream[i], command_names[vm->stream[i]]);
			if (i == 75) printf("\n");
		}
		
	}
	
	for (; vm->reg[EP] <= vm->stream_length; vm->reg[EP]++) {
		if (d) {
			if (vm->stream[vm->reg[EP] - 1] == FDEF) printf("FD %u\n", vm->reg[EP]);
			printf("- %s", command_names[vm->stream[vm->reg[EP] - 1]]);
		getchar();
		}
		
		if (vm->stream[vm->reg[EP] - 1] < 0xC0) {
			if (instructions[vm->stream[vm->reg[EP] - 1]])
				err = instructions[vm->stream[vm->reg[EP] - 1]](vm);
		}
		
		else if (vm->stream[vm->reg[EP] - 1] < 0xE0)
			err = __op_MDRP(vm, vm->stream[vm->reg[EP] - 1] - 0xC0);
		
		else
			err = __op_MIRP(vm, vm->stream[vm->reg[EP] - 1] - 0xE0);
		
		if (d) {
			printf("> RET %i (EP:%u, LP: %u, SP:%u)\n", err, vm->reg[EP], vm->reg[LP],vm->reg[SP]);
			
			__debug_draw_glyph_to_fb(cyan, vm->zone[1].num_points, vm->zone[1].current_points, 128);
			__debug_draw_glyph_to_fb(red, vm->zone[1].num_points, vm->zone[1].original_points, 384);
			
			for (hh = 0; hh < 800; hh += 32)
				for (kk = 28; kk < 1000; kk += 32)
					fb_draw_pnt(WHITEX32, kk, hh);
			
			fb_swap();
		}
	
	
		if (err) {
			GFVMERRNO = err;
			fvm_perror();exit(1);
		}
	}
	
	/* we are finished! */
	vm->is_ready = 0;
	
	return 0;
}

void free_virtual(struct vm* this) {
	if (this == NULL) return;
	
	if (this->zone[1].current_points)
		free(this->zone[1].current_points);
	
	if (this->zone[1].original_points)
		free(this->zone[1].original_points);
	
	if (this->zone[1].contours)
		free(this->zone[1].contours);
	
	if (this->zone[0].contours)
		free(this->zone[0].contours);
	
	if (this->zone[0].current_points)
		free(this->zone[0].current_points);
	
	if (this->zone[0].original_points)
		free(this->zone[0].original_points);
	
	if (this->functions) free(this->functions);
	if (this->userdefs) free(this->userdefs);
	if (this->stream) free(this->stream);
	if (this->stack) free(this->stack);
	
	free(this);
}
