#include <stdio.h>
#include <string.h>
#include <math.h>

#include "gl_shared.h"

#include "../../shared.h"
#include "../graphical.h"

// we don't really need this struct, but it's nice to have (for extensibility)
struct SGLprogram {
	struct shader {
		char *path, *text;
		GLuint handle;
		XAVAIONOTIFYWATCH watch;
	} frag, vert, geo;
	GLuint program;
} pre, post;

// shader buffers
static GLfloat *vertexData;
static GLfloat *gradientColor;

// geometry information
static GLuint SHADER_REST;
static GLuint SHADER_BAR_WIDTH;
static GLuint SHADER_BAR_SPACING;
static GLuint SHADER_BAR_COUNT;
static GLuint SHADER_BARS;
static GLuint SHADER_RESOLUTION;

// color information
static GLuint SHADER_FGCOL;
static GLuint SHADER_BGCOL;

// system information providers
static GLuint SHADER_GRAD_SECT_COUNT;
static GLuint SHADER_GRADIENTS;
static GLuint SHADER_TIME;
static GLuint SHADER_INTENSITY;

// special
static GLfloat resScale;

// this is hacked in, shut up
static bool shouldRestart;

static void ionotify_callback(struct XAVA_HANDLE *hand, int id, XAVA_IONOTIFY_EVENT event) {
	switch(event) {
		case XAVA_IONOTIFY_CHANGED:
		case XAVA_IONOTIFY_DELETED:
			shouldRestart = true;
			break;
		default:
			// noop
			break;
	}
}

enum sgl_shader_type {
	SGL_PRE,
	SGL_POST
};

enum sgl_shader_stage {
	SGL_VERT,
	SGL_GEO,
	SGL_FRAG
};

void internal_SGLLoadShader(struct SGLprogram *program,
		enum sgl_shader_type type,
		enum sgl_shader_stage stage,
		const char *name,
		struct XAVA_HANDLE *xava) {
	RawData *file;
	char *returned_path;
	char file_path[MAX_PATH];
	XAVAIONOTIFYWATCHSETUP a;
	MALLOC_SELF(a, 1);

	switch(type) {
		case SGL_PRE:
			strcpy(file_path, "gl/shaders/pre/");
			break;
		case SGL_POST:
			strcpy(file_path, "gl/shaders/pre/");
			break;
		default:
			xavaBail("A really BIG oopsie happened here!");
			break;
	}

	strcat(file_path, name);

	struct shader* shader;
	switch(stage) {
		case SGL_VERT:
			strcat(file_path, "/vertex.glsl");
			xavaLogCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG,
					file_path, &returned_path) == false,
					"Failed to load '%s'!", file_path);
			shader = &program->vert;
			break;
		case SGL_GEO:
			strcat(file_path, "/geometry.glsl");
			xavaLogCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG,
					file_path, &returned_path) == false,
					"Failed to load '%s'!", file_path);
			shader = &program->geo;
			break;
		case SGL_FRAG:
			strcat(file_path, "/fragment.glsl");
			xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG,
					file_path, &returned_path) == false,
					"Failed to load '%s'!", file_path);
			shader = &program->frag;
			break;
	}

	// load file
	shader->path = strdup(returned_path);
	file = xavaReadFile(shader->path);
	shader->text = xavaDuplicateMemory(file->data, file->size);
	xavaCloseFile(file);

	// add watcher
	a->filename = shader->path;
	a->id = rand(); // WARN: Crash-prone cringe, please fix!
	a->ionotify = xava->ionotify;
	a->xava_ionotify_func = ionotify_callback;
	shader->watch = xavaIONotifyAddWatch(a);

	// clean escape
	free(returned_path);
}

void SGLShadersLoad(struct XAVA_HANDLE *xava) {
	XAVACONFIG config = xava->default_config.config;

	char *prePack, *postPack;

	prePack  = xavaConfigGetString(config, "gl", "pre_shaderpack", "default");
	postPack = xavaConfigGetString(config, "gl", "post_shaderpack", "default");

	resScale    = xavaConfigGetDouble(config, "gl", "resolution_scale", 1.0f);

	internal_SGLLoadShader(&pre, SGL_PRE, SGL_VERT, prePack, xava);
	internal_SGLLoadShader(&pre, SGL_PRE, SGL_GEO,  prePack, xava);
	internal_SGLLoadShader(&pre, SGL_PRE, SGL_FRAG, prePack, xava);
	internal_SGLLoadShader(&post, SGL_POST, SGL_VERT, postPack, xava);
	internal_SGLLoadShader(&post, SGL_POST, SGL_GEO,  postPack, xava);
	internal_SGLLoadShader(&post, SGL_POST, SGL_FRAG, postPack, xava);
}

