#include <stdio.h>
#include <string.h>
#include <math.h>

#include "gl_shared.h"

#include "../../shared.h"
#include "../graphical.h"

// this is so that loading shaders is managable
struct SGLprogram {
	struct shader {
		char *path, *text;
		GLuint handle;
		XAVAIONOTIFYWATCH watch;
	} frag, vert, geo;
	GLuint program;
} pre, post;

static struct FBO {
	GLuint framebuffer;
	GLuint final_texture;
	GLuint depth_texture;
} FBO;

static GLfloat *vertexData;
static GLfloat projectionMatrix[16] =
	{2.0, 0.0, 0.0, -1.0,
	0.0, 2.0, 0.0, -1.0,
	0.0, 0.0, -1.0, 0.0,
	0.0, 0.0, 0.0, 1.0};

static GLfloat postVertices[] = {
	-1.0f, -1.0f,  // Position 0
	 0.0f,  0.0f,  // TexCoord 0
	 1.0f, -1.0f,  // Position 1
	 1.0f,  0.0f,  // TexCoord 1
	 1.0f,  1.0f,  // Position 2
	 1.0f,  1.0f,  // TexCoord 2
	-1.0f,  1.0f,  // Position 3
	 0.0f,  1.0f}; // TexCoord 3

static GLfloat *gradientColor;

static GLuint PRE_BARS;
static GLuint PRE_FGCOL;
static GLuint PRE_RESOLUTION;
static GLuint PRE_PROJMATRIX;
static GLuint PRE_GRAD_SECT_COUNT;
static GLuint PRE_GRADIENTS;
static GLuint PRE_TIME;
static GLuint PRE_INTENSITY;

static GLuint POST_BGCOL;
static GLuint POST_POS;
static GLuint POST_TEXCOORD;
static GLuint POST_TEXTURE;
static GLuint POST_DEPTH;
static GLuint POST_SHADOW_COLOR;
static GLuint POST_SHADOW_OFFSET;
static GLuint POST_TIME;
static GLuint POST_INTENSITY;

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

