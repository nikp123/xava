#include <math.h>
#include <cairo.h>

#include "output/shared/cairo/modules/shared/config.h"

#include "output/shared/cairo/util/module.h"
#include "output/shared/graphical.h"
#include "shared.h"

struct options {
    bool mirror;
} options;

xava_config_source *config_file;
xava_ionotify       file_notifications;
char config_file_path[MAX_PATH];

// ionotify fun
EXP_FUNC void xava_cairo_module_ionotify_callback
                (xava_ionotify_event event,
                const char* filename,
                int id,
                void* xava) {
    UNUSED(filename);
    UNUSED(id);

    xava_cairo_module_handle *handle = (xava_cairo_module_handle*)xava;
    switch(event) {
        case XAVA_IONOTIFY_CHANGED:
            // trigger restart
            pushXAVAEventStack(handle->events, XAVA_RELOAD);
            break;
        default:
            // noop
            break;
    }
}

// report version
EXP_FUNC xava_version xava_cairo_module_version(void) {
    return xava_version_host_get();
}

// load all the necessary config data and report supported drawing modes
EXP_FUNC XAVA_CAIRO_FEATURE xava_cairo_module_config_load(xava_cairo_module_handle* handle) {
    config_file = xava_cairo_module_file_load(
            XAVA_CAIRO_FILE_CONFIG, handle, "config.ini", config_file_path);

    options.mirror = xavaConfigGetBool(*config_file, "bars", "mirror", false);

    return XAVA_CAIRO_FEATURE_FULL_DRAW |
        XAVA_CAIRO_FEATURE_DRAW_REGION;
}

EXP_FUNC void               xava_cairo_module_init(xava_cairo_module_handle* handle) {
    // setup file notifications
    file_notifications = xavaIONotifySetup();

    xava_ionotify_watch_setup setup;
    setup.filename           = config_file_path;
    setup.id                 = 1;
    setup.xava_ionotify_func = &xava_cairo_module_ionotify_callback;
    setup.global             = handle;
    setup.ionotify           = file_notifications;
    xavaIONotifyAddWatch(setup);

    xavaIONotifyStart(file_notifications);

}
EXP_FUNC void               xava_cairo_module_apply(xava_cairo_module_handle* handle) {
    UNUSED(handle);
}

// report drawn regions
EXP_FUNC xava_cairo_region* xava_cairo_module_regions(xava_cairo_module_handle* handle) {
    XAVA *xava = handle->xava;
    XAVA_CONFIG *conf = &xava->conf;

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
    UNUSED(handle);
}

EXP_FUNC void               xava_cairo_module_draw_region(xava_cairo_module_handle* handle) {
    XAVA   *xava = handle->xava;
    XAVA_CONFIG *conf = &xava->conf;

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

    if(options.mirror) {
        for(size_t i = 0; i < xava->bars; i++) {
            if(xava->f[i] > xava->fl[i]) {
                cairo_set_source_rgba(handle->cr, fg.r, fg.g, fg.b, fg.a);
                cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
                int x = xava->rest    + i*(conf->bs+conf->bw) + xava->inner.x;
                int y = ((xava->inner.h - xava->f[i])>>1)     + xava->inner.y;
                int h = xava->f[i]    - xava->fl[i];
                cairo_rectangle(handle->cr, x, y, xava->conf.bw, (h>>1) + 1);

                y = ((xava->inner.h + xava->fl[i])>>1)    + xava->inner.y - 1;
                cairo_rectangle(handle->cr, x, y, xava->conf.bw, (h>>1) + 1);
                cairo_fill(handle->cr);
            } else {
                cairo_set_source_rgba(handle->cr, bg.r, bg.g, bg.b, bg.a);
                cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
                int x = xava->rest    + i*(conf->bs+conf->bw) + xava->inner.x;
                int y = ((xava->inner.h - xava->fl[i])>>1)    + xava->inner.y - 1;
                int h = xava->fl[i]   - xava->f[i];
                cairo_rectangle(handle->cr, x, y, xava->conf.bw, (h>>1) + 1);

                y = ((xava->inner.h + xava->f[i])>>1)     + xava->inner.y;
                cairo_rectangle(handle->cr, x, y, xava->conf.bw, (h>>1) + 1);
                cairo_fill(handle->cr);
            }
        }
    } else {
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
    }

    cairo_save(handle->cr);
}

// no matter what condition, this ensures a safe write
EXP_FUNC void               xava_cairo_module_draw_safe  (xava_cairo_module_handle* handle) {
    UNUSED(handle);
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

    if(options.mirror) {
        for(size_t i = 0; i < xava->bars; i++) {
            int x = xava->rest + i*(conf->bs+conf->bw)+xava->inner.x;
            int y = (xava->inner.h - xava->f[i])/2    +xava->inner.y;
            cairo_rectangle(handle->cr, x, y, xava->conf.bw, xava->f[i]);
        }
    } else {
        for(size_t i = 0; i < xava->bars; i++) {
            int x = xava->rest + i*(conf->bs+conf->bw)+xava->inner.x;
            int y = xava->inner.h - xava->f[i]        +xava->inner.y;
            cairo_rectangle(handle->cr, x, y, xava->conf.bw, xava->f[i]);
        }
    }

    cairo_fill(handle->cr);
}

EXP_FUNC void               xava_cairo_module_cleanup    (xava_cairo_module_handle* handle) {
    UNUSED(handle);

    xavaIONotifyKill(file_notifications);

    xavaConfigClose(*config_file);
    free(config_file); // a fun hacky quirk because bad design
}

