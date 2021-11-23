#ifndef __XAVA_OUTPUT_SHARED_CAIRO_UTIL_MODULE_H
#define __XAVA_OUTPUT_SHARED_CAIRO_UTIL_MODULE_H 

#include <stdbool.h>
#include <cairo/cairo.h>
#include "../../../../shared.h"
#include "feature_compat.h"
#include "region.h"

typedef struct xava_cairo_module_handle {
    XAVA_CAIRO_FEATURE use_feature;
    char               *name;
    char               *prefix;
    struct XAVA_HANDLE *xava;
    cairo_t            *cr;
} xava_cairo_module_handle;

typedef struct xava_cairo_module {
    char *name;
    char *prefix;
    XAVA_CAIRO_FEATURE features;
    xava_cairo_region  *regions;

    xava_cairo_module_handle config;

    struct functions {
        // report version
        xava_version       (*version)    (void);

        // load all the necessary config data and report supported drawing modes
        XAVA_CAIRO_FEATURE (*config_load)(xava_cairo_module_handle*);
        void               (*init)       (xava_cairo_module_handle*);
        void               (*apply)      (xava_cairo_module_handle*);

        // report drawn regions
        xava_cairo_region* (*regions)    (xava_cairo_module_handle*);

        // event handler
        XG_EVENT           (*event)      (xava_cairo_module_handle*);
 
        // only used with draw_region
        void               (*clear)      (xava_cairo_module_handle*);
        void               (*draw_region)(xava_cairo_module_handle*);

        // no matter what condition, this ensures a safe write
        void               (*draw_safe)  (xava_cairo_module_handle*);

        // assume that the entire screen's being overwritten
        void               (*draw_full)  (xava_cairo_module_handle*);

        void               (*cleanup)    (xava_cairo_module_handle*);

        // ionotify fun
        void         (*ionotify_callback)
                        (XAVA_IONOTIFY_EVENT,
                        const char* filename,
                        int id,
                        struct XAVA_HANDLE*);
    } func;

    // module handle for fun
    XAVAMODULE *handle;
} xava_cairo_module;

XAVA_CAIRO_FEATURE xava_cairo_module_check_compatibility(xava_cairo_module *modules);
#endif // __XAVA_OUTPUT_SHARED_CAIRO_UTIL_MODULE_H

