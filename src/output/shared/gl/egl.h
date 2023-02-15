#ifndef __EGL_H
#define __EGL_H

#ifndef EGL
    #define EGL
#endif
#include "main.h"

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

void           EGLConfigLoad(XAVA *xava);
EGLBoolean     EGLCreateContext(XAVA *xava, struct _escontext *ESContext);
void           EGLInit(XAVA *xava);
void           EGLApply(XAVA *xava);
XG_EVENT_STACK *EGLEvent(XAVA *xava);
void           EGLClear(XAVA *xava);
void           EGLDraw(XAVA *xava);
void           EGLCleanup(XAVA *xava, struct _escontext *ESContext);
#endif

