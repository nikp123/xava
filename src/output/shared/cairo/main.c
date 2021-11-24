#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "util/feature_compat.h"
#include "util/module.h"
#include "util/region.h"
#include "util/array.h"
#include "../graphical.h"

#define LOAD_FUNC_POINTER(name) \
    module.func.name = xava_module_symbol_address_get(module.handle, "xava_cairo_module_" #name); \
    xavaBailCondition(module.func.name == NULL, "xava_cairo_module_" #name " not found!");

// creates flexible instances, which ARE memory managed and must be free-d after use
xava_cairo_handle *__internal_xava_output_cairo_load_config(
        struct XAVA_HANDLE *xava) {

    XAVACONFIG config = xava->default_config.config;
    xava_cairo_handle *handle;
    MALLOC_SELF(handle, 1);
    handle->xava = xava;

    arr_init(handle->modules);
    handle->events = newXAVAEventStack();

    // loop until all entries have been read
    char key_name[128];
    size_t key_number = 1;
    do {
        snprintf(key_name, 128, "module_%lu", key_number);
        char *module_name = xavaConfigGetString(config, "cairo", key_name, NULL);

        // module invalid, probably means that all desired modules are loaded
        if(module_name == NULL)
            break;

        // add module name
        xava_cairo_module module;
        module.name = module_name;
        module.regions = NULL;

        // the part of the function where module path gets figured out
        do {
            char path[MAX_PATH];
            char *returned_path;

            // path gets reset here
            strcpy(path, "cairo/module/");
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
                &returned_path) == false, "Failed to open cairo module '%s'", path);

            // remove the file extension because of my bad design
            size_t offset=strlen(returned_path)-strlen(xava_module_extension_get());
            returned_path[offset] = '\0';

            module.handle = xava_module_path_load(returned_path);

            xavaBailCondition(!xava_module_valid(module.handle),
                    "Failed to load cairo module '%s' because: '%s'",
                    returned_path, xava_module_error_get(module.handle));
            free(returned_path);
        } while(0);

        // the part of the function where function pointers get loaded per module
        {
            LOAD_FUNC_POINTER(version);
            LOAD_FUNC_POINTER(config_load);
            LOAD_FUNC_POINTER(regions);
            LOAD_FUNC_POINTER(init);
            LOAD_FUNC_POINTER(apply);
            LOAD_FUNC_POINTER(event);
            LOAD_FUNC_POINTER(cleanup);
            LOAD_FUNC_POINTER(ionotify_callback);

            module.config.name   = module.name;
            module.config.xava   = xava;
            module.config.prefix = module.prefix;
            module.config.events = handle->events;
            module.features = module.func.config_load(&module.config);

            if(module.features & XAVA_CAIRO_FEATURE_DRAW_REGION) {
                LOAD_FUNC_POINTER(draw_region);
            }

            if(module.features & XAVA_CAIRO_FEATURE_DRAW_REGION_SAFE) {
                LOAD_FUNC_POINTER(draw_safe);
            }

            if(module.features & XAVA_CAIRO_FEATURE_FULL_DRAW) {
                LOAD_FUNC_POINTER(draw_full);
            }

            // remember to increment the key number
            key_number++;
        }

        // append loaded module to cairo handle
        arr_add(handle->modules, module);
    } while(1);

    return handle;
}

void __internal_xava_output_cairo_init(xava_cairo_handle *handle, cairo_t *cr) {
    // add cairo instance to our handle
    handle->cr   = cr;

    for(size_t i = 0; i < arr_count(handle->modules); i++) {
        handle->modules[i].config.cr = cr;
        handle->modules[i].func.init(&handle->modules[i].config);
    }
}

void __internal_xava_output_cairo_apply(xava_cairo_handle *handle) {
    for(size_t i = 0; i < arr_count(handle->modules); i++) {
        xava_cairo_module        *module = &handle->modules[i];
        struct functions         *f      = &module->func;
        xava_cairo_module_handle *config = &module->config;
        if(module->regions != NULL)
            arr_free(module->regions);

        module->regions = f->regions(config);
    }

    XAVA_CAIRO_FEATURE feature_detected =
        xava_cairo_module_check_compatibility(handle->modules);

    if(feature_detected != handle->feature_level) {
        handle->feature_level = feature_detected;
        for(size_t i = 0; i < arr_count(handle->modules); i++) {
            xava_cairo_module *module = &handle->modules[i];

            // do not overwrite as it's used as a reference
            //module->features = handle->feature_level;
            module->config.use_feature = handle->feature_level;
            module->func.apply(&module->config);
        }
    }
}

XG_EVENT __internal_xava_output_cairo_event(xava_cairo_handle *handle) {
    XG_EVENT event = XAVA_IGNORE;

    // run the module's event handlers
    for(size_t i = 0; i < arr_count(handle->modules); i++) {
        handle->modules[i].func.event(&handle->modules[i].config);
    }

    // process new events (if any)
    while(pendingXAVAEventStack(handle->events)) {
        event = popXAVAEventStack(handle->events);

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

void __internal_xava_output_cairo_draw(xava_cairo_handle *handle) {
    if(handle->feature_level == XAVA_CAIRO_FEATURE_FULL_DRAW) {
        cairo_set_source_rgba(handle->cr,
                ARGB_R_32(handle->xava->conf.bgcol)/255.0,
                ARGB_G_32(handle->xava->conf.bgcol)/255.0,
                ARGB_B_32(handle->xava->conf.bgcol)/255.0,
                handle->xava->conf.background_opacity);
        cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint(handle->cr);
    }

    for(size_t i = 0; i < arr_count(handle->modules); i++) {
        struct functions *f = &handle->modules[i].func;
        xava_cairo_module_handle *config = &handle->modules[i].config;
        switch(handle->feature_level) {
            case XAVA_CAIRO_FEATURE_DRAW_REGION:
                f->draw_region(config);
                break;
            case XAVA_CAIRO_FEATURE_DRAW_REGION_SAFE:
                f->draw_safe(config);
                break;
            case XAVA_CAIRO_FEATURE_FULL_DRAW:
                f->draw_full(config);
                break;
            default:
                xavaBail("Something's definitely broken! REPORT THIS.");
                break;
        }
    }
}

void __internal_xava_output_cairo_clear(xava_cairo_handle *handle) {
    // what is even the point of this function when you're already spendin'
    // precious CPU cycles
    if(handle->feature_level == XAVA_CAIRO_FEATURE_FULL_DRAW)
        return;

    cairo_set_source_rgba(handle->cr,
        ARGB_R_32(handle->xava->conf.bgcol)/255.0,
        ARGB_G_32(handle->xava->conf.bgcol)/255.0,
        ARGB_B_32(handle->xava->conf.bgcol)/255.0,
        handle->xava->conf.background_opacity);
    cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(handle->cr);
}

void __internal_xava_output_cairo_cleanup(xava_cairo_handle *handle) {
    for(size_t i = 0; i < arr_count(handle->modules); i++) {
        handle->modules[i].func.cleanup(&handle->modules[i].config);
    }
    arr_free(handle->modules);
    free(handle);
}

