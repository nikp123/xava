#ifndef __GL_MODULE_SHARED_UTIl_H
#define __GL_MODULE_SHARED_UTIl_H

#include "output/shared/gl/main.h"

typedef struct gl4f {
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat a;
} gl4f;

float xava_gl_module_util_calculate_intensity(XAVA *xava);
float xava_gl_module_util_obtain_time(void);
gl4f mix(gl4f a, gl4f b, float ratio);
#endif

