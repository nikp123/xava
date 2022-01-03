#ifndef __GL_MODULE_SHARED_SHADER_H
#define __GL_MODULE_SHARED_SHADER_H

#include "output/shared/gl/main.h"

typedef enum sgl_shader_type {
    SGL_PRE,
    SGL_POST
} sgl_shader_type;

typedef enum sgl_shader_stage {
    SGL_VERT,
    SGL_GEO,
    SGL_FRAG,
    SGL_CONFIG
} sgl_shader_stage;

typedef struct xava_gl_module_program {
    struct shader {
        char *path, *text;
        GLuint handle;
    } frag, vert, geo;
    GLuint     program;
    xava_config_source config;
} xava_gl_module_program;

GLint xava_gl_module_shader_build(struct shader *shader, GLenum shader_type);
void xava_gl_module_program_create(xava_gl_module_program *program);
void xava_gl_module_program_destroy(xava_gl_module_program *program);
void xava_gl_module_shader_load(xava_gl_module_program *program,
        sgl_shader_type type, sgl_shader_stage stage,
        const char *name, XAVAGLModuleOptions *options);
#endif

