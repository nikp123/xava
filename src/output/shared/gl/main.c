#include <stdio.h>
#include <string.h>
#include <math.h>

#include "shared.h"

#include "main.h"

static XAVAGLModuleOptions module_options;
static XAVAGLHostOptions   options;

#define LOAD_FUNC_POINTER(name) \
    options.func.name = xava_module_symbol_address_get(options.module_handle, "xava_gl_module_" #name); \
    xavaBailCondition(options.func.name == NULL, "xava_gl_module_" #name " not found!");

void SGLConfigLoad(XAVA *xava) {
    xava_config_source config = xava->default_config.config;
    module_options.resolution_scale =
        xavaConfigGetDouble(config, "gl", "resolution_scale", 1.0f);

    xavaBailCondition(module_options.resolution_scale <= 0.0f,
        "Resolution scale cannot be under or equal to 0.0");

    options.module_name = xavaConfigGetString(config, "gl", "module", "bars");

    char *returned_path;
    do {
        char path[MAX_PATH];

        // path gets reset here
        strcpy(path, "gl/module/");
        strcat(path, options.module_name);
 
        // we need the path without the extension for the folder that's going
        // to store our shaders
        module_options.module_prefix = strdup(path);

        strcat(path, "/module");

        // prefer local version (ie. build-folder)
        options.module_handle = xava_module_path_load(path);
        if(xava_module_valid(options.module_handle))
            break;

        xavaLog("Failed to load '%s' because: '%s'",
                path, xava_module_error_get(options.module_handle));

        strcat(path, xava_module_extension_get());
        xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_PACKAGE, path, 
            &returned_path) == false, "Failed to open GL module '%s'", path);

        // remove the file extension because of my bad design
        size_t offset=strlen(returned_path)-strlen(xava_module_extension_get());
        returned_path[offset] = '\0';

        options.module_handle = xava_module_path_load(returned_path);

        xavaBailCondition(!xava_module_valid(options.module_handle),
                "Failed to load GL module '%s' because: '%s'",
                returned_path, xava_module_error_get(options.module_handle));
        free(returned_path);
    } while(0);

    LOAD_FUNC_POINTER(version);
    LOAD_FUNC_POINTER(config_load);
    LOAD_FUNC_POINTER(init);
    LOAD_FUNC_POINTER(apply);
    LOAD_FUNC_POINTER(clear);
    LOAD_FUNC_POINTER(event);
    LOAD_FUNC_POINTER(draw);
    LOAD_FUNC_POINTER(cleanup);
    LOAD_FUNC_POINTER(ionotify_callback);

    module_options.xava = xava;
    module_options.ionotify_callback = options.func.ionotify_callback;

    // check if the module is of a appropriate version
    xava_version_verify(options.func.version());

    // load it's config
    options.func.config_load(&module_options);
}

void SGLInit(XAVA *xava) {
    module_options.xava = xava;
    options.func.init(&module_options);
}

void SGLApply(XAVA *xava){
    module_options.xava = xava;
    options.func.apply(&module_options);
}

XG_EVENT SGLEvent(XAVA *xava) {
    module_options.xava = xava;
    return options.func.event(&module_options);
}

void SGLClear(XAVA *xava) {
    module_options.xava = xava;
    options.func.clear(&module_options);
}

void SGLDraw(XAVA *xava) {
    module_options.xava = xava;
    options.func.draw(&module_options);
}

void SGLCleanup(XAVA *xava) {
    module_options.xava = xava;
    options.func.cleanup(&module_options);

    options.func.apply = NULL;
    options.func.cleanup = NULL;
    options.func.clear = NULL;
    options.func.config_load = NULL;
    options.func.draw = NULL;
    options.func.event = NULL;
    options.func.init = NULL;
    options.func.version = NULL;
    options.func.ionotify_callback = NULL;

    free(module_options.module_prefix);
    xava_module_free(options.module_handle);
}

