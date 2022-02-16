#include "shared.h"

#ifndef __XAVA_OUTPUT_GL_MAIN_H
#define __XAVA_OUTPUT_GL_MAIN_H

// for static checker sake
#if !defined(GL) && !defined(EGL)
    #define GL
    #warning "This WILL break your build. Fix it!"
#endif

#if defined(EGL)
    #include <EGL/eglplatform.h>
    #include <EGL/egl.h>
#endif

#ifndef GL_ALREADY_DEFINED
    #include <GL/glew.h>
#endif

typedef struct XAVAGLModuleOptions {
    // actual options used by the module
    GLdouble     resolution_scale;
    XAVA         *xava;
    void         (*ionotify_callback)
                        (XAVA_IONOTIFY_EVENT,
                        const char* filename,
                        int id,
                        XAVA*);
    
    char           *prefix;
    XG_EVENT_STACK *events;
} XAVAGLModuleOptions;

// working around my badly designed includes
typedef struct XAVAGLModule XAVAGLModule;

// shader stuff
typedef struct xava_gl_module_program {
    struct shader {
        char *path, *text;
        GLuint handle;
    } frag, vert, geo;
    GLuint     program;
    xava_config_source config;
} xava_gl_module_program;

// post render stuff
typedef enum gl_module_post_render_features {
    GL_MODULE_POST_INTENSITY = 1,
    GL_MODULE_POST_COLORS    = 2,
    GL_MODULE_POST_TIME      = 4,
} gl_module_post_render_features;

struct XAVAGLModule {
    char               *name;
    char               *prefix;
    XAVAMODULE         *handle;

    struct functions {
        xava_version (*version)(void);
        void         (*config_load)(XAVAGLModule*,XAVA*);
        void         (*init)(XAVAGLModuleOptions*);
        void         (*apply)(XAVAGLModuleOptions*);
        XG_EVENT     (*event)(XAVAGLModuleOptions*);
        void         (*clear)(XAVAGLModuleOptions*);
        void         (*draw)(XAVAGLModuleOptions*);
        void         (*cleanup)(XAVAGLModuleOptions*);
        void         (*ionotify_callback)
                        (XAVA_IONOTIFY_EVENT,
                        const char* filename,
                        int id,
                        XAVA*);
    } func;

    XAVAGLModuleOptions options;
};

typedef struct XAVAGLHostOptions {
    XAVAGLModule         *module;
    GLdouble              resolution_scale;
    XG_EVENT_STACK       *events;
    XAVA                 *xava;

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
        GLuint DOTS;

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

    xava_gl_module_program post;
    gl_module_post_render_features features;
    bool post_enabled;
} XAVAGLHostOptions;

void     SGLConfigLoad(XAVA *xava);
void     SGLInit(XAVA *xava);
void     SGLApply(XAVA *xava);
XG_EVENT SGLEvent(XAVA *xava);
void     SGLClear(XAVA *xava);
void     SGLDraw(XAVA *xava);
void     SGLCleanup(XAVA *xava);

#endif

