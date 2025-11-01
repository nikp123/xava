#include <math.h>
#include <cairo.h>

#include "output/shared/cairo/modules/shared/config.h"

#include "output/shared/cairo/util/module.h"
#include "output/shared/graphical.h"
#include "shared.h"

struct options {
    // noop
} options;

xava_config_source *config_file;
xava_ionotify       file_notifications;

// ionotify fun
EXP_FUNC void xava_cairo_module_ionotify_callback
                (xava_ionotify_event event,
                const char* filename,
                int id,
                void* global) {
    UNUSED(filename);
    UNUSED(id);
    xava_cairo_module_handle *handle = (xava_cairo_module_handle*)global;
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
    XAVA *xava = handle->xava;
    XAVA_CONFIG *conf = &xava->conf;

    //options.option = xavaConfigGetBool(*config_file, "oscilloscope", "option", false);

    // input MUST be stereo
    conf->stereo = true;
    conf->flag.skipFilter = true;

    return XAVA_CAIRO_FEATURE_FULL_DRAW;
}

EXP_FUNC void               xava_cairo_module_init(xava_cairo_module_handle* handle) {
    char config_file_path[MAX_PATH];
    config_file = xava_cairo_module_file_load(
            XAVA_CAIRO_FILE_CONFIG, handle, "config.ini", config_file_path);

    // setup file notifications
    file_notifications = xavaIONotifySetup();

    xava_ionotify_watch_setup setup;
    setup.filename           = config_file_path;
    setup.id                 = 1;
    setup.xava_ionotify_func = xava_cairo_module_ionotify_callback;
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

    xava_cairo_region *regions;
    arr_init(regions);

    xava_cairo_region region;
    region.x = xava->inner.x;
    region.y = xava->inner.y;
    region.w = xava->inner.w;
    region.h = xava->inner.h;
    arr_add(regions, region);

    return regions;
}

// event handler
EXP_FUNC void               xava_cairo_module_event      (xava_cairo_module_handle* handle) {
    UNUSED(handle);
}

// placeholder, as it literally does nothing atm
EXP_FUNC void               xava_cairo_module_clear      (xava_cairo_module_handle* handle) {
    UNUSED(handle);
}

EXP_FUNC void               xava_cairo_module_draw_region(xava_cairo_module_handle* handle) {
    UNUSED(handle);
}

// no matter what condition, this ensures a safe write
EXP_FUNC void               xava_cairo_module_draw_safe  (xava_cairo_module_handle* handle) {
    UNUSED(handle);
}

// assume that the entire screen's being overwritten
EXP_FUNC void               xava_cairo_module_draw_full  (xava_cairo_module_handle* handle) {
    XAVA   *xava = handle->xava;
    XAVA_CONFIG *conf = &xava->conf;
    XAVA_AUDIO *audio = &xava->audio;

    // setting new settings because default are WAY too slow
    cairo_antialias_t old_aa        = cairo_get_antialias(handle->cr);
    double            old_tolerance = cairo_get_tolerance(handle->cr);
    cairo_operator_t  old_operator  = cairo_get_operator(handle->cr);

    cairo_set_antialias(handle->cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_tolerance(handle->cr, 1.0);
    cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);

    cairo_set_source_rgba(handle->cr,
            ARGB_R_32(conf->col)/255.0,
            ARGB_G_32(conf->col)/255.0,
            ARGB_B_32(conf->col)/255.0,
            conf->foreground_opacity);

    cairo_set_line_width(handle->cr, 1.0);

    // optimization strats
    float scale_x = (float)xava->inner.w / INT16_MAX;
    float scale_y = (float)xava->inner.h / INT16_MIN;
    float trans_x = xava->inner.x + xava->inner.w/2.0;
    float trans_y = xava->inner.y + xava->inner.h/2.0;

    // optimized to shit loop
    double x1, y1, x2, y2;
    x1 = scale_x * audio->audio_out_l[0] + trans_x;
    y1 = scale_y * audio->audio_out_r[0] + trans_y;

    for(size_t i = 1; i < (size_t)audio->inputsize; i++) {
        x2 = scale_x * audio->audio_out_l[i] + trans_x;
        y2 = scale_y * audio->audio_out_r[i] + trans_y;

        if(i == 0)
            cairo_move_to(handle->cr, x1, y1);

        cairo_line_to(handle->cr, x2, y2);

        // swap
        x1 = x2;
        y1 = y2;
    }

    cairo_stroke(handle->cr);

    cairo_set_antialias(handle->cr, old_aa);
    cairo_set_tolerance(handle->cr, old_tolerance);
    cairo_set_operator(handle->cr, old_operator);
}

EXP_FUNC void               xava_cairo_module_cleanup    (xava_cairo_module_handle* handle) {
    UNUSED(handle);
    xavaIONotifyKill(file_notifications);

    xavaConfigClose(*config_file);
    free(config_file); // a fun hacky quirk because bad design
}

