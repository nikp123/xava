#ifndef __GL_MODULE_SHARED_POST_H
#define __GL_MODULE_SHARED_POST_H

#include "../../gl_shared.h"
#include "shader.h"

typedef enum gl_module_post_render_features {
    GL_MODULE_POST_INTENSITY = 1,
    GL_MODULE_POST_COLORS    = 2,
    GL_MODULE_POST_TIME      = 4,
} gl_module_post_render_features;

typedef struct gl_module_post_render {
    struct FBO {
        GLuint framebuffer;
        GLuint final_texture;
        GLuint depth_texture;
    } FBO;

    // post shader variables
    struct gl_vars {
        // geometry info
        GLuint POS;
        GLuint TEXCOORD;
        GLuint RESOLUTION;

        // textures
        GLuint TEXTURE;
        GLuint DEPTH;

        // system info
        GLuint TIME;
        GLuint INTENSITY;

        // color information
        GLuint FGCOL;
        GLuint BGCOL;
    } gl_vars;

    XAVAGLModuleOptions *options;

    xava_gl_module_program post;

    gl_module_post_render_features features;

    bool enabled;
} gl_module_post_render;

void xava_gl_module_post_config_load(gl_module_post_render *vars);
void xava_gl_module_post_init(gl_module_post_render *vars);
void xava_gl_module_post_apply(gl_module_post_render *vars);
void xava_gl_module_post_clear(gl_module_post_render *vars);
void xava_gl_module_post_pre_draw_setup(gl_module_post_render *vars);
void xava_gl_module_post_draw(gl_module_post_render *vars);
void xava_gl_module_post_cleanup(gl_module_post_render *vars);
#endif

