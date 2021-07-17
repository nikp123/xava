#ifndef __GL_H
#define __GL_H

#include "gl_shared.h"

#include "../../shared.h"


void GLShadersLoad(void);
void GLInit(struct XAVA_HANDLE *xava);
void GLClear(struct XAVA_HANDLE *xava);
void GLApply(struct XAVA_HANDLE *xava);
void GLDraw(struct XAVA_HANDLE *xava);
void GLCleanup();

#endif

