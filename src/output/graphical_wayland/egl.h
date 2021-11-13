#ifndef __WAYLAND_EGL_H
#define __WAYLAND_EGL_H
	#include <stdlib.h>
	#include <wayland-egl-core.h>
	#include <EGL/egl.h>
	#include "main.h"

	void     waylandEGLDestroy(struct waydata *wd);
	void     waylandEGLInit(struct waydata *wd);
	void     waylandEGLApply(struct XAVA_HANDLE *xava);
	XG_EVENT waylandEGLEvent(struct waydata *wd);
	void     waylandEGLDraw(struct XAVA_HANDLE *xava);
	void     waylandEGLConfigLoad(struct XAVA_HANDLE *xava);
	void     waylandEGLWindowResize(struct waydata *wd, int w, int h);
#endif
