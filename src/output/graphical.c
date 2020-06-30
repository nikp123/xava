#include "graphical.h"
#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef GLX
	GLXContext xavaGLXContext;
	GLXFBConfig* xavaFBConfig;
	int GLXmode;
#endif

void calculate_win_pos(int *winX, int *winY, int winW, int winH, int scrW, int scrH, char *winPos) {
	if(!strcmp(winPos, "top")){
		(*winX) = (scrW - winW) / 2 + (*winX);
		(*winY) -= p.shdw;
	}else if(!strcmp(winPos, "bottom")){
		(*winX) = (scrW - winW) / 2 + (*winX);
		(*winY) = (scrH - winH) - (*winY) + p.shdw;
	}else if(!strcmp(winPos, "top_left")){
		(*winY) -= p.shdw;
	}else if(!strcmp(winPos, "top_right")){
		(*winX) = (scrW - winW) - (*winX);
		(*winY) -= p.shdw;
	}else if(!strcmp(winPos, "left")){
		(*winY) = (scrH - winH) / 2;
	}else if(!strcmp(winPos, "right")){
		(*winX) = (scrW - winW) - (*winX);
		(*winY) = (scrH - winH) / 2 + (*winY);
	}else if(!strcmp(winPos, "bottom_left")){
		(*winY) = (scrH - winH) - (*winY) + p.shdw;
	}else if(!strcmp(winPos, "bottom_right")){
		(*winX) = (scrW - winW) - (*winX);
		(*winY) = (scrH - winH) - (*winY) + p.shdw;
	}else if(!strcmp(winPos, "center")){
		(*winX) = (scrW - winW) / 2 + (*winX);
		(*winY) = (scrH - winH) / 2 + (*winY);
	}
	// Some error checking
	#ifdef DEBUG
		if(winX > scrW - winW) printf("Warning: Screen out of bounds (X axis)!");
		if(winY > scrH - winH) printf("Warning: Screen out of bounds (Y axis)!");
	#endif
}

#ifdef GL
static unsigned int VBObuffer;
static unsigned int shader;
static float glBars[8];

// yolo
static const char* dumpAFile(char *filename, size_t *size) {
	FILE *fp = fopen(filename, "ro");
	if(!fp) {
		fprintf(stderr, "this file broke lmao: %s\n", filename);
		return NULL; // this crashes any misuse or mistreating programmer
	}

	// get the file size
	fseek(fp, 0, SEEK_END);
	(*size) = ftell(fp);
	rewind(fp);

	// muh memory allocation
	char *content = malloc((*size)+1);
	if(!content) {
		fprintf(stderr, "computer is broken or user/programmer doesn't know what's (s)he doing!\n");
		return NULL;
	}

	// read the damn file
	fread(content, (*size), 1, fp);

	// muh memory leaks
	fclose(fp);

	return content;
}

static unsigned int CompileShader(unsigned int type, char *source) {
	unsigned int id = glCreateShader(type);
	glShaderSource(id, 1, &source, NULL);
	glCompileShader(id);

	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if(result == GL_FALSE) {
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char *message = (char*)alloca(length*sizeof(char));
		glGetShaderInfoLog(id, length, &length, message);
		fprintf(stderr, "Failed to compile %s shader: %s\n", type==GL_VERTEX_SHADER? "vertex" : "fragment", message);

		glDeleteShader(id);
		return 0;
	}

	return id;
}

static unsigned int CreateShader(const char* vertexShader,
								 const char* fragmentShader) {
	unsigned int program = glCreateProgram();
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

void VBOGLsetup() {
	glewInit();

	glGenBuffers(1, &VBObuffer);
	glBindBuffer(GL_ARRAY_BUFFER, VBObuffer);

	// draw over the entire screen essentially
	// and let the shader do the actual work
	glBars[0] = 0.0; glBars[1] = 0.0;
	glBars[2] = p.w; glBars[3] = 0.0;
	glBars[4] = p.w; glBars[5] = p.h;
	glBars[6] = 0.0; glBars[7] = p.h;

	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*8, glBars, GL_STATIC_READ);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, sizeof(float)*2, 0);

	size_t iDontCare;
	const char *vertex = dumpAFile("default.vert", &iDontCare);
	const char *fragment = dumpAFile("default.frag", &iDontCare);

	shader = CreateShader(vertex, fragment);
	glUseProgram(shader);
}

void VBOGLdestroy() {
	//free(glBars);
}

int drawGLBars(int rest, int bars, float colors[12], float gradColors[24], int *f) {
	glUniform4f(glGetUniformLocation(shader, "foreground_color"), 
		colors[0], colors[1], colors[2], colors[3]);
	glUniform4f(glGetUniformLocation(shader, "background_color"),
		colors[8], colors[9], colors[10], colors[11]);
	glUniform4f(glGetUniformLocation(shader, "shadow_color"), 
		colors[4], colors[5], colors[6], colors[7]);
	glUniform1i(glGetUniformLocation(shader, "shadow_size"), p.shdw);
	glUniform3fv(glGetUniformLocation(shader, "gradient_color"), p.gradients, gradColors);
	glUniform1i(glGetUniformLocation(shader, "gradients"), p.gradients);

	glUniform1i(glGetUniformLocation(shader, "bars"), bars);
	glUniform1i(glGetUniformLocation(shader, "rest"), rest);
	glUniform1i(glGetUniformLocation(shader, "bar_width"), p.bw);
	glUniform1i(glGetUniformLocation(shader, "bar_spacing"), p.bs);
	glUniform1i(glGetUniformLocation(shader, "window_width"), p.w);
	glUniform1i(glGetUniformLocation(shader, "window_height"), p.h);

	for(int i=0; i<bars; i++) {
		int a = p.h-p.shdw*2;
		if(f[i]>a) f[i] = a;
	}
	glUniform1iv(glGetUniformLocation(shader, "f"), bars, f);

	glBufferSubData(GL_ARRAY_BUFFER, 0, 8*sizeof(float), glBars);
	glDrawArrays(GL_QUADS, 0, 4);
	return 0;
}
#endif
