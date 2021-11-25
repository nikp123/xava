#include <limits.h>
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
float star_density = 0.0001;
uint32_t star_count = 0;
uint32_t star_max_size = 5;

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

float xava_box_muller_transform(void) {
    float u1 = (float)rand()/(float)INT_MAX;
    float u2 = (float)rand()/(float)INT_MAX;

    return (float)sqrt(-2.0*log(u1))*(float)sin(2.0*M_PI*u2)/2.0;
}

float xava_generate_star_angle(void) {
    float r = (float)rand()/(float)INT_MAX;

    return 1.0 - pow(sin(r*M_PI), 0.5);
}

uint32_t xava_generate_star_size(void) {
    float r = (float)rand()/(float)INT_MAX;

    return floor((1.0-pow(r, 0.5))*star_max_size)+1;
}

EXP_FUNC void               xava_cairo_module_apply(xava_cairo_module_handle* handle) {
    XAVA *xava = handle->xava;

    // very scientific, much wow
    star_count = (float)xava->outer.w*(float)xava->outer.h*star_density;

    arr_resize(stars, star_count);

    for(int i = 0; i < star_count; i++) {
        // generate the stars with random angles
        // but with a bias towards the right
        stars[i].angle = xava_generate_star_angle();
        stars[i].x     = fmod(rand(), xava->outer.w);
        stars[i].y     = fmod(rand(), xava->outer.h);
        stars[i].size  = xava_generate_star_size();
        xavaLog("%d", stars[i].size);
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
    XAVA   *xava = handle->xava;
    XAVA_CONFIG *conf = &xava->conf;

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
        float alpha = (float)(1+star_max_size-stars[i].size)/star_max_size;

        cairo_new_path(handle->cr);
        cairo_set_source_rgba(handle->cr,
                ARGB_R_32(conf->col)/255.0,
                ARGB_G_32(conf->col)/255.0,
                ARGB_B_32(conf->col)/255.0,
                conf->foreground_opacity*alpha);

        stars[i].x += stars[i].size*cos(stars[i].angle)*intensity;
        stars[i].y += stars[i].size*sin(stars[i].angle)*intensity;

        if(stars[i].x < 0.0-stars[i].size) {
            stars[i].x = xava->outer.w;
            stars[i].angle = xava_generate_star_angle();
        } else if(stars[i].x > xava->outer.w+stars[i].size) {
            stars[i].x = 0;
            stars[i].angle = xava_generate_star_angle();
        }

        if(stars[i].y < 0.0-stars[i].size) {
            stars[i].y = xava->outer.h;
            stars[i].angle = xava_generate_star_angle();
        } else if(stars[i].y > xava->outer.h+stars[i].size) {
            stars[i].y = 0;
            stars[i].angle = xava_generate_star_angle();
        }

        cairo_rectangle(handle->cr, stars[i].x, stars[i].y,
                stars[i].size, stars[i].size);

        cairo_fill(handle->cr);
    }
}

EXP_FUNC void               xava_cairo_module_cleanup    (xava_cairo_module_handle* handle) {
    arr_free(stars);
}

// ionotify fun
EXP_FUNC void         xava_cairo_module_ionotify_callback
                (XAVA_IONOTIFY_EVENT event,
                const char* filename,
                int id,
                XAVA* xava) {
}