GLint SGLShaderBuild(struct shader *shader, GLenum shader_type) {
	GLint status;

	shader->handle = glCreateShader(shader_type);
	xavaReturnErrorCondition(shader->handle == 0, 0, "Failed to build shader");

	glShaderSource(shader->handle, 1, (const char **) &shader->text, NULL);
	glCompileShader(shader->handle);

	glGetShaderiv(shader->handle, GL_COMPILE_STATUS, &status);
	if (!status) {
		char log[1000] = {0};
		GLsizei len;
		glGetShaderInfoLog(shader->handle, 1000, &len, log);

		bool unknown_type = false;
		int file, line, char_num;
		size_t matched_elements = sscanf(log, "%d:%d(%d)",
				&file, &line, &char_num);
		if(matched_elements < 3) {
			size_t matched_elements = sscanf(log, "%d(%d)",
					&line, &char_num);
			if(matched_elements < 2)
				unknown_type = true;
		}

		if(unknown_type) {
			xavaError("Error: Compiling '%s' failed\n%*s",
					shader->path, len, log);
		} else {
			char *source_line;
			char *string_ptr = shader->text;
			for(int i=0; i < line-1; i++) {
				string_ptr = strchr(string_ptr, '\n');
				string_ptr++;
			}
			source_line = strdup(string_ptr);
			*strchr(source_line, '\n') = '\0';

			xavaError("Error: Compiling '%s' failed\n%.*sCode:\n% 4d: %.*s",
					shader->path, len, log, line, 256, source_line);

			free(source_line);
		}

		return status;
	}

	xavaSpam("Compiling '%s' successful", shader->path);

	// free text and path info
	free(shader->path);
	free(shader->text);

	return status;
}

void SGLCreateProgram(struct SGLprogram *program) {
	GLint status;

	program->program = glCreateProgram();
	SGLShaderBuild(&program->vert, GL_VERTEX_SHADER);
	if(program->geo.text) // optional stage, we check if it's included in the shader pack
		SGLShaderBuild(&program->geo, GL_GEOMETRY_SHADER);
	SGLShaderBuild(&program->frag, GL_FRAGMENT_SHADER);

	glAttachShader(program->program, program->vert.handle);
	if(program->geo.text) // optional stage, we check if it's included in the shader pack
		glAttachShader(program->program, program->geo.handle);
	glAttachShader(program->program, program->frag.handle);
	glLinkProgram(program->program);

	glDeleteShader(program->frag.handle);
	if(program->geo.text) // optional stage, we check if it's included in the shader pack
		glDeleteShader(program->geo.handle);
	glDeleteShader(program->vert.handle);

	glGetProgramiv(program->program, GL_LINK_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetProgramInfoLog(program->program, 1000, &len, log);
		xavaBail("Error: linking:\n%*s\n", len, log);
	}
}

void SGLInit(struct XAVA_HANDLE *xava) {
	struct config_params *conf = &xava->conf;

	// automatically assign this so it isn't invalid during framebuffer creation
	xava->w = conf->w;
	xava->h = conf->h+conf->shdw;

	SGLCreateProgram(&pre);

	// color
	SHADER_FGCOL      = glGetUniformLocation(pre.program, "foreground_color");
	SHADER_BGCOL      = glGetUniformLocation(pre.program, "background_color");

	// sys info provider
	SHADER_TIME       = glGetUniformLocation(pre.program, "time");
	SHADER_INTENSITY  = glGetUniformLocation(pre.program, "intensity");

	// geometry
	SHADER_BARS          = glGetAttribLocation( pre.program, "fft_bars");
	SHADER_RESOLUTION    = glGetUniformLocation(pre.program, "resolution");
	SHADER_REST          = glGetUniformLocation(pre.program, "rest");
	SHADER_BAR_WIDTH     = glGetUniformLocation(pre.program, "bar_width");
	SHADER_BAR_SPACING   = glGetUniformLocation(pre.program, "bar_spacing");
	SHADER_BAR_COUNT     = glGetUniformLocation(pre.program, "bar_count");

	glUseProgram(pre.program);

	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
	glEnable(GL_DEPTH_TEST);

	// we just need working pointers so that realloc() works
	vertexData = malloc(1);

	// gradients
	if(conf->gradients)
		gradientColor = malloc(4*sizeof(GLfloat)*conf->gradients);

	for(int i=0; i<conf->gradients; i++) {
		uint32_t grad_col;
		sscanf(conf->gradient_colors[i], "#%x", &grad_col);
		gradientColor[i*4+0] = ARGB_R_32(grad_col) / 255.0;
		gradientColor[i*4+1] = ARGB_G_32(grad_col) / 255.0;
		gradientColor[i*4+2] = ARGB_B_32(grad_col) / 255.0;
		//gradientColor[i*4+3] = ARGB_A_32(grad_col) / 255.0;
		gradientColor[i*4+3] = conf->foreground_opacity;
	}

	shouldRestart = false;
}

