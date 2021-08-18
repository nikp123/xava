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
				"Screen out of bounds (X axis) (%d %d %d %d)",
				scrW, scrH, p->wx, p->w);
		xavaLogCondition(p->wy > (int)(scrH-p->h),
				"Screen out of bounds (Y axis) (%d %d %d %d)",
				scrW, scrH, p->wy, p->h);
	#endif
}

// default behaviour is to compensate for shadows (not sure if it's the smartest
// thing to do, but eh.)
void calculate_inner_win_pos(struct XAVA_HANDLE *xava,
		unsigned int newW, unsigned int newH) {
	struct config_params *conf = &xava->conf;

	// sanity check
	if(!conf->fullF || !conf->holdSizeF) {
		xava->w = newW;
		xava->h = newH;
		conf->w = xava->w;
		conf->h = xava->h - conf->shdw;
	}

	// if the screen is smaller we need to shrink the visualizer
	xava->w = newW;
	xava->h = newH;
	conf->w = MIN(xava->w, conf->w);
	conf->h = MIN(xava->h, conf->h);

	// if the window is the same as display
	if((xava->w == conf->w) && (xava->h == conf->h))
		return;

	if(!strcmp(conf->winA, "top")) {
		xava->x = (xava->w-conf->w)/2;
		xava->y = 0;
	} else if(!strcmp(conf->winA, "bottom")) {
		xava->x = (xava->w-conf->w)/2;
		xava->y = xava->h-conf->h-conf->shdw;
	} else if(!strcmp(conf->winA, "top_left")) {
		xava->x = 0;
		xava->y = 0;
	} else if(!strcmp(conf->winA, "top_right")) {
		xava->x = xava->w-conf->w;
		xava->y = 0;
	} else if(!strcmp(conf->winA, "left")) {
		xava->x = 0;
		xava->y = (xava->h-conf->h-conf->shdw)/2;
	} else if(!strcmp(conf->winA, "right")) {
		xava->x = xava->w-conf->w;
		xava->y = (xava->h-conf->h-conf->shdw)/2;
	} else if(!strcmp(conf->winA, "bottom_left")) {
		xava->x = 0;
		xava->y = xava->h-conf->h-conf->shdw;
	} else if(!strcmp(conf->winA, "bottom_right")) {
		xava->x = xava->w-conf->w;
		xava->y = xava->h-conf->h-conf->shdw;
	} else if(!strcmp(conf->winA, "center")) {
		xava->x = (xava->w-conf->w)/2;
		xava->y = (xava->h-conf->h-conf->shdw)/2;
	}

	xavaLog("Resized window to (w: %d, h: %d, x: %d, y: %d)",
			xava->w, xava->h, xava->x, xava->y);
}

