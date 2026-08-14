#ifndef __EGL_H
#define __EGL_H

#ifndef EGL
#define EGL
#endif
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "main.h"

struct _escontext {
  // because windowing systems are complicated
  EGLNativeDisplayType native_display;
  EGLNativeWindowType native_window;
  // EGL platform (EGL_PLATFORM_*_EXT) that native_display/native_window belong to
  EGLenum platform;

  // EGL display
  EGLDisplay display;
  // EGL context
  EGLContext context;
  // EGL surface
  EGLSurface surface;
};

void EGLConfigLoad(XAVA *xava);
EGLBoolean EGLCreateContext(XAVA *xava, struct _escontext *ESContext);
void EGLInit(XAVA *xava);
void EGLApply(XAVA *xava);
XG_EVENT *EGLEvent(XAVA *xava);
void EGLClear(XAVA *xava);
void EGLDraw(XAVA *xava);
void EGLCleanup(XAVA *xava, struct _escontext *ESContext);
#endif
