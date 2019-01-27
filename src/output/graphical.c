#include "graphical.h"
#include "../config.h"
#include <string.h>
#include <math.h>

void calculate_win_pos(int *winX, int *winY, int winW, int winH, int scrW, int scrH, char *winPos) {
	if(!strcmp(winPos, "top")){
		(*winX) = (scrW - winW) / 2 + (*winX);
	}else if(!strcmp(winPos, "bottom")){
		(*winX) = (scrW - winW) / 2 + (*winX);
		(*winY) = (scrH - winH) + (-1*(*winY));
	}else if(!strcmp(winPos, "top_right")){
		(*winX) = (scrW - winW) + (-1*(*winX));
	}else if(!strcmp(winPos, "left")){
		(*winY) = (scrH - winH) / 2;
	}else if(!strcmp(winPos, "right")){
		(*winX) = (scrW - winW) + (-1*(*winX));
		(*winY) = (scrH - winH) / 2 + (*winY);
	}else if(!strcmp(winPos, "bottom_left")){
		(*winY) = (scrH - winH) + (-1*(*winY));
	}else if(!strcmp(winPos, "bottom_right")){
		(*winX) = (scrW - winW) + (-1*(*winX));
		(*winY) = (scrH - winH) + (-1*(*winY));
	}else if(!strcmp(winPos, "center")){
		(*winX) = (scrW - winW) / 2 + (*winX);
		(*winY) = (scrH - winH) / 2 + (*winY);
	}
	// Some error checking
	#ifdef DEBUG
		if(winX > scrW - winW) printf("Warning: Screen out of bounds (X axis)!");
		if(winY > scrH - winH) printf("Warning: Screen out of bounds (Y axis)!");
	#endif
}

#ifdef GL
int drawGLBars(int rest, int bars, float gradColors[24], int *f) {
	const double colors[8] = {((p.col>>24)%0x100)/255.0, ((p.col>>8)%0x100)/255.0, (p.col%0x100)/255.0, (float)p.foreground_opacity,
							 (p.shdw_col>>24)%256/255.0, (p.shdw_col>>16)%256/255.0, (p.shdw_col>>8)%256/255.0, p.shdw_col%256/255.0};
	for(int i = 0; i < bars; i++) {
		double point[4];
		point[0] = rest+(p.bw+p.bs)*i;
		point[1] = rest+(p.bw+p.bs)*i+p.bw;
		point[2] = (unsigned int)f[i]+p.shdw> (unsigned int)p.h-p.shdw ? (unsigned int)p.h-p.shdw : (unsigned int)f[i]+p.shdw;
		point[3] = p.shdw;
		
		glBegin(GL_QUADS);
			if(p.shdw) {
				// left side
				glColor4d(colors[5], colors[6], colors[7], colors[4]);
				glVertex2d(point[0], point[2]);
				glVertex2d(point[0], point[3]);
				glColor4d(0.0, 0.0, 0.0, 0.0);
				glVertex2d(point[0]-p.shdw/2, point[3]-p.shdw);
				glVertex2d(point[0]-p.shdw/2, point[2]+p.shdw/2);
				
				// right side
				glColor4d(colors[5], colors[6], colors[7], colors[4]);
				glVertex2d(point[1], point[2]);
				glVertex2d(point[1], point[3]);
				glColor4d(0.0, 0.0, 0.0, 0.0);
				glVertex2d(point[1]+p.shdw, point[3]-p.shdw);
				glVertex2d(point[1]+p.shdw, point[2]+p.shdw/2);
				
				// top side
				glColor4d(colors[5], colors[6], colors[7], colors[4]);
				glVertex2d(point[1], point[2]);
				glVertex2d(point[0], point[2]);
				glColor4d(0.0, 0.0, 0.0, 0.0);
				glVertex2d(point[0]-p.shdw/2, point[2]+p.shdw/2);
				glVertex2d(point[1]+p.shdw, point[2]+p.shdw/2);

				// bottom side
				glColor4d(colors[5], colors[6], colors[7], colors[4]);
				glVertex2d(point[1], point[3]);
				glVertex2d(point[0], point[3]);
				glColor4d(0.0, 0.0, 0.0, 0.0);
				glVertex2d(point[0]-p.shdw/2, point[3]-p.shdw);
				glVertex2d(point[1]+p.shdw, point[3]-p.shdw);
			}
		
			if(p.gradient) {
				double progress = (double)(point[2]-p.shdw)/(double)((unsigned int)p.h-p.shdw);
				int gcMax = ceil((p.gradient_count-1.0)*progress);
				double cutLenght = ((unsigned int)p.h-p.shdw)/(double)(p.gradient_count-1.0);
				for(int gcPhase=0; gcPhase<gcMax; gcPhase++) {
					if(gcPhase==gcMax-1) {
						double barProgress = fmod(point[2]-1.0-(double)p.shdw, cutLenght)/cutLenght;
						glColor4d(
							gradColors[gcPhase*3]+(gradColors[gcPhase*3+3]-gradColors[gcPhase*3])*barProgress, 
							gradColors[gcPhase*3+1]+(gradColors[gcPhase*3+4]-gradColors[gcPhase*3+1])*barProgress,
							gradColors[gcPhase*3+2]+(gradColors[gcPhase*3+5]-gradColors[gcPhase*3+2])*barProgress, colors[3]);
						glVertex2d(point[0], point[2]);
						glVertex2d(point[1], point[2]);
					} else {
						glColor4d(gradColors[gcPhase*3+3], gradColors[gcPhase*3+4],
							gradColors[gcPhase*3+5], colors[3]);
						glVertex2d(point[0], cutLenght*(gcPhase+1)+point[3]);
						glVertex2d(point[1], cutLenght*(gcPhase+1)+point[3]);
					}
					
					glColor4d(gradColors[gcPhase*3], gradColors[gcPhase*3+1], gradColors[gcPhase*3+2], colors[3]);
					glVertex2d(point[1], cutLenght*gcPhase+point[3]);
					glVertex2d(point[0], cutLenght*gcPhase+point[3]);
				}
		} else {
				glColor4d(colors[0],colors[1],colors[2],colors[3]);
				glVertex2d(point[0], point[2]);
				glVertex2d(point[1], point[2]);
				
				glColor4d(colors[0], colors[1], colors[2], colors[3]);
				glVertex2d(point[1], point[3]);
				glVertex2d(point[0], point[3]);
			}
		glEnd();
	}
	return 0;
}
#endif
