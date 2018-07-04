#if defined(WIN)||defined(GLX)
	#define GL
#endif

#ifndef H_GRAPHICAL
#define H_GRAPHICAL

#ifdef GLX
	#include <GL/glx.h>
	GLXContext cavaGLXContext;
	GLXFBConfig* cavaFBConfig;
	extern int GLXmode;
#endif

#ifdef GL
	#include <GL/gl.h>
#endif

int windowX, windowY;
unsigned char fs, borderFlag, transparentFlag, keepInBottom, interactable;
char *windowAlignment;
#endif
