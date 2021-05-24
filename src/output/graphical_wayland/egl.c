#include <GLES2/gl2.h>
#include <stdint.h>
#include <wayland-egl-core.h>
#include <wayland-egl.h>
#include <EGL/egl.h>

// this is so that my static analyser doesn't bitch as much
#ifndef EGL
	#define EGL
#endif

#include "../graphical.h"
#include "../shared/egl.h"

#include "egl.h"
#include "main.h"
#include "render.h"

static struct EGLprogram pre, post;
static struct FBO        FBO;

static GLfloat *vertexData;
static GLfloat projectionMatrix[16] = 
	{2.0, 0.0, 0.0, -1.0,
	0.0, 2.0, 0.0, -1.0,
	0.0, 0.0, -1.0, 0.0,
	0.0, 0.0, 0.0, 1.0};

static GLuint PRE_POS;
static GLuint PRE_FGCOL;
static GLuint PRE_W, PRE_H;
static GLuint PRE_PROJMATRIX;

static GLuint POST_POS;
static GLuint POST_TEXCOORD;
static GLuint POST_TEXTURE;

void waylandEGLCreateWindow(struct waydata *wd) {
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

EGLBoolean waylandEGLCreateContext(struct waydata *wd) {
	struct XAVA_HANDLE *xava = wd->hand;
	struct config_params *conf = &xava->conf;
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
		EGL_ALPHA_SIZE,      conf->transF ? 8 : 0,
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

void waylandEGLDestroy(struct waydata *wd) {
	eglDestroySurface(wd->ESContext.display, wd->ESContext.surface);
	wl_egl_window_destroy(wd->ESContext.native_window);
}


void waylandEGLInit(struct waydata *wd) {
	// creates everything EGL related
	waylandEGLCreateWindow(wd);
	xavaBailCondition(waylandEGLCreateContext(wd) == EGL_FALSE,
			"Failed to create EGL context");

	EGLCreateProgram(&pre);
	EGLCreateProgram(&post);

	PRE_POS        = glGetAttribLocation(pre.program,  "pos");
	PRE_FGCOL      = glGetUniformLocation(pre.program, "color");
	PRE_W          = glGetUniformLocation(pre.program, "width");
	PRE_H          = glGetUniformLocation(pre.program, "height");
	PRE_PROJMATRIX = glGetUniformLocation(pre.program, "projectionMatrix");

	POST_POS       = glGetAttribLocation(post.program, "a_position");
	POST_TEXCOORD  = glGetAttribLocation(post.program, "a_texCoord");
	POST_TEXTURE   = glGetUniformLocation(post.program, "s_texture");
	glUseProgram(pre.program);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// we just need working pointers so that realloc() works
	vertexData = malloc(1);
}

void waylandEGLApply(struct XAVA_HANDLE *xava) {
	struct config_params *conf = &xava->conf;

	glUseProgram(post.program);

	// allocate three buffers
	glGenFramebuffers(1,  &FBO.framebuffer);
	glGenTextures(1,      &FBO.texture);

	// set texture properties
	glBindTexture(GL_TEXTURE_2D, FBO.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, conf->w, conf->h,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// set framebuffer properties
	glBindFramebuffer(GL_FRAMEBUFFER, FBO.framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, FBO.texture, 0);

	// check if it borked
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	xavaBailCondition(status != GL_FRAMEBUFFER_COMPLETE, 
			"Framebuffer Objects are not supported! Error code 0x%X\n",
			status);

	glUseProgram(pre.program);

	// reallocate and attach verticies data
	vertexData = realloc(vertexData, sizeof(GLfloat)*xava->bars*12);
	glVertexAttribPointer(PRE_POS, 2, GL_FLOAT, GL_FALSE, 0, vertexData);

	// update screen width and height uniforms
	glUniform1f(PRE_W, xava->conf.w);
	glUniform1f(PRE_H, xava->conf.h);

	// update projection matrix
	projectionMatrix[0] = 2.0/xava->conf.w;
	projectionMatrix[5] = 2.0/xava->conf.h;

	glUniformMatrix4fv(PRE_PROJMATRIX, 1, GL_FALSE, (GLfloat*) projectionMatrix);

	// set and attach foreground color
	uint32_t fgcol = wayland_color_blend(conf->col, conf->foreground_opacity*255);
	glUniform4f(PRE_FGCOL, ARGB_R_32(fgcol)/255.0, ARGB_G_32(fgcol)/255.0,
			ARGB_B_32(fgcol)/255.0, conf->foreground_opacity);

	// set background clear color
	uint32_t bgcol = wayland_color_blend(conf->bgcol, conf->background_opacity*255);
	glClearColor(ARGB_R_32(bgcol)/255.0, ARGB_G_32(bgcol)/255.0,
			ARGB_B_32(bgcol)/255.0, conf->background_opacity);
}

// dummy abstraction function
void waylandEGLShadersLoad(void) {
	EGLShadersLoad(&pre, &post);
}

void waylandEGLDraw(struct XAVA_HANDLE *xava) {
	struct config_params *conf = &xava->conf;

	/**
	 * Here we start rendering to the texture
	 * */

	// bind render target to texture
	glBindFramebuffer(GL_FRAMEBUFFER, FBO.framebuffer);
	glViewport(0, 0, conf->w, conf->h);

	// switch to pre shaders
	glUseProgram(pre.program);

	// do all of the vertex math needed
	for(int i=0; i<xava->bars; i++) {
		vertexData[i*12]    = xava->rest + i*(conf->bs+conf->bw);  
		vertexData[i*12+1]  = xava->f[i];
		vertexData[i*12+2]  = vertexData[i*12];  
		vertexData[i*12+3]  = 0.0;
		vertexData[i*12+4]  = vertexData[i*12]+conf->bw;
		vertexData[i*12+5]  = 0.0; 
		vertexData[i*12+6]  = vertexData[i*12+4];
		vertexData[i*12+7]  = vertexData[i*12+1];
		vertexData[i*12+8]  = vertexData[i*12+4];
		vertexData[i*12+9]  = 0.0;
		vertexData[i*12+10] = vertexData[i*12];
		vertexData[i*12+11] = vertexData[i*12+1]; 
	}

	glClear(GL_COLOR_BUFFER_BIT);

	// for some fucking reason this pointer gets reset after a context switch
	// this is to prevent the thing from killing itself when using glUseProgram()
	glVertexAttribPointer(PRE_POS, 2, GL_FLOAT, GL_FALSE, 0, vertexData);

	glEnableVertexAttribArray(PRE_POS);

	// You use the number of verticies, but not the actual polygon count
	glDrawArrays(GL_TRIANGLES, 0, xava->bars*6);

	glDisableVertexAttribArray(PRE_POS);

	/**
	 * Once the texture has been conpleted, we now activate a seperate pipeline
	 * which just displays that texture to the actual framebuffer for easier
	 * shader writing
	 * */

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, conf->w, conf->h);

	// Switch to post shaders
	glUseProgram(post.program);

	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLfloat vVertices[] = { 
		-1.0f, -1.0f, // Position 0
		0.0f, 0.0f,   // TexCoord 0 
		1.0f, -1.0f,  // Position 1
		1.0f, 0.0f,   // TexCoord 1
		1.0f, 1.0f,   // Position 2
		1.0f, 1.0f,   // TexCoord 2
		-1.0f, 1.0f,  // Position 3
		0.0f, 1.0f    // TexCoord 3
	};

	// Load the vertex position
	glVertexAttribPointer(POST_POS, 2, GL_FLOAT,
		GL_FALSE, 4 * sizeof(GLfloat), vVertices);
	// Load the texture coordinate
	glVertexAttribPointer(POST_TEXCOORD, 2, GL_FLOAT,
		GL_FALSE, 4 * sizeof(GLfloat), &vVertices[2]);

	glEnableVertexAttribArray(POST_POS);
	glEnableVertexAttribArray(POST_TEXCOORD);

	// Bind the texture
	// Set the sampler texture unit to 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, FBO.texture);
	glUniform1i(POST_TEXTURE, 0);

	// This looks like every frame.
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}