void SGLApply(struct XAVA_HANDLE *xava){
	struct config_params *conf = &xava->conf;

	glUseProgram(pre.program);

	// reallocate and attach verticies data
	vertexData = realloc(vertexData, sizeof(GLfloat)*xava->bars*2);
	glVertexAttribPointer(SHADER_BARS, 3, GL_FLOAT, GL_FALSE, 0, vertexData);

	// update screen resoltion
	glUniform2f(SHADER_RESOLUTION, xava->w, xava->h);

	// update spacing info
	glUniform1f(SHADER_REST, (float)2.0f*xava->rest    / xava->conf.w);
	glUniform1f(SHADER_BAR_WIDTH, (float)2.0f*xava->conf.bw / xava->conf.w);
	glUniform1f(SHADER_BAR_SPACING, (float)2.0f*xava->conf.bs / xava->conf.w);
	glUniform1f(SHADER_BAR_COUNT, xava->bars);

	SHADER_GRAD_SECT_COUNT = glGetUniformLocation(pre.program, "gradient_sections");
	SHADER_GRADIENTS  = glGetUniformLocation(pre.program, "gradient_color");

	glUniform1f(SHADER_GRAD_SECT_COUNT, conf->gradients ? conf->gradients-1 : 0);
	glUniform4fv(SHADER_GRADIENTS, conf->gradients, gradientColor);

	// "clear" the screen
	SGLClear(xava);
}

XG_EVENT SGLEvent(struct XAVA_HANDLE *xava) {
	if(shouldRestart) {
		shouldRestart = false;
		return XAVA_RELOAD;
	}
	return XAVA_IGNORE;
}

// The original intention of this was to be called when the screen buffer was "unsafe" or "dirty"
// This is not needed in EGL since glClear() is called on each frame. HOWEVER, this clear function
// is often preceded by a slight state change such as a color change, so we pass color info to the
// shaders HERE and ONLY HERE.
void SGLClear(struct XAVA_HANDLE *xava) {
	struct config_params *conf = &xava->conf;

	// if you want to fiddle with certain uniforms from a shader, YOU MUST SWITCH TO IT
	// (https://www.khronos.org/opengl/wiki/GLSL_:_common_mistakes#glUniform_doesn.27t_work)
	glUseProgram(pre.program);

	// set and attach foreground color
	uint32_t fgcol = conf->col;
	glUniform4f(SHADER_FGCOL, ARGB_R_32(fgcol)/255.0, ARGB_G_32(fgcol)/255.0,
			ARGB_B_32(fgcol)/255.0, conf->foreground_opacity);

	// set background clear color
	uint32_t bgcol = conf->bgcol;
	float bgcolF = conf->background_opacity/255.0;
	glUniform4f(SHADER_BGCOL, ARGB_R_32(bgcol)*bgcolF, ARGB_G_32(bgcol)*bgcolF,
			ARGB_B_32(bgcol)*bgcolF, conf->background_opacity);

	//uint32_t shdw_col = conf->shdw_col;
	//glUniform4f(POST_SHADOW_COLOR, ARGB_R_32(shdw_col), ARGB_G_32(shdw_col),
	//		ARGB_B_32(shdw_col), 1.0);


	//glUniform2f(POST_SHADOW_OFFSET,
	//		((float)conf->shdw)/((float)xava->w)*-0.5,
	//		((float)conf->shdw)/((float)xava->h)*0.5);
}

void SGLDraw(struct XAVA_HANDLE *xava) {
	struct config_params *conf = &xava->conf;

	// restrict time variable to one hour because floating point precision issues
	float currentTime = (float)fmodl((long double)xavaGetTime()/(long double)1000.0, 3600.0);
	float intensity = 0.0;

	/**
	 * Here we start rendering to the texture
	 **/

	// i am speed
	for(register int i=0; i<xava->bars; i++) {
		float value = (float) xava->f[i] / xava->h;
		vertexData[i<<1] = i;
		vertexData[(i<<1)+1] = value;
		intensity += value;
	}

	// since im not bothering to do the math, this'll do
	// - used to balance out intensity across various number of bars
	intensity /= xava->bars;

	// bind render target to texture
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, xava->w*resScale, xava->h*resScale);

	// switch to pre shaders
	glUseProgram(pre.program);

	// update time
	glUniform1f(SHADER_TIME, currentTime);

	// update intensity
	glUniform1f(SHADER_INTENSITY, intensity);

	// GL_COLOR_BUFFER_BIT | <- let the shaders handle this one
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// pointers get reset after each glUseProgram(), that's why this is done
	glVertexAttribPointer(SHADER_BARS, 2, GL_FLOAT, GL_FALSE, 0, vertexData);

	glEnableVertexAttribArray(SHADER_BARS);

	// You use the number of verticies, but not the actual polygon count
	glDrawArrays(GL_POINTS, 0, xava->bars);

	glDisableVertexAttribArray(SHADER_BARS);
}

void SGLDestroyProgram(struct SGLprogram *program) {
	glDeleteProgram(program->program);
}

void SGLCleanup(struct XAVA_HANDLE *xava) {
	// WARN: May be leaky here
	// cleanup watches
	// xavaIONotifyEndWatch(xava->ionotify, preFragWatch);
	// xavaIONotifyEndWatch(xava->ionotify, preVertWatch);

	// delete both pipelines
	SGLDestroyProgram(&pre);
	SGLDestroyProgram(&post);

	free(gradientColor);
	free(vertexData);
}

