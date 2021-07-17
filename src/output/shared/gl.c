#include <math.h>

#include <GL/gl.h>

#include "../../shared.h"
#include "../graphical.h"

static double colors[8];
static double gradColors[24];
unsigned int *gradientColor;

void GLInit(struct XAVA_HANDLE *xava) {
	struct config_params *conf = &xava->conf;

	// parse all of the values
	gradientColor = malloc(sizeof(int)*conf->gradients);
	for(int i=0; i<conf->gradients; i++)
		sscanf(conf->gradient_colors[i], "#%06x", &gradientColor[i]);

	glEnable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	if(conf->transF) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	return;
}

void GLApply(struct XAVA_HANDLE *xava) {
	struct config_params *conf = &xava->conf;

	glViewport(0, 0, conf->w, conf->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, conf->w, 0, conf->h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void GLClear(struct XAVA_HANDLE *xava) {
	struct config_params *conf = &xava->conf;

	colors[0] = ARGB_R_32(conf->col)/255.0;
	colors[1] = ARGB_G_32(conf->col)/255.0;
	colors[2] = ARGB_B_32(conf->col)/255.0;
	colors[3] = conf->transF ? conf->foreground_opacity : 1.0;
	colors[4] = ARGB_A_32(conf->shdw_col)/255.0;
	colors[5] = ARGB_R_32(conf->shdw_col)/255.0;
	colors[6] = ARGB_G_32(conf->shdw_col)/255.0;
	colors[7] = ARGB_B_32(conf->shdw_col)/255.0;

	for(int i=0; i<conf->gradients; i++) {
		gradColors[i*3] = ARGB_R_32(gradientColor[i])/255.0;
		gradColors[i*3+1] = ARGB_G_32(gradientColor[i])/255.0;
		gradColors[i*3+2] = ARGB_B_32(gradientColor[i])/255.0;;
	}

	glClearColor(ARGB_R_32(conf->bgcol)/255.0, ARGB_G_32(conf->bgcol)/255.0, ARGB_B_32(conf->bgcol)/255.0, 0.0);
}

void GLDraw(struct XAVA_HANDLE *xava) {
	struct config_params *conf = &xava->conf;
	int rest = xava->rest;
	int bars = xava->bars;
	int *f =   xava->f;
	
	// clear color and calculate pixel witdh in double
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for(int i = 0; i < bars; i++) {
		double point[4];
		point[0] = rest+(conf->bw+conf->bs)*i;
		point[1] = rest+(conf->bw+conf->bs)*i+conf->bw;
		point[2] = (unsigned int)f[i]+conf->shdw> (unsigned int)conf->h-conf->shdw ? (unsigned int)conf->h-conf->shdw : (unsigned int)f[i]+conf->shdw;
		point[3] = conf->shdw;
		
		glBegin(GL_QUADS);
			if(conf->shdw) {
				// left side
				glColor4d(colors[5], colors[6], colors[7], colors[4]);
				glVertex3d(point[0], point[2], 1.0);
				glVertex3d(point[0], point[3], 1.0);
				glColor4d(0.0, 0.0, 0.0, 0.0);
				glVertex3d(point[0]-conf->shdw/2, point[3]-conf->shdw, 1.0);
				glVertex3d(point[0]-conf->shdw/2, point[2]+conf->shdw/2, 1.0);
				
				// right side
				glColor4d(colors[5], colors[6], colors[7], colors[4]);
				glVertex3d(point[1], point[2], 1.0);
				glVertex3d(point[1], point[3], 1.0);
				glColor4d(0.0, 0.0, 0.0, 0.0);
				glVertex3d(point[1]+conf->shdw, point[3]-conf->shdw, 0.9);
				glVertex3d(point[1]+conf->shdw, point[2]+conf->shdw/2, 0.9);
				
				// top side
				glColor4d(colors[5], colors[6], colors[7], colors[4]);
				glVertex3d(point[1], point[2], 1.0);
				glVertex3d(point[0], point[2], 1.0);
				glColor4d(0.0, 0.0, 0.0, 0.0);
				glVertex3d(point[0]-conf->shdw/2, point[2]+conf->shdw/2, 0.9);
				glVertex3d(point[1]+conf->shdw, point[2]+conf->shdw/2, 0.9);

				// bottom side
				glColor4d(colors[5], colors[6], colors[7], colors[4]);
				glVertex3d(point[1], point[3], 1.0);
				glVertex3d(point[0], point[3], 1.0);
				glColor4d(0.0, 0.0, 0.0, 0.0);
				glVertex3d(point[0]-conf->shdw/2, point[3]-conf->shdw, 0.9);
				glVertex3d(point[1]+conf->shdw, point[3]-conf->shdw, 0.9);
			}

			if(conf->gradients) {
				double progress = (double)(point[2]-conf->shdw)/(double)((unsigned int)conf->h-conf->shdw);
				int gcMax = ceil((conf->gradients-1.0)*progress);
				double cutLenght = ((unsigned int)conf->h-conf->shdw)/(double)(conf->gradients-1.0);
				for(int gcPhase=0; gcPhase<gcMax; gcPhase++) {
					if(gcPhase==gcMax-1) {
						double barProgress = fmod(point[2]-1.0-(double)conf->shdw, cutLenght)/cutLenght;
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

	glFinish();
}

void GLCleanup() {
	free(gradientColor);
}

