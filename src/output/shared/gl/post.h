#ifndef __GL_MODULE_SHARED_POST_H
#define __GL_MODULE_SHARED_POST_H

#include "output/shared/gl/main.h"

void xava_gl_module_post_config_load(XAVAGLHostOptions *vars);
void xava_gl_module_post_init(XAVAGLHostOptions *vars);
void xava_gl_module_post_apply(XAVAGLHostOptions *vars);
void xava_gl_module_post_clear(XAVAGLHostOptions *vars);
void xava_gl_module_post_pre_draw_setup(XAVAGLHostOptions *vars);
void xava_gl_module_post_draw(XAVAGLHostOptions *vars);
void xava_gl_module_post_cleanup(XAVAGLHostOptions *vars);
#endif

