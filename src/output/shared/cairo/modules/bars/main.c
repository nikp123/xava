#include <cairo/cairo.h>
#include "../../util/module.h"
#include "../../../../graphical.h"
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
EXP_FUNC XG_EVENT           xava_cairo_module_event      (xava_cairo_module_handle* handle) {
    return XAVA_IGNORE;
}

// only used with draw_region
EXP_FUNC void               xava_cairo_module_clear      (xava_cairo_module_handle* handle) {
}

EXP_FUNC void               xava_cairo_module_draw_region(xava_cairo_module_handle* handle) {
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
        int x = xava->rest + i*(conf->bs+conf->bw)+xava->x;
        int y = xava->h - xava->f[i]+xava->y;
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
