#include "gl_shared.h"

#include "../../shared.h"
#include "../graphical.h"

// this is so that loading shaders is managable
struct SGLprogram {
	char *fragText, *vertText;
	GLuint vert, frag;
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

static GLuint PRE_POS;
static GLuint PRE_FGCOL;
static GLuint PRE_RESOLUTION;
static GLuint PRE_PROJMATRIX;
static GLuint PRE_GRAD_SECT_COUNT;
static GLuint PRE_GRADIENTS;

static GLuint POST_POS;
static GLuint POST_TEXCOORD;
static GLuint POST_TEXTURE;
static GLuint POST_DEPTH;
static GLuint POST_SHADOW_COLOR;
static GLuint POST_SHADOW_OFFSET;

void SGLShadersLoad() {
	char *preFragPath, *preVertPath;
	char *postFragPath, *postVertPath;
	RawData *preFrag, *preVert, *postFrag, *postVert;

	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG,
			"gl/shaders/pre.frag", &preFragPath) == false, 
			"Failed to load pre-render fragment shader!");
	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG, 
			"gl/shaders/pre.vert", &preVertPath) == false,
			"Failed to load pre-render vertex shader!");
	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG,
			"gl/shaders/post.frag", &postFragPath) == false, 
			"Failed to load post-render fragment shader!");
	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG, 
			"gl/shaders/post.vert", &postVertPath) == false,
			"Failed to load post-render vertex shader!");

	preFrag = xavaReadFile(preFragPath);
	free(preFragPath);
	pre.fragText = xavaDuplicateMemory(preFrag->data, preFrag->size);
	xavaCloseFile(preFrag);

	preVert = xavaReadFile(preVertPath);
	free(preVertPath);
	pre.vertText = xavaDuplicateMemory(preVert->data, preVert->size);
	xavaCloseFile(preVert);

	postFrag = xavaReadFile(postFragPath);
	free(postFragPath);
	post.fragText = xavaDuplicateMemory(postFrag->data, postFrag->size);
	xavaCloseFile(postFrag);

	postVert = xavaReadFile(postVertPath);
	free(postVertPath);
	post.vertText = xavaDuplicateMemory(postVert->data, postVert->size);
	xavaCloseFile(postVert);
}


