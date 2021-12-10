#include <math.h>
#include <cairo/cairo.h>

#include "../../util/module.h"
#include "../../util/array.h"
#include "../../../graphical.h"
#include "../../../../../shared.h"

#include "media_data.h"

struct media_data_thread *media_data_thread;

typedef enum artwork_display_mode {
    ARTWORK_FULLSCREEN,
    ARTWORK_SCALED,
    ARTWORK_PRESERVE
} artwork_display_mode;

typedef enum artwork_texture_mode {
    ARTWORK_NEAREST
} artwork_texture_mode;

struct artwork_geometry {
    // x == 0 -> leftmost
    // x == 1 -> rightmost
    // y == 0 -> topmost
    // y == 1 -> bottommost
    // size is in screen sizes (smaller dimension will always apply)
    float                x, y, size;
    artwork_display_mode display_mode;
    artwork_texture_mode texture_mode;
};

typedef enum text_weight {
    TEXT_NORMAL,
    TEXT_BOLD
} text_weight;

struct text {
    char        *text;
    char        *font;
    text_weight  weight;

    // same logic as for artwork except size controls the font size
    float        x, y, size;

    struct color {
        float r, g, b, a;
    } color;
};

struct region {
    int x, y, w, h;
};

cairo_surface_t *surface;
struct region    surface_region;
uint64_t last_version;

// report version
EXP_FUNC xava_version xava_cairo_module_version(void) {
    return xava_version_host_get();
}

// load all the necessary config data and report supported drawing modes
EXP_FUNC XAVA_CAIRO_FEATURE xava_cairo_module_config_load(xava_cairo_module_handle* handle) {
    return XAVA_CAIRO_FEATURE_FULL_DRAW;
}

EXP_FUNC void               xava_cairo_module_init(xava_cairo_module_handle* handle) {
    media_data_thread = xava_cairo_module_media_data_thread_create();

    last_version = 0;
    surface = NULL;
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

struct region xava_cairo_module_draw_artwork(
        cairo_t                 *cr,
        XAVA                    *xava,
        struct artwork          *artwork,
        struct artwork_geometry *geometry) {
    // avoid unsafety
    if(artwork->ready == false)
        return (struct region){ 0 };

    cairo_surface_t *surface = cairo_image_surface_create_for_data(
            artwork->image_data,
            CAIRO_FORMAT_RGB24, artwork->w, artwork->h,
            cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, artwork->w));

    float actual_scale_x = (float)xava->outer.w / artwork->w;
    float actual_scale_y = (float)xava->outer.h / artwork->h;

    float scale_x;
    float scale_y;

    switch(geometry->display_mode) {
        case ARTWORK_FULLSCREEN:
            scale_x = actual_scale_x;
            scale_y = actual_scale_y;
            break;
        case ARTWORK_PRESERVE:
            if(actual_scale_x > actual_scale_y) {
                scale_x = actual_scale_y*geometry->size;
                scale_y = actual_scale_y*geometry->size;
            } else {
                scale_x = actual_scale_x*geometry->size;
                scale_y = actual_scale_x*geometry->size;
            }
            break;
        case ARTWORK_SCALED:
            scale_x = actual_scale_x*geometry->size;
            scale_y = actual_scale_y*geometry->size;
            break;
    }

    // this is necessary because performance otherwise is god-awful
    switch(geometry->texture_mode) {
        case ARTWORK_NEAREST:
            cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
            break;
    }

    // scale image
    cairo_scale(cr, scale_x, scale_y);

    // pixel scale x/y
    float ps_x = 1.0/scale_x;
    float ps_y = 1.0/scale_y;

    float offset_x = (xava->outer.w/scale_x - artwork->w)*geometry->x;
    float offset_y = (xava->outer.h/scale_y - artwork->h)*geometry->y;

    // set artwork as brush
    cairo_set_source_surface(cr, surface, offset_x, offset_y);

    // overwrite dem pixels
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle(cr, offset_x, offset_y,
            artwork->w, artwork->h);

    // draw artwork
    cairo_fill(cr);

    // restore old scale
    cairo_scale(cr, ps_x, ps_y);

    // free artwork surface
    cairo_surface_destroy(surface);

    return (struct region) {
        .w = artwork->w*scale_x,
        .h = artwork->h*scale_y,
        .x = offset_x*scale_x,
        .y = offset_y*scale_x,
    };
}

