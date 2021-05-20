#ifndef __WAYLAND_EGL_H
#define __WAYLAND_EGL_H
	#include <wayland-egl-core.h>
	#include <EGL/egl.h>

	void waylandEGLCreate(struct waydata *wd);
	void waylandEGLDestroy(struct waydata *wd);
	EGLint waylandEGLShaderBuild(struct waydata *wd, const char *source,
			EGLenum shader_type);
#endif
