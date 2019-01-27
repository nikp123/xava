#ifndef H_GRAPHICAL
#define H_GRAPHICAL

#ifdef GLX
	#include <GL/glx.h>
	GLXContext xavaGLXContext;
	GLXFBConfig* xavaFBConfig;
	extern int GLXmode;
#endif

#ifdef GL
	#include <GL/gl.h>
	int drawGLBars(int rest, int bars, float gradColors[24], int *f);
#endif

void calculate_win_pos(int *winX, int *winY, int winW, int winH, int scrW, int scrH, char *winPos);

static unsigned int definedColors[] = {0x000000, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF};

int windowX, windowY;
unsigned char fs, borderFlag, transparentFlag, keepInBottom, interactable;
char *windowAlignment;
#endif
