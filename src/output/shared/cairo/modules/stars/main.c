#include <stdio.h>
#include <limits.h>
#include <math.h>

#include <cairo/cairo.h>

#include "../shared/config.h"

#include "../../util/module.h"
#include "../../util/array.h"
#include "../../../graphical.h"
#include "../../../../../shared.h"

struct star {
    float x, y;
    float angle;
    uint32_t size;
} *stars;

struct options {
    struct star_options {
        float    density;
        uint32_t count;
        uint32_t max_size;
        char     *color_str;
        uint32_t color;
    } star;
} options;

xava_config_source *config_file;
XAVAIONOTIFY       file_notifications;

// ionotify fun
void         xava_cairo_module_ionotify_callback
                (XAVA_IONOTIFY_EVENT event,
                const char* filename,
                int id,
                XAVA* xava) {
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
    char config_file_path[MAX_PATH];
    config_file = xava_cairo_module_file_load(
            XAVA_CAIRO_FILE_CONFIG, handle, "config.ini", config_file_path);

    options.star.count     = xavaConfigGetInt(*config_file, "stars", "count", 0);
    options.star.density   = 0.0001 *
        xavaConfigGetDouble(*config_file, "stars", "density", 1.0);
    options.star.max_size  = xavaConfigGetInt(*config_file, "stars", "max_size", 5);
    options.star.color_str = xavaConfigGetString(*config_file, "stars", "color", NULL);

    xavaBailCondition(options.star.max_size < 1, "max_size cannot be below 1");
    xavaBailCondition(options.star.count < 0, "star count cannot be negative");

    // setup file notifications
    file_notifications = xavaIONotifySetup();

    XAVAIONOTIFYWATCHSETUP setup;
    MALLOC_SELF(setup, 1);
    setup->filename           = config_file_path;
    setup->id                 = 1;
    setup->xava_ionotify_func = xava_cairo_module_ionotify_callback;
    setup->xava               = (XAVA*) handle;
    setup->ionotify           = file_notifications;
    xavaIONotifyAddWatch(setup);

    xavaIONotifyStart(file_notifications);

    free(setup);

    return XAVA_CAIRO_FEATURE_FULL_DRAW;
}

EXP_FUNC void               xava_cairo_module_init(xava_cairo_module_handle* handle) {
    arr_init(stars);
}

float xava_generate_star_angle(void) {
    float r = (float)rand()/(float)RAND_MAX;

    return 0.7 - pow(sin(r*M_PI), 0.5);
}

uint32_t xava_generate_star_size(void) {
    float r = (float)rand()/(float)RAND_MAX;

    return floor((1.0-pow(r, 0.5))*options.star.max_size)+1;
}

EXP_FUNC void               xava_cairo_module_apply(xava_cairo_module_handle* handle) {
    XAVA *xava = handle->xava;

    int32_t star_count;
    if(options.star.count == 0) {
        // very scientific, much wow
        star_count = xava->outer.w*xava->outer.h*options.star.density;
    } else {
        star_count = options.star.count;
    }

    arr_resize(stars, star_count);

    for(int i = 0; i < star_count; i++) {
        // generate the stars with random angles
        // but with a bias towards the right
        stars[i].angle = xava_generate_star_angle();
        stars[i].x     = fmod(rand(), xava->outer.w);
        stars[i].y     = fmod(rand(), xava->outer.h);
        stars[i].size  = xava_generate_star_size();
    }

    if(options.star.color_str == NULL) {
        options.star.color = xava->conf.col;

        // this is dumb, but it works
        options.star.color |= ((uint8_t)xava->conf.foreground_opacity*0xFF)<<24;
    } else do {
        int err = sscanf(options.star.color_str,
                "#%08x", &options.star.color);
        if(err == 1)
            break;

        err = sscanf(options.star.color_str,
                "#%08X", &options.star.color);
        if(err == 1)
            break;

        xavaBail("'%s' is not a valid color", options.star.color_str);
    } while(0);
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

    // batching per color because that's fast in cairo
    for(register int j=1; j<=options.star.max_size; j++) {
        float alpha = (float)(1+options.star.max_size-j)/options.star.max_size;

        cairo_set_source_rgba(handle->cr,
                ARGB_R_32(options.star.color)/255.0,
                ARGB_G_32(options.star.color)/255.0,
                ARGB_B_32(options.star.color)/255.0,
                ARGB_A_32(options.star.color)/255.0*alpha);
        for(register int i=0; i<arr_count(stars); i++) {
            // we're drawing per star
            if(stars[i].size != j)
                continue;

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
        }

        // execute batch
        cairo_fill(handle->cr);
    }
}

EXP_FUNC void               xava_cairo_module_cleanup    (xava_cairo_module_handle* handle) {
    arr_free(stars);

    xavaIONotifyKill(file_notifications);

    xavaConfigClose(*config_file);
    free(config_file); // a fun hacky quirk because bad design
}