struct region xava_cairo_module_draw_text(
        cairo_t     *cr,
        XAVA        *xava,
        struct text *text) {
    XAVA_CONFIG *conf = &xava->conf;

    // colors
    cairo_set_source_rgba(cr, text->color.r,
            text->color.g, text->color.b, text->color.a);

    // processing weight and slant
    cairo_font_slant_t slant;
    slant = CAIRO_FONT_SLANT_NORMAL;
    cairo_font_weight_t weight;
    switch(text->weight) {
        case TEXT_BOLD:
            weight = CAIRO_FONT_WEIGHT_BOLD;
            break;
        case TEXT_NORMAL:
            weight = CAIRO_FONT_WEIGHT_NORMAL;
            break;
    }

    // set text properties
    cairo_select_font_face(cr,
                        text->font,
                        slant,
                        weight);
    cairo_set_font_size(cr, text->size);

    // calculate text extents
    cairo_text_extents_t extents;
    cairo_text_extents(cr, text->text, &extents);

    // calculate text offsets
    float offset_x = xava->outer.w * text->x;
    float offset_y = extents.height + (xava->outer.h - extents.height) * text->y;

    // draw text
    cairo_move_to(cr, offset_x, offset_y);
    cairo_show_text(cr, text->text);

    const int hacky_x_add_because_cairo = 20;

    return (struct region) {
        .w = extents.width+hacky_x_add_because_cairo,
        .h = extents.height,
        .x = offset_x,
        .y = offset_y-extents.height,
    };
}

cairo_surface_t *xava_cairo_module_draw_new_media_screen(
        xava_cairo_module_handle *handle,
        struct media_data        *data,
        struct region            *new_region) {

    XAVA *xava = handle->xava;

    struct artwork_geometry cover_geo;
    struct text             title, artist;

    cover_geo.x = 0.05;
    cover_geo.y = 0.05;
    cover_geo.size = 0.2;
    cover_geo.display_mode = ARTWORK_PRESERVE;
    cover_geo.texture_mode = ARTWORK_NEAREST;

    title.font = "Noto Sans";
    title.weight = TEXT_BOLD;
    title.x = 0.175;
    title.y = 0.100;
    title.size = 40;
    title.color.r = 1.0;
    title.color.g = 1.0;
    title.color.b = 1.0;
    title.color.a = 1.0;

    artist.font = "Noto Sans";
    artist.weight = TEXT_NORMAL;
    artist.x = 0.175;
    artist.y = 0.160;
    artist.size = 28;
    artist.color.r = 1.0;
    artist.color.g = 1.0;
    artist.color.b = 1.0;
    artist.color.a = 1.0;

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, xava->outer.w, xava->outer.h);
    cairo_t *new_context = cairo_create(surface);

    cairo_set_source_rgba(new_context, 0.0, 0.0, 0.0, 0.0);

    cairo_set_operator(new_context, CAIRO_OPERATOR_SOURCE);
    cairo_set_antialias(new_context, CAIRO_ANTIALIAS_BEST);
    cairo_paint(new_context);

    const int region_count = 3;
    struct region regions[region_count];

    regions[0] = xava_cairo_module_draw_artwork(
            new_context, handle->xava, &data->cover, &cover_geo);

    title.text = data->title;
    regions[1] = xava_cairo_module_draw_text(
            new_context, handle->xava, &title);

    artist.text = data->artist;
    regions[2] = xava_cairo_module_draw_text(
            new_context, handle->xava, &artist);

    int min_x = xava->outer.w,
        min_y = xava->outer.h,
        max_x = 0, max_y = 0;

    // region bound dark magic
    for(int i = 0; i < region_count; i++) {
        if(min_x > regions[i].x)
           min_x = regions[i].x;
        if(min_y > regions[i].y)
           min_y = regions[i].y;
        if(max_x < regions[i].x+regions[i].w)
           max_x = regions[i].x+regions[i].w;
        if(max_y < regions[i].y+regions[i].h)
           max_y = regions[i].y+regions[i].h;
    }

    cairo_destroy(new_context);

    xavaLog("Artwork draw %d", data->version);

    new_region->x = min_x;
    new_region->y = min_y;
    new_region->w = max_x - min_x;
    new_region->h = max_y - min_y;

    return surface;
}

// assume that the entire screen's being overwritten
EXP_FUNC void               xava_cairo_module_draw_full  (xava_cairo_module_handle* handle) {

    struct media_data *data;
    data = xava_cairo_module_media_data_thread_data(media_data_thread);

    // artwork has been updated, draw the new one
    if(data->version != last_version) {
        if(data->version > 1) {
            cairo_surface_destroy(surface);
        }

        surface = xava_cairo_module_draw_new_media_screen(
                handle, data, &surface_region);
        last_version = data->version;
    }

    // skip drawing if no artwork is available
    if(data->version == 0)
        return;

    cairo_set_operator(handle->cr, CAIRO_OPERATOR_OVER);

    // set artwork as brush
    cairo_set_source_surface(handle->cr, surface,
            0, 0);

    // overwrite dem pixels
    cairo_rectangle(handle->cr,
            surface_region.x, surface_region.y,
            surface_region.w, surface_region.h);

    // draw artwork
    cairo_fill(handle->cr);

    // revert old (default) pixel drawing mode
    cairo_set_operator(handle->cr, CAIRO_OPERATOR_SOURCE);
}

EXP_FUNC void               xava_cairo_module_cleanup    (xava_cairo_module_handle* handle) {
    xava_cairo_module_media_data_thread_destroy(media_data_thread);
    if(last_version > 0)
        cairo_surface_destroy(surface);
}

// ionotify fun
EXP_FUNC void         xava_cairo_module_ionotify_callback
                (XAVA_IONOTIFY_EVENT event,
                const char* filename,
                int id,
                XAVA* xava) {
}
