#ifndef __XAVA_OUTPUT_SHARED_CAIRO_H
#define __XAVA_OUTPUT_SHARED_CAIRO_H

#include <cairo/cairo.h>
#include "../../../shared.h"
#include "util/feature_compat.h"
#include "util/module.h"

typedef struct xava_cairo_handle {
    struct XAVA_HANDLE *xava;
    cairo_t            *cr; // name used by a lot of docs, so I'm going with it

    XG_EVENT_STACK     *events;

    xava_cairo_module  *modules;
    XAVA_CAIRO_FEATURE  feature_level;
} xava_cairo_handle;

xava_cairo_handle *__internal_xava_output_cairo_load_config(
                                        struct XAVA_HANDLE *xava);
void               __internal_xava_output_cairo_init(xava_cairo_handle *handle,
                                                     cairo_t *cr);
void              __internal_xava_output_cairo_apply(xava_cairo_handle *handle);
XG_EVENT          __internal_xava_output_cairo_event(xava_cairo_handle *handle);
void              __internal_xava_output_cairo_draw(xava_cairo_handle *handle);
void              __internal_xava_output_cairo_clear(xava_cairo_handle *handle);
void              __internal_xava_output_cairo_cleanup(xava_cairo_handle *handle);

#endif //__XAVA_OUTPUT_SHARED_CAIRO_H

