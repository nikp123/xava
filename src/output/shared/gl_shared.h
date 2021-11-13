#ifndef __GL_SHARED_H
#define __GL_SHARED_H

// for static checker sake
#if !defined(GL) && !defined(EGL)
    #define GL
    #warning "This WILL break your build. Fix it!"
#endif

#if defined(EGL)
    #include <EGL/eglplatform.h>
    #include <EGL/egl.h>
#endif

#ifndef GL_ALREADY_DEFINED
    #include <GL/glew.h>
#endif

#include "../../shared.h"

void     SGLConfigLoad(struct XAVA_HANDLE *xava);
void     SGLInit(struct XAVA_HANDLE *xava);
void     SGLApply(struct XAVA_HANDLE *xava);
XG_EVENT SGLEvent(struct XAVA_HANDLE *xava);
void     SGLClear(struct XAVA_HANDLE *xava);
void     SGLDraw(struct XAVA_HANDLE *xava);
void     SGLCleanup(struct XAVA_HANDLE *xava);

#endif