// Better to be ugly and long then short and buggy
void SGLShadersLoad(struct XAVA_HANDLE *xava) {
	XAVACONFIG config = xava->default_config.config;
	XAVAIONOTIFYWATCHSETUP a;
	MALLOC_SELF(a, 1);

	char *preShaderPack, *postShaderPack;

	preShaderPack  = xavaConfigGetString(config, "gl", "pre_shaderpack", "default");
	postShaderPack = xavaConfigGetString(config, "gl", "post_shaderpack", "default");

	resScale       = xavaConfigGetDouble(config, "gl", "resolution_scale", 1.0f);

	RawData *file;
	char *returned_path;
	char *file_path = malloc(MAX_PATH);
	strcpy(file_path, "gl/shaders/pre/");
	strcat(file_path, preShaderPack);
	strcat(file_path, "/fragment.glsl");
	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG,
			file_path, &returned_path) == false,
			"Failed to load pre-render fragment shader!");
	pre.frag.path = strdup(returned_path);
	a->filename = pre.frag.path;
	a->id = 2;
	a->ionotify = xava->ionotify;
	a->xava_ionotify_func = ionotify_callback;
	pre.frag.watch = xavaIONotifyAddWatch(a);
	free(returned_path);

	strcpy(file_path, "gl/shaders/pre/");
	strcat(file_path, preShaderPack);
	strcat(file_path, "/vertex.glsl");
	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG,
			file_path, &returned_path) == false,
			"Failed to load pre-render vertex shader!");
	pre.vert.path = strdup(returned_path);
	a->filename = pre.vert.path;
	a->id = 3;
	a->ionotify = xava->ionotify;
	a->xava_ionotify_func = ionotify_callback;
	pre.vert.watch = xavaIONotifyAddWatch(a);
	free(returned_path);

	strcpy(file_path, "gl/shaders/pre/");
	strcat(file_path, preShaderPack);
	strcat(file_path, "/geometry.glsl");
	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG,
			file_path, &returned_path) == false,
			"Failed to load pre-render geometry shader!");
	pre.geo.path = strdup(returned_path);
	a->filename = pre.geo.path;
	a->id = 6;
	a->ionotify = xava->ionotify;
	a->xava_ionotify_func = ionotify_callback;
	pre.geo.watch = xavaIONotifyAddWatch(a);
	free(returned_path);

	strcpy(file_path, "gl/shaders/post/");
	strcat(file_path, postShaderPack);
	strcat(file_path, "/fragment.glsl");
	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG,
			file_path, &returned_path) == false,
			"Failed to load post-render fragment shader!");
	post.frag.path = strdup(returned_path);
	a->filename = post.frag.path;
	a->id = 4;
	a->ionotify = xava->ionotify;
	a->xava_ionotify_func = ionotify_callback;
	post.frag.watch = xavaIONotifyAddWatch(a);
	free(returned_path);

	strcpy(file_path, "gl/shaders/post/");
	strcat(file_path, postShaderPack);
	strcat(file_path, "/vertex.glsl");
	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG,
			file_path, &returned_path) == false,
			"Failed to load post-render vertex shader!");
	post.vert.path = strdup(returned_path);
	a->filename = post.vert.path;
	a->id = 5;
	a->ionotify = xava->ionotify;
	a->xava_ionotify_func = ionotify_callback;
	post.vert.watch = xavaIONotifyAddWatch(a);
	free(returned_path);

	// check (optionally for shader here)
	post.geo.path = NULL;

	file = xavaReadFile(pre.frag.path);
	pre.frag.text = xavaDuplicateMemory(file->data, file->size);
	xavaCloseFile(file);

	file = xavaReadFile(pre.vert.path);
	pre.vert.text = xavaDuplicateMemory(file->data, file->size);
	xavaCloseFile(file);

	file = xavaReadFile(pre.geo.path);
	pre.geo.text = xavaDuplicateMemory(file->data, file->size);
	xavaCloseFile(file);

	file = xavaReadFile(post.frag.path);
	post.frag.text = xavaDuplicateMemory(file->data, file->size);
	xavaCloseFile(file);

	file = xavaReadFile(post.vert.path);
	post.vert.text = xavaDuplicateMemory(file->data, file->size);
	xavaCloseFile(file);

	// optional stage
	//file = xavaReadFile(post.geo.path);
	//post.geo.text = xavaDuplicateMemory(file->data, file->size);
	//xavaCloseFile(file);

	free(file_path);
	free(a);
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

	return status;
}