GLint SGLShaderBuild(const char *source, GLenum shader_type) {
	GLint shader;
	GLint status;

	shader = glCreateShader(shader_type);
	xavaBailCondition(shader == 0, "Failed to build shader");

	glShaderSource(shader, 1, (const char **) &source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetShaderInfoLog(shader, 1000, &len, log);
		fputs(source, stdout);
		xavaBail("Error: compiling %s: %*s\n",
				shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment",
				len, log);
	}

	xavaSpam("Compiling %s shader successful",
			shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment");

	return shader;
}

void SGLCreateProgram(struct SGLprogram *program) {
	GLint status;

	program->program = glCreateProgram();
	program->frag = SGLShaderBuild(program->fragText, GL_FRAGMENT_SHADER); 
	program->vert = SGLShaderBuild(program->vertText, GL_VERTEX_SHADER);

	glAttachShader(program->program, program->frag);
	glAttachShader(program->program, program->vert);
	glLinkProgram(program->program);

	glGetProgramiv(program->program, GL_LINK_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetProgramInfoLog(program->program, 1000, &len, log);
		xavaBail("Error: linking:\n%*s\n", len, log);
	}

	// this might move
	free(program->fragText);
	free(program->vertText);
}

void SGLInit(struct XAVA_HANDLE *xava) {
	struct config_params *conf = &xava->conf;

	SGLCreateProgram(&pre);
	SGLCreateProgram(&post);

	PRE_POS        = glGetAttribLocation(pre.program,  "pos");
	PRE_FGCOL      = glGetUniformLocation(pre.program, "color");
	PRE_RESOLUTION = glGetUniformLocation(pre.program, "u_resolution");
	PRE_PROJMATRIX = glGetUniformLocation(pre.program, "projectionMatrix");

	POST_POS           = glGetAttribLocation(post.program, "a_position");
	POST_TEXCOORD      = glGetAttribLocation(post.program, "a_texCoord");
	POST_TEXTURE       = glGetUniformLocation(post.program, "s_texture");
	POST_DEPTH         = glGetUniformLocation(post.program, "s_depth");
	POST_SHADOW_COLOR  = glGetUniformLocation(post.program, "shadow_color");
	POST_SHADOW_OFFSET = glGetUniformLocation(post.program, "shadow_offset");
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
}

void SGLApply(struct XAVA_HANDLE *xava){
	struct config_params *conf = &xava->conf;

	glUseProgram(post.program);

	// set texture properties
	glGenTextures(1,               &FBO.final_texture);
	glBindTexture(GL_TEXTURE_2D,   FBO.final_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, conf->w, conf->h,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// set texture properties
	glGenTextures(1,      &FBO.depth_texture);
	glBindTexture(GL_TEXTURE_2D, FBO.depth_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, conf->w, conf->h,
			0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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
	vertexData = realloc(vertexData, sizeof(GLfloat)*xava->bars*12);
	glVertexAttribPointer(PRE_POS, 2, GL_FLOAT, GL_FALSE, 0, vertexData);

	// since most of this information remains untouched, let's precalculate
	for(int i=0; i<xava->bars; i++) {
		vertexData[i*12]    = xava->rest + i*(conf->bs+conf->bw);
		vertexData[i*12+1]  = 1.0f;
		vertexData[i*12+2]  = vertexData[i*12];
		vertexData[i*12+3]  = 0.0f;
		vertexData[i*12+4]  = vertexData[i*12]+conf->bw;
		vertexData[i*12+5]  = 0.0f;
		vertexData[i*12+6]  = vertexData[i*12+4];
		vertexData[i*12+7]  = 1.0f;
		vertexData[i*12+8]  = vertexData[i*12+4];
		vertexData[i*12+9]  = 0.0f;
		vertexData[i*12+10] = vertexData[i*12];
		vertexData[i*12+11] = 1.0f;
	}

	// update screen resoltion
	glUniform2f(PRE_RESOLUTION, conf->w, conf->h);

	// update projection matrix
	// trick: use projection matrix to optimize for shadows so our CPU doesn't have
	// to work as hard -> math found here: https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glOrtho.xml

	// horizontal scaling not done due to artifacting
	//projectionMatrix[0] = 2.0/(conf->w+conf->shdw);
	//projectionMatrix[3] = -((float)conf->w/(float)(conf->w+conf->shdw));

	projectionMatrix[0] = 2.0/conf->w;
	projectionMatrix[5] = 2.0/(conf->h+conf->shdw);
	projectionMatrix[7] = -((float)conf->h/(float)(conf->h+conf->shdw));

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

	// set background clear color
	uint32_t bgcol = conf->bgcol;
	float bgcolF = conf->background_opacity/255.0;
	glClearColor(ARGB_R_32(bgcol)*bgcolF, ARGB_G_32(bgcol)*bgcolF,
			ARGB_B_32(bgcol)*bgcolF, conf->background_opacity);

	glUseProgram(post.program);

	uint32_t shdw_col = conf->shdw_col;
	glUniform4f(POST_SHADOW_COLOR, ARGB_R_32(shdw_col), ARGB_G_32(shdw_col),
			ARGB_B_32(shdw_col), 1.0);

	glUniform2f(POST_SHADOW_OFFSET,
			((float)conf->shdw)/((float)conf->w)*-0.5,
			((float)conf->shdw)/((float)conf->h)*0.5);
}

void SGLDraw(struct XAVA_HANDLE *xava) {
	struct config_params *conf = &xava->conf;

	/**
	 * Here we start rendering to the texture
	 **/

	// i am speed
	register GLfloat *d = vertexData;
	for(register int i=0; i<xava->bars; i++) {
		*(++d)  = xava->f[i];
		*(d+=6) = xava->f[i];
		*(d+=4) = xava->f[i];
		d++;
	}

	// bind render target to texture
	glBindFramebuffer(GL_FRAMEBUFFER, FBO.framebuffer);
	glViewport(0, 0, conf->w, conf->h);

	// switch to pre shaders
	glUseProgram(pre.program);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// pointers get reset after each glUseProgram(), that's why this is done
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

	// Change framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, conf->w, conf->h);

	// Switch to post shaders
	glUseProgram(post.program);

	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// set attribute array pointers for the texture unit and vertex unit
	glVertexAttribPointer(POST_POS, 2, GL_FLOAT,
		GL_FALSE, 4 * sizeof(GLfloat), postVertices);
	glVertexAttribPointer(POST_TEXCOORD, 2, GL_FLOAT,
		GL_FALSE, 4 * sizeof(GLfloat), &postVertices[2]);

	// enable the use of the following attribute elements
	glEnableVertexAttribArray(POST_POS);
	glEnableVertexAttribArray(POST_TEXCOORD);

	// Bind the textures
	// Render texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, FBO.final_texture);
	glUniform1i(POST_TEXTURE, 0);

	// Depth texture
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, FBO.depth_texture);
	glUniform1i(POST_DEPTH, 1);

	// draw frame
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

	glDisableVertexAttribArray(POST_POS);
	glDisableVertexAttribArray(POST_POS);
}

void SGLCleanup(void) {
	free(vertexData);
}

