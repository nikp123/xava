#ifndef __EGL_H
#define __EGL_H

#ifndef EGL
	#define EGL
#endif
#include "gl_shared.h"

struct _escontext {
	// because windowing systems are complicated
	EGLNativeDisplayType native_display;
	EGLNativeWindowType native_window;

	// EGL display
	EGLDisplay  display;
	// EGL context
	EGLContext  context;
	// EGL surface
	EGLSurface  surface;
};

void       EGLConfigLoad(struct XAVA_HANDLE *xava);
EGLBoolean EGLCreateContext(struct XAVA_HANDLE *xava, struct _escontext *ESContext);
void       EGLInit(struct XAVA_HANDLE *xava);
void       EGLApply(struct XAVA_HANDLE *xava);
XG_EVENT   EGLEvent(struct XAVA_HANDLE *xava);
void       EGLClear(struct XAVA_HANDLE *xava);
void       EGLDraw(struct XAVA_HANDLE *xava);
void       EGLCleanup(struct XAVA_HANDLE *xava, struct _escontext *ESContext);
#endif

