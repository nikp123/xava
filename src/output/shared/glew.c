#include <math.h>

#ifndef GL
	#define GL
#endif
#include "glew.h"

#include "../../shared.h"
#include "../graphical.h"


void GLConfigLoad(struct XAVA_HANDLE *xava) {
	SGLConfigLoad(xava);
}

void GLInit(struct XAVA_HANDLE *xava) {
	glewInit();

	SGLInit(xava);
}

void GLApply(struct XAVA_HANDLE *xava) {
	SGLApply(xava);
}

XG_EVENT GLEvent(struct XAVA_HANDLE *xava) {
	return SGLEvent(xava);
}

void GLClear(struct XAVA_HANDLE *xava) {
	SGLClear(xava);
}

void GLDraw(struct XAVA_HANDLE *xava) {
	SGLDraw(xava);
}

void GLCleanup(struct XAVA_HANDLE *xava) {
	SGLCleanup(xava);
}

