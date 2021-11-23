#include <cairo/cairo.h>
#include "../../util/module.h"
#include "../../util/array.h"
#include "../../../../graphical.h"
#include "../../../../../shared.h"

// report version
EXP_FUNC xava_version xava_cairo_module_version(void) {
    return xava_version_host_get();
}

// load all the necessary config data and report supported drawing modes
EXP_FUNC XAVA_CAIRO_FEATURE xava_cairo_module_config_load(xava_cairo_module_handle* handle) {
    return XAVA_CAIRO_FEATURE_FULL_DRAW |
        XAVA_CAIRO_FEATURE_DRAW_REGION;
}

EXP_FUNC void               xava_cairo_module_init(xava_cairo_module_handle* handle) {

}
EXP_FUNC void               xava_cairo_module_apply(xava_cairo_module_handle* handle) {

}

// report drawn regions
EXP_FUNC xava_cairo_region* xava_cairo_module_regions(xava_cairo_module_handle* handle) {
    struct XAVA_HANDLE *xava = handle->xava;
    struct config_params *conf = &xava->conf;

    xava_cairo_region *regions;
    arr_init(regions);

    xava_cairo_region region;
    for(size_t i = 0; i < xava->bars; i++) {
        region.x = xava->rest + i*(conf->bs+conf->bw)+xava->inner.x;
        region.y =                                    xava->inner.y;
        region.w = xava->conf.bw;
        region.h = xava->inner.h;
        arr_add(regions, region);
    }

    return regions;
}

// event handler
EXP_FUNC XG_EVENT           xava_cairo_module_event      (xava_cairo_module_handle* handle) {
    return XAVA_IGNORE;
}

// placeholder, as it literally does nothing atm
EXP_FUNC void               xava_cairo_module_clear      (xava_cairo_module_handle* handle) {
}

EXP_FUNC void               xava_cairo_module_draw_region(xava_cairo_module_handle* handle) {
    struct XAVA_HANDLE   *xava = handle->xava;
    struct config_params *conf = &xava->conf;

    cairo_new_path(handle->cr);

    struct color {
        float r, g, b, a;
    } fg, bg;

    fg.r = ARGB_R_32(conf->col)/255.0;
    fg.g = ARGB_G_32(conf->col)/255.0;
    fg.b = ARGB_B_32(conf->col)/255.0;
    fg.a = conf->foreground_opacity;

    bg.r = ARGB_R_32(conf->bgcol)/255.0;
    bg.g = ARGB_G_32(conf->bgcol)/255.0;
    bg.b = ARGB_B_32(conf->bgcol)/255.0;
    bg.a = conf->background_opacity;

    for(size_t i = 0; i < xava->bars; i++) {
        if(xava->f[i] > xava->fl[i]) {
            cairo_set_source_rgba(handle->cr, fg.r, fg.g, fg.b, fg.a);
            cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
            int x = xava->rest    + i*(conf->bs+conf->bw) + xava->inner.x;
            int y = xava->inner.h - xava->f[i]            + xava->inner.y;
            int h = xava->f[i]    - xava->fl[i];
            cairo_rectangle(handle->cr, x, y, xava->conf.bw, h);
            cairo_fill(handle->cr);
        } else {
            cairo_set_source_rgba(handle->cr, bg.r, bg.g, bg.b, bg.a);
            cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
            int x = xava->rest    + i*(conf->bs+conf->bw) + xava->inner.x;
            int y = xava->inner.h - xava->fl[i]           + xava->inner.y;
            int h = xava->fl[i]   - xava->f[i];
            cairo_rectangle(handle->cr, x, y, xava->conf.bw, h);
            cairo_fill(handle->cr);
        }
    }

    cairo_save(handle->cr);
}

// no matter what condition, this ensures a safe write
EXP_FUNC void               xava_cairo_module_draw_safe  (xava_cairo_module_handle* handle) {
}

// assume that the entire screen's being overwritten
EXP_FUNC void               xava_cairo_module_draw_full  (xava_cairo_module_handle* handle) {
    struct XAVA_HANDLE   *xava = handle->xava;
    struct config_params *conf = &xava->conf;

    cairo_new_path(handle->cr);
    cairo_set_source_rgba(handle->cr, 
            ARGB_R_32(conf->col)/255.0,
            ARGB_G_32(conf->col)/255.0,
            ARGB_B_32(conf->col)/255.0,
            conf->foreground_opacity);
    for(size_t i = 0; i < xava->bars; i++) {
        int x = xava->rest + i*(conf->bs+conf->bw)+xava->inner.x;
        int y = xava->inner.h - xava->f[i]        +xava->inner.y;
        cairo_rectangle(handle->cr, x, y, xava->conf.bw, xava->f[i]);
    }

    cairo_fill(handle->cr);
    cairo_save(handle->cr);
}

EXP_FUNC void               xava_cairo_module_cleanup    (xava_cairo_module_handle* handle) {
}

// ionotify fun
EXP_FUNC void         xava_cairo_module_ionotify_callback
                (XAVA_IONOTIFY_EVENT event,
                const char* filename,
                int id,
                struct XAVA_HANDLE* xava) {
}
