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

#include "egl.h"
#include "main.h"
#include "render.h"

static GLfloat *vertexData;
static GLfloat projectionMatrix[16] = 
	{2.0, 0.0, 0.0, -1.0,
	0.0, 2.0, 0.0, -1.0,
	0.0, 0.0, -1.0, 0.0,
	0.0, 0.0, 0.0, 1.0};

static GLuint GL_POS;
static GLuint GL_FGCOL;
static GLuint GL_W, GL_H;
static GLuint GL_PROJMATRIX;

// stupidly unsafe function, now behave yourselves
raw_data *load_file(const char *file) {
	raw_data *data = malloc(sizeof(raw_data));

	FILE *fp = fopen(file, "r");

	fseek(fp, 0, SEEK_END);
	data->size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	data->data = malloc(data->size+1);
	fread(data->data, data->size, 1, fp);

	// pro-gamer move (NOTE: might not work if char is bigger than 8 bits)
	((char*)data->data)[data->size] = 0x00;

	fclose(fp);

	return data;
}

void close_file(raw_data *file) {
	free(file->data);
	free(file);
}

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

EGLint waylandEGLShaderBuild(const char *source, GLenum shader_type) {
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

void waylandEGLDestroy(struct waydata *wd) {
	eglDestroySurface(wd->ESContext.display, wd->ESContext.surface);
	wl_egl_window_destroy(wd->ESContext.native_window);
}

void waylandEGLShadersLoad(struct waydata *wd) {
	char *fragmentShaderPath, *vertexShaderPath;
	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG, "egl/shaders/fragment.glsl", &fragmentShaderPath) == false, 
			"Failed to load fragment shader!");
	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG, "egl/shaders/vertex.glsl", &vertexShaderPath) == false,
			"Failed to load vertex shader!");

	wd->fragSHD = load_file(fragmentShaderPath);
	wd->vertSHD = load_file(vertexShaderPath);

	free(vertexShaderPath);
	free(fragmentShaderPath);
}

void waylandEGLInit(struct waydata *wd) {
	// creates everything EGL related
	EGLCreateWindow(wd);
	xavaBailCondition(EGLCreateContext(wd) == EGL_FALSE,
			"Failed to create EGL context");

	EGLint fragment, vertex, program, status;

	program = glCreateProgram();
	fragment = waylandEGLShaderBuild(wd->fragSHD->data, GL_FRAGMENT_SHADER); 
	vertex   = waylandEGLShaderBuild(wd->vertSHD->data, GL_VERTEX_SHADER);
	close_file(wd->fragSHD);
	close_file(wd->vertSHD);

	glAttachShader(program, fragment);
	glAttachShader(program, vertex);
	glLinkProgram(program);
	glUseProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetProgramInfoLog(program, 1000, &len, log);
		fprintf(stderr, "Error: linking:\n%*s\n", len, log);
		exit(1);
	}

	GL_POS        = glGetAttribLocation(program,  "pos");
	GL_FGCOL      = glGetUniformLocation(program, "color");
	GL_W          = glGetUniformLocation(program, "width");
	GL_H          = glGetUniformLocation(program, "height");
	GL_PROJMATRIX = glGetUniformLocation(program, "projectionMatrix");

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// we just need working pointers so that realloc() works
	vertexData = malloc(1);
}

void waylandEGLApply(struct XAVA_HANDLE *xava) {
	struct config_params *conf = &xava->conf;

	// define viewport size
	glViewport(0, 0, conf->w, conf->h);

	// reallocate and attach verticies data
	vertexData = realloc(vertexData, sizeof(GLfloat)*xava->bars*12);
	glVertexAttribPointer(GL_POS, 2, GL_FLOAT, GL_FALSE, 0, vertexData);

	// update screen width and height uniforms
	glUniform1f(GL_W, xava->conf.w);
	glUniform1f(GL_H, xava->conf.h);

	// update projection matrix
	projectionMatrix[0] = 2.0/xava->conf.w;
	projectionMatrix[5] = 2.0/xava->conf.h;

	glUniformMatrix4fv(GL_PROJMATRIX, 1, GL_FALSE, (GLfloat*) projectionMatrix);

	// set and attach foreground color
	uint32_t fgcol = wayland_color_blend(conf->col, conf->foreground_opacity*255);
	glUniform4f(GL_FGCOL, ARGB_R_32(fgcol)/255.0, ARGB_G_32(fgcol)/255.0,
			ARGB_B_32(fgcol)/255.0, conf->foreground_opacity);

	// set background clear color
	uint32_t bgcol = wayland_color_blend(conf->bgcol, conf->background_opacity*255);
	glClearColor(ARGB_R_32(bgcol)/255.0, ARGB_G_32(bgcol)/255.0,
			ARGB_B_32(bgcol)/255.0, conf->background_opacity);
}

void waylandEGLDraw(struct XAVA_HANDLE *xava) {
	struct config_params *conf = &xava->conf;

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

	glEnableVertexAttribArray(GL_POS);

	// You use the number of verticies, but not the actual polygon count
	glDrawArrays(GL_TRIANGLES, 0, xava->bars*6);

	glDisableVertexAttribArray(GL_POS);
}
