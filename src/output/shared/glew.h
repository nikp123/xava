#ifndef __GL_H
#define __GL_H

#include "gl_shared.h"

#include "../../shared.h"


void     GLConfigLoad(struct XAVA_HANDLE *xava);
void     GLInit(struct XAVA_HANDLE *xava);
void     GLClear(struct XAVA_HANDLE *xava);
void     GLApply(struct XAVA_HANDLE *xava);
XG_EVENT GLEvent(struct XAVA_HANDLE *xava);
void     GLDraw(struct XAVA_HANDLE *xava);
void     GLCleanup(struct XAVA_HANDLE *xava);

#endif

