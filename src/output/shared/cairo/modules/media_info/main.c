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
} cover_geo;

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
} title, artist;

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

    cover_geo.x = 0.05;
    cover_geo.y = 0.05;
    cover_geo.size = 0.2;
    cover_geo.display_mode = ARTWORK_PRESERVE;
    cover_geo.texture_mode = ARTWORK_NEAREST;

    title.text = "";
    title.font = "Gotham";
    title.weight = TEXT_NORMAL;
    title.x = 0.175;
    title.y = 0.075;
    title.size = 60;
    title.color.r = 1.0;
    title.color.g = 1.0;
    title.color.b = 1.0;
    title.color.a = 1.0;

    artist.text = "";
    artist.font = "Gotham Book";
    artist.weight = TEXT_NORMAL;
    artist.x = 0.175;
    artist.y = 0.175;
    artist.size = 48;
    artist.color.r = 1.0;
    artist.color.g = 1.0;
    artist.color.b = 1.0;
    artist.color.a = 1.0;
}

EXP_FUNC void               xava_cairo_module_apply(xava_cairo_module_handle* handle) {
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

void xava_cairo_module_draw_artwork(
        cairo_t                 *cr,
        XAVA                    *xava,
        struct artwork          *artwork,
        struct artwork_geometry *geometry) {
    // avoid unsafety
    if(artwork->ready == false)
        return;

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
    cairo_set_source_surface(cr, artwork->surface, offset_x, offset_y);

    // overwrite dem pixels
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle(cr, offset_x, offset_y,
            artwork->w, artwork->h);

    // draw artwork
    cairo_fill(cr);

    // restore old scale
    cairo_scale(cr, ps_x, ps_y);
}

void xava_cairo_module_draw_text(
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
}

// assume that the entire screen's being overwritten
EXP_FUNC void               xava_cairo_module_draw_full  (xava_cairo_module_handle* handle) {

    struct media_data *data;
    data = xava_cairo_module_media_data_thread_data(media_data_thread);

    struct artwork *cover = &data->cover;
    xava_cairo_module_draw_artwork(handle->cr, handle->xava,
                                    cover, &cover_geo);

    title.text = data->title;
    xava_cairo_module_draw_text(handle->cr, handle->xava, &title);
 
    artist.text = data->artist;
    xava_cairo_module_draw_text(handle->cr, handle->xava, &artist);
}

EXP_FUNC void               xava_cairo_module_cleanup    (xava_cairo_module_handle* handle) {
    xava_cairo_module_media_data_thread_destroy(media_data_thread);
}

// ionotify fun
EXP_FUNC void         xava_cairo_module_ionotify_callback
                (XAVA_IONOTIFY_EVENT event,
                const char* filename,
                int id,
                XAVA* xava) {
}
