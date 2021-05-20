#include <GLES2/gl2.h>
#include <wayland-egl-core.h>
#include <wayland-egl.h>
#include <EGL/egl.h>

// this is so that my static analyser doesn't bitch as much
#define EGL

#include "main.h"

void EGLCreateWindow(struct waydata *wd) {
	//region = wl_compositor_create_region(wd->compositor);
	//wl_region_add(region, 0, 0, width, height);
	//wl_surface_set_opaque_region(surface, region);
	
	struct wl_egl_window *egl_window = wl_egl_window_create(wd->surface,
			wd->hand->conf.w, wd->hand->conf.h);

	xavaBailCondition(egl_window == EGL_NO_SURFACE,
			"Failed to create EGL window!\n");

	xavaSpam("Created EGL window!");

	wd->ESContext.native_window = egl_window;
	wd->ESContext.window_width = wd->hand->conf.w;
	wd->ESContext.window_height = wd->hand->conf.h;
}

EGLBoolean EGLCreateContext(struct waydata *wd) {
	EGLint numConfigs;
	EGLint majorVersion;
	EGLint minorVersion;
	EGLContext context;
	EGLSurface surface;
	EGLConfig config;
	EGLint fbAttribs[] =
	{
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE,        8,
		EGL_GREEN_SIZE,      8,
		EGL_BLUE_SIZE,       8,
		EGL_NONE
	};
	EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
	EGLDisplay display = eglGetDisplay(wd->display);
	if ( display == EGL_NO_DISPLAY )
	{
		xavaError("No EGL display");
		return EGL_FALSE;
	}

	// Initialize EGL
	if ( !eglInitialize(display, &majorVersion, &minorVersion) )
	{
		xavaError("eglInitialize failed");
		return EGL_FALSE;
	}

	eglBindAPI(EGL_OPENGL_ES_API);

	// Get configs
	if ( (eglGetConfigs(display, NULL, 0, &numConfigs) != EGL_TRUE) || (numConfigs == 0))
	{
		xavaError("EGL was unable to find display configs");
		return EGL_FALSE;
	}

	// Choose config
	if ( (eglChooseConfig(display, fbAttribs, &config, 1, &numConfigs) != EGL_TRUE) || (numConfigs != 1))
	{
		xavaError("EGL was unable to choose a config");
		return EGL_FALSE;
	}

	// Create a surface
	surface = eglCreateWindowSurface(display, config, wd->ESContext.native_window, NULL);
	if ( surface == EGL_NO_SURFACE )
	{
		xavaError("EGL was unable to create a surface");
		return EGL_FALSE;
	}

	// Create a GL context
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs );
	if ( context == EGL_NO_CONTEXT )
	{
		xavaError("EGL was unable to create a context");
		return EGL_FALSE;
	}

	// Make the context current
	if ( !eglMakeCurrent(display, surface, surface, context) )
	{
		xavaError("EGL was unable to switch to the wayland window");
		return EGL_FALSE;
	}

	wd->ESContext.display = display;
	wd->ESContext.surface = surface;
	wd->ESContext.context = context;
	return EGL_TRUE;

}

EGLint waylandEGLShaderBuild(struct waydata *wd, const char *source, GLenum shader_type) {
	EGLint shader;
	EGLint status;

	shader = glCreateShader(shader_type);
	xavaBailCondition(shader == 0, "Failed to build shader");

	glShaderSource(shader, 1, (const char **) &source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetShaderInfoLog(shader, 1000, &len, log);
		xavaBail("Error: compiling %s: %*s\n",
				shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment",
				len, log);
	}

	xavaSpam("Compiling %s shader successful",
			shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment");

	return shader;
}

void waylandEGLCreate(struct waydata *wd) {
	EGLCreateWindow(wd);
	xavaBailCondition(EGLCreateContext(wd) == EGL_FALSE,
			"Failed to create EGL context");
}

void waylandEGLDestroy(struct waydata *wd) {
	eglDestroySurface(wd->ESContext.display, wd->ESContext.surface);
	wl_egl_window_destroy(wd->ESContext.native_window);
}

