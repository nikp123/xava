#ifndef __GL_H
#define __GL_H

#include "main.h"

#include "shared.h"

void GLConfigLoad(XAVA *xava);
void GLInit(XAVA *xava);
void GLClear(XAVA *xava);
void GLApply(XAVA *xava);
XG_EVENT *GLEvent(XAVA *xava);
void GLDraw(XAVA *xava);
void GLCleanup(XAVA *xava);

#endif
