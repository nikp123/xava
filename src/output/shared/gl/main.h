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

#include "../../../shared.h"

typedef struct XAVAGLModuleOptions {
    GLdouble            resolution_scale;
    char               *module_prefix;
    struct XAVA_HANDLE *xava;
    void                (*ionotify_callback)(XAVA_IONOTIFY_EVENT,
                                const char* filename, int id,
                                struct XAVA_HANDLE*);
} XAVAGLModuleOptions;

typedef struct XAVAGLHostOptions {
    char       *module_name;
    XAVAMODULE *module_handle;

    struct functions {
        xava_version (*version)(void);
        void         (*config_load)(XAVAGLModuleOptions*);
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
                        struct XAVA_HANDLE*);
    } func;
} XAVAGLHostOptions;

void     SGLConfigLoad(struct XAVA_HANDLE *xava);
void     SGLInit(struct XAVA_HANDLE *xava);
void     SGLApply(struct XAVA_HANDLE *xava);
XG_EVENT SGLEvent(struct XAVA_HANDLE *xava);
void     SGLClear(struct XAVA_HANDLE *xava);
void     SGLDraw(struct XAVA_HANDLE *xava);
void     SGLCleanup(struct XAVA_HANDLE *xava);

#endif
