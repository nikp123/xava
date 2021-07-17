#include <string.h>

#include "graphical.h"

void calculate_win_pos(struct config_params *p, int scrW, int scrH) {
	if(!strcmp(p->winA, "top")){
		p->wx = (scrW - p->w) / 2 + p->wx;
		p->wy -= p->shdw;
	}else if(!strcmp(p->winA, "bottom")){
		p->wx = (scrW - p->w) / 2 + p->wx;
		p->wy = (scrH - p->h) - p->wy + p->shdw;
	}else if(!strcmp(p->winA, "top_left")){
		p->wy -= p->shdw;
	}else if(!strcmp(p->winA, "top_right")){
		p->wx = (scrW - p->w) - p->wx;
		p->wy -= p->shdw;
	}else if(!strcmp(p->winA, "left")){
		p->wy = (scrH - p->h) / 2;
	}else if(!strcmp(p->winA, "right")){
		p->wx = (scrW - p->w) - p->wx;
		p->wy = (scrH - p->h) / 2 + p->wy;
	}else if(!strcmp(p->winA, "bottom_left")){
		p->wy = (scrH - p->h) - p->wy + p->shdw;
	}else if(!strcmp(p->winA, "bottom_right")){
		p->wx = (scrW - p->w) - p->wx;
		p->wy = (scrH - p->h) - p->wy + p->shdw;
	}else if(!strcmp(p->winA, "center")){
		p->wx = (scrW - p->w) / 2 + p->wx;
		p->wy = (scrH - p->h) / 2 + p->wy;
	}
	// Some error checking
	#ifdef DEBUG
		xavaLogCondition(p->wx > scrW-p->w, 
				"Screen out of bounds (X axis)");
		xavaLogCondition(p->wy > scrH-p->h, 
				"Screen out of bounds (Y axis)");
	#endif
}

