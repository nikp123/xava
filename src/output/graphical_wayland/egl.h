#ifndef __WAYLAND_EGL_H
#define __WAYLAND_EGL_H
	#include <stdlib.h>
	#include <wayland-egl-core.h>
	#include <EGL/egl.h>
	#include "main.h"

	/**
	 * Unsafe function, as it assumes that the file is valid in front of time!
	 * */
	raw_data *load_file(const char *file);
	void      close_file(raw_data *file);

	void   waylandEGLCreate(struct waydata *wd);
	void   waylandEGLDestroy(struct waydata *wd);
	EGLint waylandEGLShaderBuild(const char *source, EGLenum shader_type);
	void   waylandEGLShadersLoad(struct waydata *wd);
#endif
