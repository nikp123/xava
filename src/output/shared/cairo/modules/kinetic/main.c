#include <math.h>
#include <cairo/cairo.h>
#include "../../util/module.h"
#include "../../util/array.h"
#include "../../../graphical.h"
#include "../../../../../shared.h"

// report version
EXP_FUNC xava_version xava_cairo_module_version(void) {
    return xava_version_host_get();
}

// load all the necessary config data and report supported drawing modes
EXP_FUNC XAVA_CAIRO_FEATURE xava_cairo_module_config_load(xava_cairo_module_handle* handle) {
    return XAVA_CAIRO_FEATURE_FULL_DRAW;
}

EXP_FUNC void               xava_cairo_module_init(xava_cairo_module_handle* handle) {
}

EXP_FUNC void               xava_cairo_module_apply(xava_cairo_module_handle* handle) {
}

// report drawn regions
EXP_FUNC xava_cairo_region* xava_cairo_module_regions(xava_cairo_module_handle* handle) {
    return NULL;
}

// event handler
EXP_FUNC void               xava_cairo_module_event      (xava_cairo_module_handle* handle) {
    XAVA *xava = handle->xava;

    // check if the visualizer bounds were changed
    if((xava->inner.w != xava->bar_space.w) ||
       (xava->inner.h != xava->bar_space.h)) {
        xava->bar_space.w = xava->inner.w;
        xava->bar_space.h = xava->inner.h;
        pushXAVAEventStack(handle->events, XAVA_RESIZE);
    }
}

// placeholder, as it literally does nothing atm
EXP_FUNC void               xava_cairo_module_clear      (xava_cairo_module_handle* handle) {
}

EXP_FUNC void               xava_cairo_module_draw_region(xava_cairo_module_handle* handle) {
}

// no matter what condition, this ensures a safe write
EXP_FUNC void               xava_cairo_module_draw_safe  (xava_cairo_module_handle* handle) {
}

// assume that the entire screen's being overwritten
EXP_FUNC void               xava_cairo_module_draw_full  (xava_cairo_module_handle* handle) {
    XAVA   *xava = handle->xava;
    XAVA_CONFIG *conf = &xava->conf;

    cairo_set_source_rgba(handle->cr,
            ARGB_R_32(conf->col)/255.0,
            ARGB_G_32(conf->col)/255.0,
            ARGB_B_32(conf->col)/255.0,
            conf->foreground_opacity);

    float intensity = 0.0;

    for(register int i=0; i<xava->bars; i++) {
        // the not so, speed part
        // intensity has a low-freq bias as they are more "physical"
        float bar_percentage = (float)(xava->f[i]-1)/(float)conf->h;
        if(bar_percentage > 0.0) {
            intensity+=powf(bar_percentage,
                    (float)2.0*(float)i/(float)xava->bars);
        }
    }

    // since im not bothering to do the math, this'll do
    // - used to balance out intensity across various number of bars
    intensity /= xava->bars;

    cairo_set_source_rgba(handle->cr,
            ARGB_R_32(conf->bgcol)/255.0,
            ARGB_G_32(conf->bgcol)/255.0,
            ARGB_B_32(conf->bgcol)/255.0,
            conf->background_opacity*(1.0-intensity));
    cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(handle->cr);
}

EXP_FUNC void               xava_cairo_module_cleanup    (xava_cairo_module_handle* handle) {
}

// ionotify fun
EXP_FUNC void         xava_cairo_module_ionotify_callback
                (XAVA_IONOTIFY_EVENT event,
                const char* filename,
                int id,
                XAVA* xava) {
}
