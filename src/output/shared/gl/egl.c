
#include "egl.h"

#include "output/shared/graphical.h"
#include "shared.h"

void EGLConfigLoad(XAVA *xava) { SGLConfigLoad(xava); }

EGLBoolean EGLCreateContext(XAVA *xava, struct _escontext *ESContext) {
  XAVA_CONFIG *conf = &xava->conf;
  EGLint numConfigs;
  EGLint majorVersion;
  EGLint minorVersion;
  EGLContext context;
  EGLSurface surface;
  EGLConfig config;
  EGLint fbAttribs[] = {EGL_SURFACE_TYPE,
                        EGL_WINDOW_BIT,
                        EGL_RENDERABLE_TYPE,
                        EGL_OPENGL_BIT | EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT,
                        EGL_DEPTH_SIZE,
                        8,
                        EGL_RED_SIZE,
                        8,
                        EGL_GREEN_SIZE,
                        8,
                        EGL_BLUE_SIZE,
                        8,
                        EGL_ALPHA_SIZE,
                        conf->flag.transparency ? 8 : 0,
                        EGL_NONE};
  EGLint contextAttribs[] = {EGL_CONTEXT_MAJOR_VERSION,
                             2,
                             EGL_CONTEXT_MINOR_VERSION,
                             1,
                             EGL_CONTEXT_OPENGL_PROFILE_MASK,
                             EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                             EGL_NONE};

  eglBindAPI(EGL_OPENGL_API);

  EGLDisplay display = eglGetDisplay(ESContext->native_display);
  if (display == EGL_NO_DISPLAY) {
    xavaError("No EGL display");
    return EGL_FALSE;
  }

  // Initialize EGL
  if (!eglInitialize(display, &majorVersion, &minorVersion)) {
    xavaError("eglInitialize failed");
    return EGL_FALSE;
  }

  xavaSpam("Initialized EGL API version: %d.%d", majorVersion, minorVersion);

  // Get configs
  if ((eglGetConfigs(display, NULL, 0, &numConfigs) != EGL_TRUE) ||
      (numConfigs == 0)) {
    xavaError("EGL was unable to find display configs");
    return EGL_FALSE;
  }

  // Choose config
  if ((eglChooseConfig(display, fbAttribs, &config, 1, &numConfigs) !=
       EGL_TRUE) ||
      (numConfigs != 1)) {
    xavaError("EGL was unable to choose a config");
    return EGL_FALSE;
  }

  // Create a surface
  surface =
      eglCreateWindowSurface(display, config, ESContext->native_window, NULL);
  if (surface == EGL_NO_SURFACE) {
    xavaError("EGL was unable to create a surface");
    return EGL_FALSE;
  }

  // Create a GL context
  context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
  if (context == EGL_NO_CONTEXT) {
    xavaError("EGL was unable to create a context");
    return EGL_FALSE;
  }

  // Make the context current
  if (!eglMakeCurrent(display, surface, surface, context)) {
    xavaError("EGL was not able to switch to to the current window");
    return EGL_FALSE;
  }

  ESContext->display = display;
  ESContext->surface = surface;
  ESContext->context = context;
  return EGL_TRUE;
}

void EGLInit(XAVA *xava) {
  glewInit();
  SGLInit(xava);
}

void EGLApply(XAVA *xava) { SGLApply(xava); }

XG_EVENT *EGLEvent(XAVA *xava) { return SGLEvent(xava); }

void EGLClear(XAVA *xava) { SGLClear(xava); }

void EGLDraw(XAVA *xava) { SGLDraw(xava); }

void EGLCleanup(XAVA *xava, struct _escontext *ESContext) {
  SGLCleanup(xava);
  eglDestroyContext(ESContext->display, ESContext->context);
  eglDestroySurface(ESContext->display, ESContext->surface);
  eglTerminate(ESContext->display);
}
