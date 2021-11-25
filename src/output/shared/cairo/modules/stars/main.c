#include <math.h>
#include <cairo/cairo.h>
#include "../../util/module.h"
#include "../../util/array.h"
#include "../../../graphical.h"
#include "../../../../../shared.h"

struct star {
    float x, y;
    float angle;
    uint32_t size;
} *stars;
uint32_t star_count = 100;

// report version
EXP_FUNC xava_version xava_cairo_module_version(void) {
    return xava_version_host_get();
}

// load all the necessary config data and report supported drawing modes
EXP_FUNC XAVA_CAIRO_FEATURE xava_cairo_module_config_load(xava_cairo_module_handle* handle) {
    return XAVA_CAIRO_FEATURE_FULL_DRAW;
}

EXP_FUNC void               xava_cairo_module_init(xava_cairo_module_handle* handle) {
    arr_init(stars);
}

EXP_FUNC void               xava_cairo_module_apply(xava_cairo_module_handle* handle) {
    struct XAVA_HANDLE *xava = handle->xava;

    arr_resize(stars, star_count);

    for(int i = 0; i < star_count; i++) {
        stars[i].angle = fmod(rand()/1000.0, 2.0*M_PI) - M_PI;
        stars[i].x     = fmod(rand(), xava->outer.w);
        stars[i].y     = fmod(rand(), xava->outer.h);
        stars[i].size  = rand()%4+1;
        xavaLog("%f %f %f %d", stars[i].angle, stars[i].x, stars[i].y,
                stars[i].size);
    }
}

// report drawn regions
EXP_FUNC xava_cairo_region* xava_cairo_module_regions(xava_cairo_module_handle* handle) {
    return NULL;
}

// event handler
EXP_FUNC void               xava_cairo_module_event      (xava_cairo_module_handle* handle) {
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
    struct XAVA_HANDLE   *xava = handle->xava;
    struct config_params *conf = &xava->conf;

    cairo_new_path(handle->cr);
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

    for(register int i=0; i<star_count; i++) {
        stars[i].x += stars[i].size*cos(stars[i].angle)*intensity;
        stars[i].y += stars[i].size*sin(stars[i].angle)*intensity;

        if(stars[i].x < 0.0-stars[i].size) {
            stars[i].x = xava->outer.w;
        } else if(stars[i].x > xava->outer.w+stars[i].size) {
            stars[i].x = 0;
        }

        if(stars[i].y < 0.0-stars[i].size) {
            stars[i].y = xava->outer.h;
        } else if(stars[i].y > xava->outer.h+stars[i].size) {
            stars[i].y = 0;
        }

        cairo_rectangle(handle->cr, stars[i].x, stars[i].y,
                stars[i].size, stars[i].size);
    }

    cairo_fill(handle->cr);
    cairo_save(handle->cr);
}

EXP_FUNC void               xava_cairo_module_cleanup    (xava_cairo_module_handle* handle) {
    arr_free(stars);
}

// ionotify fun
EXP_FUNC void         xava_cairo_module_ionotify_callback
                (XAVA_IONOTIFY_EVENT event,
                const char* filename,
                int id,
                struct XAVA_HANDLE* xava) {
}
