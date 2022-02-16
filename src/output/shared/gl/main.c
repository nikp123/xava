#include <stdio.h>
#include <string.h>
#include <math.h>

#include "shared.h"

#include "post.h"
#include "main.h"
#include "shared/module.h"

// eat dick, compiler
#include "post.c"

static XAVAGLHostOptions host;

#define LOAD_FUNC_POINTER(name) \
    module->func.name = xava_module_symbol_address_get(module->handle, "xava_gl_module_" #name); \
    xavaBailCondition(module->func.name == NULL, "xava_gl_module_" #name " not found!");

void SGLConfigLoad(XAVA *xava) {
    xava_config_source config = xava->default_config.config;
    host.resolution_scale =
        xavaConfigGetDouble(config, "gl", "resolution_scale", 1.0f);

    xavaBailCondition(host.resolution_scale <= 0.0f,
        "Resolution scale cannot be under or equal to 0.0");

    arr_init(host.module);

    host.events = newXAVAEventStack();
    host.xava   = xava;

    // loop until all entries have been read
    char key_name[128];
    uint32_t key_number = 1;
    do {
        snprintf(key_name, 128, "module_%u", key_number);
        char *module_name = xavaConfigGetString(config, "gl", key_name, NULL);

        // module invalid, probably means that all desired modules are loaded
        if(module_name == NULL)
            break;

        // add module name
        XAVAGLModule module;
        module.name = module_name;

        // the part of the function where module path gets figured out
        do {
            char path[MAX_PATH];
            char *returned_path;

            // path gets reset here
            strcpy(path, "gl/module/");
            strcat(path, module_name);

            // we need the path without the extension for the folder that's going
            // to store our shaders
            module.prefix = strdup(path);

            strcat(path, "/module");

            // prefer local version (ie. build-folder)
            module.handle = xava_module_path_load(path);
            if(xava_module_valid(module.handle))
                break;

            xavaLog("Failed to load '%s' because: '%s'",
                    path, xava_module_error_get(module.handle));

            strcat(path, xava_module_extension_get());
            xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_PACKAGE, path,
                &returned_path) == false, "Failed to open gl module '%s'", path);

            // remove the file extension because of my bad design
            size_t offset=strlen(returned_path)-strlen(xava_module_extension_get());
            returned_path[offset] = '\0';

            module.handle = xava_module_path_load(returned_path);

            xavaBailCondition(!xava_module_valid(module.handle),
                    "Failed to load gl module '%s' because: '%s'",
                    returned_path, xava_module_error_get(module.handle));
            free(returned_path);
        } while(0);

        // append loaded module to cairo handle
        arr_add(host.module, module);

        // the part of the function where function pointers get loaded per module
        {
            XAVAGLModule *module =
                &host.module[arr_count(host.module)-1];

            LOAD_FUNC_POINTER(version);
            LOAD_FUNC_POINTER(config_load);
            LOAD_FUNC_POINTER(ionotify_callback);
            LOAD_FUNC_POINTER(init);
            LOAD_FUNC_POINTER(apply);
            LOAD_FUNC_POINTER(event);
            LOAD_FUNC_POINTER(draw);
            LOAD_FUNC_POINTER(clear);
            LOAD_FUNC_POINTER(cleanup);

            module->options.xava             = xava;
            module->options.resolution_scale = host.resolution_scale;
            module->options.prefix           = module->prefix;
            module->options.events           = host.events;

            // remember to increment the key number
            key_number++;

            // check if the module is of a appropriate version
            xava_version_verify(module->func.version());

            // load it's config
            module->func.config_load(module, xava);
        }
    } while(1);

    xava_gl_module_post_config_load(&host);
}

void SGLInit(XAVA *xava) {
    for(int i = 0; i < arr_count(host.module); i++) {
        XAVAGLModule *module = &host.module[i];

        module->options.xava = xava;
        module->func.init(&module->options);
    }

    xava_gl_module_post_init(&host);
}

void SGLApply(XAVA *xava){
    for(int i = 0; i < arr_count(host.module); i++) {
        XAVAGLModule *module = &host.module[i];

        module->options.xava = xava;
        module->func.apply(&module->options);
    }

    xava_gl_module_post_apply(&host);
}

XG_EVENT SGLEvent(XAVA *xava) {
    XG_EVENT event = XAVA_IGNORE;
    for(int i = 0; i < arr_count(host.module); i++) {
        XAVAGLModule *module = &host.module[i];

        module->options.xava = xava;
        module->func.event(&module->options);
    }

    // process new events (if any)
    while(pendingXAVAEventStack(host.events)) {
        event = popXAVAEventStack(host.events);

        switch(event) {
            case XAVA_RESIZE:
                return XAVA_RESIZE;
            case XAVA_QUIT:
                return XAVA_QUIT;
            default:
                break;
        }
    }

    return event;
}

void SGLClear(XAVA *xava) {
    for(int i = 0; i < arr_count(host.module); i++) {
        XAVAGLModule *module = &host.module[i];

        module->options.xava = xava;
        module->func.clear(&module->options);
    }
}

void SGLDraw(XAVA *xava) {
    // FIXME
    // enable blending temporary so that the colors get properly calculated on
    // the shader end of the pre stage
    //if(gl_options.color.blending) {
        glEnable(GL_BLEND);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE, GL_ZERO);
    //}

    // bind render target to texture
    xava_gl_module_post_pre_draw_setup(&host);

    // clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    for(int i = 0; i < arr_count(host.module); i++) {
        XAVAGLModule *module = &host.module[i];

        module->options.xava = xava;
        module->func.draw(&module->options);
    }

    // get out the draw function if post shaders aren't enabled
    xava_gl_module_post_draw(&host);
}

void SGLCleanup(XAVA *xava) {
    for(int i = 0; i < arr_count(host.module); i++) {
        XAVAGLModule *module = &host.module[i];

        module->options.xava = xava;
        module->func.cleanup(&module->options);

        xava_module_free(module->handle);
        //free(module->name);
        free(module->prefix);
    }

    xava_gl_module_post_cleanup(&host);

    arr_free(host.module);
    destroyXAVAEventStack(host.events);
}

