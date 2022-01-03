#include <math.h>

#ifndef GL
    #define GL
#endif
#include "glew.h"

#include "shared.h"
#include "output/shared/graphical.h"


void GLConfigLoad(XAVA *xava) {
    SGLConfigLoad(xava);
}

void GLInit(XAVA *xava) {
    glewInit();

    SGLInit(xava);
}

void GLApply(XAVA *xava) {
    SGLApply(xava);
}

XG_EVENT GLEvent(XAVA *xava) {
    return SGLEvent(xava);
}

void GLClear(XAVA *xava) {
    SGLClear(xava);
}

void GLDraw(XAVA *xava) {
    SGLDraw(xava);
}

void GLCleanup(XAVA *xava) {
    SGLCleanup(xava);
}

