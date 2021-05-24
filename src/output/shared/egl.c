#include "../../shared.h"
#include "egl.h"

void EGLShadersLoad(struct EGLprogram *pre, struct EGLprogram *post) {
	char *preFragPath, *preVertPath;
	char *postFragPath, *postVertPath;
	RawData *preFrag, *preVert, *postFrag, *postVert;

	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG,
			"egl/shaders/pre.frag", &preFragPath) == false, 
			"Failed to load pre-render fragment shader!");
	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG, 
			"egl/shaders/pre.vert", &preVertPath) == false,
			"Failed to load pre-render vertex shader!");
	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG,
			"egl/shaders/post.frag", &postFragPath) == false, 
			"Failed to load post-render fragment shader!");
	xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG, 
			"egl/shaders/post.vert", &postVertPath) == false,
			"Failed to load post-render vertex shader!");

	preFrag = xavaReadFile(preFragPath);
	free(preFragPath);
	pre->fragText = xavaDuplicateMemory(preFrag->data, preFrag->size);
	xavaCloseFile(preFrag);

	preVert = xavaReadFile(preVertPath);
	free(preVertPath);
	pre->vertText = xavaDuplicateMemory(preVert->data, preVert->size);
	xavaCloseFile(preVert);

	postFrag = xavaReadFile(postFragPath);
	free(postFragPath);
	post->fragText = xavaDuplicateMemory(postFrag->data, postFrag->size);
	xavaCloseFile(postFrag);

	postVert = xavaReadFile(postVertPath);
	free(postVertPath);
	post->vertText = xavaDuplicateMemory(postVert->data, postVert->size);
	xavaCloseFile(postVert);
}

EGLint EGLShaderBuild(const char *source, GLenum shader_type) {
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

void EGLCreateProgram(struct EGLprogram *program) {
	GLint status;

	program->program = glCreateProgram();
	program->frag = EGLShaderBuild(program->fragText, GL_FRAGMENT_SHADER); 
	program->vert = EGLShaderBuild(program->vertText, GL_VERTEX_SHADER);

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
