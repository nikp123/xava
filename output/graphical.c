#include "graphical.h"
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
int drawGLBars(int rest, int bw, int bs, int bars, int h, int shadow, int gradient, float colors[8], float gradColors[24], int *f) {
	for(int i = 0; i < bars; i++) {
		double point[4];
		point[0] = rest+(bw+bs)*i;
		point[1] = rest+(bw+bs)*i+bw;
		point[2] = f[i]+shadow > h-shadow ? h-shadow : f[i]+shadow;
		point[3] = shadow;
		
		glBegin(GL_QUADS);
			if(shadow) {
				// left side
				glColor4f(colors[5], colors[6], colors[7], colors[4]);
				glVertex2f(point[0], point[2]);
				glVertex2f(point[0], point[3]);
				glColor4f(0.0, 0.0, 0.0, 0.0);
				glVertex2f(point[0]-shadow/2, point[3]-shadow);
				glVertex2f(point[0]-shadow/2, point[2]+shadow/2);
				
				// right side
				glColor4f(colors[5], colors[6], colors[7], colors[4]);
				glVertex2f(point[1], point[2]);
				glVertex2f(point[1], point[3]);
				glColor4f(0.0, 0.0, 0.0, 0.0);
				glVertex2f(point[1]+shadow, point[3]-shadow);
				glVertex2f(point[1]+shadow, point[2]+shadow/2);
				
				// top side
				glColor4f(colors[5], colors[6], colors[7], colors[4]);
				glVertex2f(point[1], point[2]);
				glVertex2f(point[0], point[2]);
				glColor4f(0.0, 0.0, 0.0, 0.0);
				glVertex2f(point[0]-shadow/2, point[2]+shadow/2);
				glVertex2f(point[1]+shadow, point[2]+shadow/2);

				// bottom side
				glColor4f(colors[5], colors[6], colors[7], colors[4]);
				glVertex2f(point[1], point[3]);
				glVertex2f(point[0], point[3]);
				glColor4f(0.0, 0.0, 0.0, 0.0);
				glVertex2f(point[0]-shadow/2, point[3]-shadow);
				glVertex2f(point[1]+shadow, point[3]-shadow);
			}
		
			if(gradient) {
				float progress = (float)(point[2]-shadow)/(float)(h-shadow);
				int gcMax = ceil((gradient-1.0)*progress);
				float cutLenght = (h-shadow)/(float)(gradient-1.0);
				for(int gcPhase=0; gcPhase<gcMax; gcPhase++) {
					if(gcPhase==gcMax-1) {
						float barProgress = fmod(point[2]-1.0-(float)shadow, cutLenght)/cutLenght;
						glColor4f(
							gradColors[gcPhase*3]+(gradColors[gcPhase*3+3]-gradColors[gcPhase*3])*barProgress, 
							gradColors[gcPhase*3+1]+(gradColors[gcPhase*3+4]-gradColors[gcPhase*3+1])*barProgress,
							gradColors[gcPhase*3+2]+(gradColors[gcPhase*3+5]-gradColors[gcPhase*3+2])*barProgress, colors[3]);
						glVertex2f(point[0], point[2]);
						glVertex2f(point[1], point[2]);
					} else {
						glColor4f(gradColors[gcPhase*3+3], gradColors[gcPhase*3+4],
							gradColors[gcPhase*3+5], colors[3]);
						glVertex2f(point[0], cutLenght*(gcPhase+1)+point[3]);
						glVertex2f(point[1], cutLenght*(gcPhase+1)+point[3]);
					}
					
					glColor4f(gradColors[gcPhase*3], gradColors[gcPhase*3+1], gradColors[gcPhase*3+2], colors[3]);
					glVertex2f(point[1], cutLenght*gcPhase+point[3]);
					glVertex2f(point[0], cutLenght*gcPhase+point[3]);
				}
		} else {
				glColor4f(colors[0],colors[1],colors[2],colors[3]);
				glVertex2f(point[0], point[2]);
				glVertex2f(point[1], point[2]);
				
				glColor4f(colors[0], colors[1], colors[2], colors[3]);
				glVertex2f(point[1], point[3]);
				glVertex2f(point[0], point[3]);
			}
		glEnd();
	}
	return 0;
}
#endif
