#include "graphical.h"
#include "../shared.h"
#include <stdio.h>
#include <string.h>
#include <math.h>


#ifdef GLX
	GLXContext xavaGLXContext;
	GLXFBConfig* xavaFBConfig;
#endif

void calculate_win_pos(struct config_params *p, int scrW, int scrH) {
	#if !defined(GL)
		p->shdw = 0;
	#endif

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

#ifdef GL
int drawGLBars(struct XAVA_HANDLE *hand, double colors[8], double gradColors[24]) {
	struct config_params *p = &hand->conf;
	int rest = hand->rest;
	int bars = hand->bars;
	int *f =   hand->f;

	for(int i = 0; i < bars; i++) {
		double point[4];
		point[0] = rest+(p->bw+p->bs)*i;
		point[1] = rest+(p->bw+p->bs)*i+p->bw;
		point[2] = (unsigned int)f[i]+p->shdw> (unsigned int)p->h-p->shdw ? (unsigned int)p->h-p->shdw : (unsigned int)f[i]+p->shdw;
		point[3] = p->shdw;
		
		glBegin(GL_QUADS);
			if(p->shdw) {
				// left side
				glColor4d(colors[5], colors[6], colors[7], colors[4]);
				glVertex3d(point[0], point[2], 1.0);
				glVertex3d(point[0], point[3], 1.0);
				glColor4d(0.0, 0.0, 0.0, 0.0);
				glVertex3d(point[0]-p->shdw/2, point[3]-p->shdw, 1.0);
				glVertex3d(point[0]-p->shdw/2, point[2]+p->shdw/2, 1.0);
				
				// right side
				glColor4d(colors[5], colors[6], colors[7], colors[4]);
				glVertex3d(point[1], point[2], 1.0);
				glVertex3d(point[1], point[3], 1.0);
				glColor4d(0.0, 0.0, 0.0, 0.0);
				glVertex3d(point[1]+p->shdw, point[3]-p->shdw, 0.9);
				glVertex3d(point[1]+p->shdw, point[2]+p->shdw/2, 0.9);
				
				// top side
				glColor4d(colors[5], colors[6], colors[7], colors[4]);
				glVertex3d(point[1], point[2], 1.0);
				glVertex3d(point[0], point[2], 1.0);
				glColor4d(0.0, 0.0, 0.0, 0.0);
				glVertex3d(point[0]-p->shdw/2, point[2]+p->shdw/2, 0.9);
				glVertex3d(point[1]+p->shdw, point[2]+p->shdw/2, 0.9);

				// bottom side
				glColor4d(colors[5], colors[6], colors[7], colors[4]);
				glVertex3d(point[1], point[3], 1.0);
				glVertex3d(point[0], point[3], 1.0);
				glColor4d(0.0, 0.0, 0.0, 0.0);
				glVertex3d(point[0]-p->shdw/2, point[3]-p->shdw, 0.9);
				glVertex3d(point[1]+p->shdw, point[3]-p->shdw, 0.9);
			}

			if(p->gradients) {
				double progress = (double)(point[2]-p->shdw)/(double)((unsigned int)p->h-p->shdw);
				int gcMax = ceil((p->gradients-1.0)*progress);
				double cutLenght = ((unsigned int)p->h-p->shdw)/(double)(p->gradients-1.0);
				for(int gcPhase=0; gcPhase<gcMax; gcPhase++) {
					if(gcPhase==gcMax-1) {
						double barProgress = fmod(point[2]-1.0-(double)p->shdw, cutLenght)/cutLenght;
						glColor4d(
							gradColors[gcPhase*3]+(gradColors[gcPhase*3+3]-gradColors[gcPhase*3])*barProgress, 
							gradColors[gcPhase*3+1]+(gradColors[gcPhase*3+4]-gradColors[gcPhase*3+1])*barProgress,
							gradColors[gcPhase*3+2]+(gradColors[gcPhase*3+5]-gradColors[gcPhase*3+2])*barProgress, colors[3]);
						glVertex3d(point[0], point[2], 1.0);
						glVertex3d(point[1], point[2], 1.0);
					} else {
						glColor4d(gradColors[gcPhase*3+3], gradColors[gcPhase*3+4],
							gradColors[gcPhase*3+5], colors[3]);
						glVertex3d(point[0], cutLenght*(gcPhase+1)+point[3], 1.0);
						glVertex3d(point[1], cutLenght*(gcPhase+1)+point[3], 1.0);
					}
					
					glColor4d(gradColors[gcPhase*3], gradColors[gcPhase*3+1], gradColors[gcPhase*3+2], colors[3]);
					glVertex3d(point[1], cutLenght*gcPhase+point[3], 1.0);
					glVertex3d(point[0], cutLenght*gcPhase+point[3], 1.0);
				}
			} else {
				glColor4d(colors[0],colors[1],colors[2],colors[3]);
				glVertex3d(point[0], point[2], 1.0);
				glVertex3d(point[1], point[2], 1.0);

				glColor4d(colors[0], colors[1], colors[2], colors[3]);
				glVertex3d(point[1], point[3], 1.0);
				glVertex3d(point[0], point[3], 1.0);
			}
		glEnd();
	}
	return 0;
}
#endif
