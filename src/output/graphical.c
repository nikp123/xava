#include <string.h>

#include "graphical.h"

void calculate_win_pos(struct config_params *p, uint32_t scrW, uint32_t scrH) {
	if(!strcmp(p->winA, "top")){
		p->wx = (int)(scrW - p->w) / 2 + p->wx;
		p->wy -= (int)p->shdw;
	}else if(!strcmp(p->winA, "bottom")){
		p->wx = (int)(scrW - p->w) / 2 + p->wx;
		p->wy = (int)(scrH - p->h) - p->wy + (int)p->shdw;
	}else if(!strcmp(p->winA, "top_left")){
		p->wy -= (int)p->shdw;
	}else if(!strcmp(p->winA, "top_right")){
		p->wx = (int)(scrW - p->w) - p->wx;
		p->wy -= (int)p->shdw;
	}else if(!strcmp(p->winA, "left")){
		p->wy = (int)(scrH - p->h) / 2;
	}else if(!strcmp(p->winA, "right")){
		p->wx = (int)(scrW - p->w) - p->wx;
		p->wy = (int)(scrH - p->h) / 2 + p->wy;
	}else if(!strcmp(p->winA, "bottom_left")){
		p->wy = (int)(scrH - p->h) - p->wy + (int)p->shdw;
	}else if(!strcmp(p->winA, "bottom_right")){
		p->wx = (int)(scrW - p->w) - p->wx;
		p->wy = (int)(scrH - p->h) - p->wy + (int)p->shdw;
	}else if(!strcmp(p->winA, "center")){
		p->wx = (int)(scrW - p->w) / 2 + p->wx;
		p->wy = (int)(scrH - p->h) / 2 + p->wy;
	}
	// Some error checking
	#ifdef DEBUG
		xavaLogCondition(p->wx > (int)(scrW-p->w), 
				"Screen out of bounds (X axis)");
		xavaLogCondition(p->wy > (int)(scrH-p->h), 
				"Screen out of bounds (Y axis)");
	#endif
}