void SGLCreateProgram(struct SGLprogram *program) {
	GLint status;

	program->program = glCreateProgram();
	SGLShaderBuild(&program->vert, GL_VERTEX_SHADER);
	if(program->geo.path) // optional stage, we check if it's included in the shader pack
		SGLShaderBuild(&program->geo, GL_GEOMETRY_SHADER);
	SGLShaderBuild(&program->frag, GL_FRAGMENT_SHADER);

	glAttachShader(program->program, program->vert.handle);
	if(program->geo.path) // optional stage, we check if it's included in the shader pack
		glAttachShader(program->program, program->geo.handle);
	glAttachShader(program->program, program->frag.handle);
	glLinkProgram(program->program);

	glDeleteShader(program->frag.handle);
	if(program->geo.path) // optional stage, we check if it's included in the shader pack
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
	SGLCreateProgram(&post);

	PRE_BARS        = glGetAttribLocation(pre.program, "pos");
	PRE_FGCOL      = glGetUniformLocation(pre.program, "color");
	PRE_RESOLUTION = glGetUniformLocation(pre.program, "u_resolution");
	PRE_PROJMATRIX = glGetUniformLocation(pre.program, "projectionMatrix");
	PRE_TIME       = glGetUniformLocation(pre.program, "u_time");
	PRE_INTENSITY  = glGetUniformLocation(pre.program, "intensity");

	POST_POS           = glGetAttribLocation(post.program, "a_position");
	POST_TEXCOORD      = glGetAttribLocation(post.program, "a_texCoord");
	POST_TEXTURE       = glGetUniformLocation(post.program, "s_texture");
	POST_DEPTH         = glGetUniformLocation(post.program, "s_depth");
	POST_SHADOW_COLOR  = glGetUniformLocation(post.program, "shadow_color");
	POST_SHADOW_OFFSET = glGetUniformLocation(post.program, "shadow_offset");
	POST_TIME          = glGetUniformLocation(post.program, "u_time");
	POST_INTENSITY     = glGetUniformLocation(post.program, "intensity");
	POST_BGCOL         = glGetUniformLocation(post.program, "bgcolor");

	glUseProgram(pre.program);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);

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

	glUseProgram(post.program);

	// set texture properties
	glGenTextures(1,               &FBO.final_texture);
	glBindTexture(GL_TEXTURE_2D,   FBO.final_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xava->w*resScale, xava->h*resScale,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// set texture properties
	glGenTextures(1,      &FBO.depth_texture);
	glBindTexture(GL_TEXTURE_2D, FBO.depth_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, xava->w*resScale,
			xava->h*resScale, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// set framebuffer properties
	glGenFramebuffers(1,  &FBO.framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO.framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, FBO.final_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			GL_TEXTURE_2D, FBO.depth_texture, 0);

	// check if it borked
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	xavaBailCondition(status != GL_FRAMEBUFFER_COMPLETE,
			"Failed to create framebuffer(s)! Error code 0x%X\n",
			status);

	glUseProgram(pre.program);

	// reallocate and attach verticies data
	vertexData = realloc(vertexData, sizeof(GLfloat)*xava->bars*2);
	glVertexAttribPointer(PRE_BARS, 2, GL_FLOAT, GL_FALSE, 0, vertexData);

	// update screen resoltion
	glUniform2f(PRE_RESOLUTION, xava->w, xava->h);

	// update projection matrix
	// trick: use projection matrix to optimize for shadows so our CPU doesn't have
	// to work as hard -> math found here: https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glOrtho.xml

	// horizontal scaling not done due to artifacting
	//projectionMatrix[0] = 2.0/(conf->w+conf->shdw);
	//projectionMatrix[3] = -((float)conf->w/(float)(conf->w+conf->shdw));
	// vertical scaling
	//projectionMatrix[5] = 2.0/(conf->h+conf->shdw);
	//projectionMatrix[7] = -((float)xava->h/(float)(xava->h+conf->shdw));

	// do image scaling
	projectionMatrix[0] = 2.0/xava->w;
	projectionMatrix[5] = 2.0/xava->h;

	// do image translation
	projectionMatrix[3] = (float)xava->x/xava->w*2.0 - 1.0;
	projectionMatrix[7] = 1.0 - (float)(xava->y+conf->h)/xava->h*2.0;

	glUniformMatrix4fv(PRE_PROJMATRIX, 1, GL_FALSE, (GLfloat*) projectionMatrix);

	// enable superior color blending (change my mind)
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	PRE_GRAD_SECT_COUNT = glGetUniformLocation(pre.program, "gradient_sections");
	PRE_GRADIENTS  = glGetUniformLocation(pre.program, "gradient_color");

	glUniform1f(PRE_GRAD_SECT_COUNT, conf->gradients ? conf->gradients-1 : 0);
	glUniform4fv(PRE_GRADIENTS, conf->gradients, gradientColor);

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
	glUniform4f(PRE_FGCOL, ARGB_R_32(fgcol)/255.0, ARGB_G_32(fgcol)/255.0,
			ARGB_B_32(fgcol)/255.0, conf->foreground_opacity);

	glUseProgram(post.program);

	// set background clear color
	uint32_t bgcol = conf->bgcol;
	float bgcolF = conf->background_opacity/255.0;
	glUniform4f(POST_BGCOL, ARGB_R_32(bgcol)*bgcolF, ARGB_G_32(bgcol)*bgcolF,
			ARGB_B_32(bgcol)*bgcolF, conf->background_opacity);

	uint32_t shdw_col = conf->shdw_col;
	glUniform4f(POST_SHADOW_COLOR, ARGB_R_32(shdw_col), ARGB_G_32(shdw_col),
			ARGB_B_32(shdw_col), 1.0);


	glUniform2f(POST_SHADOW_OFFSET,
			((float)conf->shdw)/((float)xava->w)*-0.5,
			((float)conf->shdw)/((float)xava->h)*0.5);
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
		vertexData[i<<1] = (float) xava->f[i] / xava->h * 2.0 - 1.0;
		vertexData[(i<<1)+1] = i;
	}

	// since im not bothering to do the math, this'll do
	// - used to balance out intensity across various number of bars
	intensity /= xava->bars;

	// bind render target to texture
	//glBindFramebuffer(GL_FRAMEBUFFER, FBO.framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, xava->w*resScale, xava->h*resScale);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	// switch to pre shaders
	glUseProgram(pre.program);

	// update time
	glUniform1f(PRE_TIME, currentTime);

	// update intensity
	glUniform1f(PRE_INTENSITY, intensity);

	// GL_COLOR_BUFFER_BIT | <- let the shaders handle this one
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// pointers get reset after each glUseProgram(), that's why this is done
	glVertexAttribPointer(PRE_BARS, 2, GL_FLOAT, GL_FALSE, 0, vertexData);

	glEnableVertexAttribArray(PRE_BARS);

	// You use the number of verticies, but not the actual polygon count
	glDrawArrays(GL_POINTS, 0, xava->bars);

	glDisableVertexAttribArray(PRE_BARS);

	/**
	 * Once the texture has been conpleted, we now activate a seperate pipeline
	 * which just displays that texture to the actual framebuffer for easier
	 * shader writing
	 * */

	// Change framebuffer
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glViewport(0, 0, xava->w, xava->h);

	//// Switch to post shaders
	//glUseProgram(post.program);

	//// update time
	//glUniform1f(POST_TIME, currentTime);

	//// update intensity
	//glUniform1f(POST_INTENSITY, intensity);

	//// Clear the color buffer
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// set attribute array pointers for the texture unit and vertex unit
	//glVertexAttribPointer(POST_POS, 2, GL_FLOAT,
	//	GL_FALSE, 4 * sizeof(GLfloat), postVertices);
	//glVertexAttribPointer(POST_TEXCOORD, 2, GL_FLOAT,
	//	GL_FALSE, 4 * sizeof(GLfloat), &postVertices[2]);

	//// enable the use of the following attribute elements
	//glEnableVertexAttribArray(POST_POS);
	//glEnableVertexAttribArray(POST_TEXCOORD);

	//// Bind the textures
	//// Render texture
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, FBO.final_texture);
	//glUniform1i(POST_TEXTURE, 0);

	//// Depth texture
	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, FBO.depth_texture);
	//glUniform1i(POST_DEPTH, 1);

	//// draw frame
	//GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

	//glDisableVertexAttribArray(POST_POS);
}

void SGLDestroyProgram(struct SGLprogram *program) {
	free(program->frag.path);
	free(program->frag.text);
	free(program->vert.path);
	free(program->vert.text);
	glDeleteProgram(program->program);
}

void SGLCleanup(struct XAVA_HANDLE *xava) {
	// WARN: May be leaky here
	// cleanup watches
	// xavaIONotifyEndWatch(xava->ionotify, preFragWatch);
	// xavaIONotifyEndWatch(xava->ionotify, preVertWatch);
	// xavaIONotifyEndWatch(xava->ionotify, postFragWatch);
	// xavaIONotifyEndWatch(xava->ionotify, postVertWatch);

	// restore framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// unbind textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);

	// cleanup FBO left-overs
	glDeleteFramebuffers(1, &FBO.framebuffer);
	glDeleteTextures(1, &FBO.depth_texture);
	glDeleteTextures(1, &FBO.final_texture);

	// delete both pipelines
	SGLDestroyProgram(&pre);
	SGLDestroyProgram(&post);

	free(gradientColor);
	free(vertexData);
}

