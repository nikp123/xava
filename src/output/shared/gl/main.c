#include <stdio.h>
#include <string.h>
#include <math.h>

#include "shared.h"

#include "post.h"
#include "main.h"

// eat dick, compiler
#include "post.c"

// TODO: NUKE THIS
static XAVAGLHostOptions host;

XAVAGLModule SGLModuleGetHandle(char *module_name) {
    // add module name
    XAVAGLModule module;
    module.name = module_name;

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
        return module;

    xavaLog("Failed to load '%s' because: '%s'",
            path, xava_module_error_get(module.handle));

    strcat(path, xava_module_extension_get());
    returned_path = xavaFindAndCheckFile(XAVA_FILE_TYPE_PACKAGE, path);
    xavaBailCondition(returned_path == NULL, "Failed to open gl module '%s'", path);

    // remove the file extension because of my bad design
    size_t offset=strlen(returned_path)-strlen(xava_module_extension_get());
    returned_path[offset] = '\0';

    module.handle = xava_module_path_load(returned_path);

    xavaBailCondition(!xava_module_valid(module.handle),
            "Failed to load gl module '%s' because: '%s'",
            returned_path, xava_module_error_get(module.handle));
    free(returned_path);

    return module;
}

#define LOAD_FUNC_POINTER(name) { \
    module->func.name = xava_module_symbol_address_get(module->handle, "xava_gl_module_" #name); \
    xavaBailCondition(module->func.name == NULL, "xava_gl_module_" #name " not found!"); \
}

void SGLModuleAppendToSystem(XAVA *xava, XAVAGLModule *module, XAVAGLHostOptions *host) {
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
    module->options.resolution_scale = host->resolution_scale;
    module->options.prefix           = module->prefix;
    module->options.events           = host->events;

    // check if the module is of a appropriate version
    xava_version_verify(module->func.version());

    // load it's config
    module->func.config_load(module, xava);
}

void SGLModuleLoad(XAVA *xava, XAVAGLHostOptions *host, char *known_key_name, uint32_t key_number) {
    xava_config_source config = xava->default_config.config;
    char key_name[128];

    // preprocessor macro fuckery (should've been rust)
    XAVA_CONFIG_OPTION(char*, module_name);

    if(known_key_name == NULL) {
        snprintf(key_name, 128, "module_%u", key_number);
        XAVA_CONFIG_GET_STRING(config, "gl", key_name, NULL, module_name);

        // module invalid, probably means that all desired modules are loaded
        if(!module_name_is_set_from_file) {
            xavaLog("'%s' has not been set properly!", module_name);
            return;
        }
    } else {
        module_name = known_key_name;
    }

    // This is where the path gets processed and the module handle figured out
    XAVAGLModule module = SGLModuleGetHandle(module_name);

    // the part of the function where function pointers get loaded per module
    SGLModuleAppendToSystem(xava, &module, host);

    // append loaded module to GL handle
    arr_add(host->module, module);
}

void SGLConfigLoad(XAVA *xava) {
    xava_config_source config = xava->default_config.config;
    XAVA_CONFIG_GET_F64(config, "gl", "resolution_scale", 1.0f, (&host)->resolution_scale);

    xavaBailCondition(host.resolution_scale <= 0.0f,
        "Resolution scale cannot be under or equal to 0.0");

    arr_init(host.module);

    host.events = newXAVAEventStack();
    host.xava   = xava;

    //
    // module code goes here
    //

    // Load each individual module (try up to 128, idfk)
    // Fix this if possible
    for(uint8_t i = 1; i < 128; i++) {
        SGLModuleLoad(xava, &host, NULL, i);
    }

    if(arr_count(host.module) == 0) {
       xavaWarn("You have NO GL output modules loaded.\n"
           "Refusing to run like that. Loading default 'bars' module instead");
       SGLModuleLoad(xava, &host, "bars", 1);
    }

    xava_gl_module_post_config_load(&host);
}

void SGLInit(XAVA *xava) {
    for(uint32_t i = 0; i < arr_count(host.module); i++) {
        XAVAGLModule *module = &host.module[i];

        module->options.xava = xava;
        module->func.init(&module->options);
    }

    xava_gl_module_post_init(&host);
}

void SGLApply(XAVA *xava){
    for(uint32_t i = 0; i < arr_count(host.module); i++) {
        XAVAGLModule *module = &host.module[i];

        module->options.xava = xava;
        module->func.apply(&module->options);
    }

    xava_gl_module_post_apply(&host);
}

XG_EVENT *SGLEvent(XAVA *xava) {
    for(uint32_t i = 0; i < arr_count(host.module); i++) {
        XAVAGLModule *module = &host.module[i];

        module->options.xava = xava;
        module->func.event(&module->options);
    }

    return host.events;
}

void SGLClear(XAVA *xava) {
    for(uint32_t i = 0; i < arr_count(host.module); i++) {
        XAVAGLModule *module = &host.module[i];

        module->options.xava = xava;
        module->func.clear(&module->options);
    }
}

void SGLDraw(XAVA *xava) {
    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_SRC_ALPHA, GL_DST_ALPHA);

    // bind render target to texture
    xava_gl_module_post_pre_draw_setup(&host);

    // clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    for(uint32_t i = 0; i < arr_count(host.module); i++) {
        XAVAGLModule *module = &host.module[i];

        module->options.xava = xava;
        module->func.draw(&module->options);
    }

    // get out the draw function if post shaders aren't enabled
    xava_gl_module_post_draw(&host);
}

void SGLCleanup(XAVA *xava) {
    for(uint32_t i = 0; i < arr_count(host.module); i++) {
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

