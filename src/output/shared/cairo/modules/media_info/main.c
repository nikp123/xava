#include <math.h>
#include <cairo/cairo.h>

#include "../../../graphical.h"
#include "../../../../../shared.h"

// cairo-only utils
#include "../../util/module.h"
#include "../../util/array.h"

// shared utils
#include "../../../util/media/media_data.h"

bool redraw_everything;
static const int region_count = 3;

struct media_data_thread *media_data_thread;

typedef enum artwork_display_mode {
    ARTWORK_FULLSCREEN,
    ARTWORK_SCALED,
    ARTWORK_PRESERVE,
    ARTWORK_CROP_CENTER
} artwork_display_mode;

typedef enum artwork_texture_mode {
    ARTWORK_NEAREST,
    ARTWORK_BEST
} artwork_texture_mode;

typedef struct artwork_container {
    // x == 0 -> leftmost
    // x == 1 -> rightmost
    // y == 0 -> topmost
    // y == 1 -> bottommost
    // size is in screen sizes (smaller dimension will always apply)
    float                x, y, size;
    artwork_display_mode display_mode;
    artwork_texture_mode texture_mode;

    struct artwork       *image;
} artwork;

typedef struct text {
    char        *text;
    char        *font;

    // same logic as for artwork except size controls the font size
    float        x, y, size;

    float        max_x;

    struct color {
        float r, g, b, a;
    } color;

    cairo_font_slant_t slant;
    cairo_font_weight_t weight;
} text;

struct region {
    int x, y, w, h;
};

struct options {
    artwork cover;
    text    title, artist;
} options;

cairo_surface_t *surface;
struct region    surface_region;
int64_t last_version;

// report version
EXP_FUNC xava_version xava_cairo_module_version(void) {
    return xava_version_host_get();
}

// load all the necessary config data and report supported drawing modes
EXP_FUNC XAVA_CAIRO_FEATURE xava_cairo_module_config_load(xava_cairo_module_handle* handle) {
    options.cover.image = NULL; // because C

    options.cover.x = 0.05;
    options.cover.y = 0.05;
    options.cover.size = 0.2;
    options.cover.display_mode = ARTWORK_CROP_CENTER;
    options.cover.texture_mode = ARTWORK_BEST;

    options.title.font = "Noto Sans";
    options.title.weight = CAIRO_FONT_WEIGHT_BOLD;
    options.title.slant  = CAIRO_FONT_SLANT_NORMAL;
    options.title.x = 0.175;
    options.title.max_x = 0.8;
    options.title.y = 0.100;
    options.title.size = 40;
    options.title.color.r = 1.0;
    options.title.color.g = 1.0;
    options.title.color.b = 1.0;
    options.title.color.a = 1.0;

    options.artist.font = "Noto Sans";
    options.artist.weight = CAIRO_FONT_WEIGHT_NORMAL;
    options.artist.slant  = CAIRO_FONT_SLANT_NORMAL;
    options.artist.x = 0.175;
    options.artist.max_x = 0.8;
    options.artist.y = 0.160;
    options.artist.size = 28;
    options.artist.color.r = 1.0;
    options.artist.color.g = 1.0;
    options.artist.color.b = 1.0;
    options.artist.color.a = 1.0;

    // processing weight and slant

    return XAVA_CAIRO_FEATURE_FULL_DRAW |
        XAVA_CAIRO_FEATURE_DRAW_REGION_SAFE |
        XAVA_CAIRO_FEATURE_DRAW_REGION;
}

EXP_FUNC void               xava_cairo_module_init(xava_cairo_module_handle* handle) {
    media_data_thread = xava_util_media_data_thread_create();

    last_version = 0;
    redraw_everything = true;
    surface = NULL;
}

EXP_FUNC void               xava_cairo_module_apply(xava_cairo_module_handle* handle) {
    if(handle->use_feature == XAVA_CAIRO_FEATURE_DRAW_REGION) {
    }
}

struct xava_cairo_region xava_cairo_module_calculate_artwork_region(
        xava_cairo_module_handle *handle,
        artwork                  *artwork) {
    XAVA *xava = handle->xava;

    struct xava_cairo_region region = {
        .w = xava->outer.w,
        .h = xava->outer.h,
        .x = 0,
        .y = 0
    };

    switch(artwork->display_mode) {
        case ARTWORK_FULLSCREEN:
            // already correct
            break;
        case ARTWORK_CROP_CENTER:
        case ARTWORK_PRESERVE:
        {
            float scale = (float)xava->outer.w/xava->outer.h;
            float scale_x, scale_y;
            if(scale > 1.0) {
                scale_x = 1.0 / scale;
                scale_y = 1.0;
            } else {
                scale_x = 1.0;
                scale_y = 1.0 / scale;
            }

            float corrected_x = xava->outer.w - artwork->size*xava->outer.w;
            float corrected_y = xava->outer.h - artwork->size*xava->outer.h;

            region.x = (float)artwork->x * corrected_x * scale_x;
            region.y = (float)artwork->y * corrected_y * scale_y;
            region.w = artwork->size*xava->outer.w*scale_x;
            region.h = artwork->size*xava->outer.h*scale_y;
            break;
        }
        case ARTWORK_SCALED:
            region.x = artwork->x/(artwork->size+1.0)*xava->outer.w;
            region.y = artwork->y/(artwork->size+1.0)*xava->outer.y;
            region.w = artwork->size*xava->outer.w;
            region.h = artwork->size*xava->outer.h;
            break;
    }

    return region;
}

struct xava_cairo_region xava_cairo_module_calculate_text_region(
        xava_cairo_module_handle *handle,
        text                     *text) {
    XAVA *xava = handle->xava;

    struct xava_cairo_region region = {
        .w = xava->outer.w,
        .h = xava->outer.h,
        .x = 0,
        .y = 0
    };

    region.x = text->x * xava->outer.w;
    region.y = text->y * xava->outer.h;

    // set text properties
    cairo_select_font_face(handle->cr,
                        text->font,
                        text->slant,
                        text->weight);
    cairo_set_font_size(handle->cr, text->size);

    // calculate text extents
    cairo_text_extents_t extents;
    cairo_text_extents(handle->cr, "test123", &extents);

    region.w = text->max_x * xava->outer.w;
    region.h = extents.height;

    return region;
}

// report drawn regions
EXP_FUNC xava_cairo_region* xava_cairo_module_regions(xava_cairo_module_handle* handle) {
    struct xava_cairo_region *regions;

    arr_init_n(regions, region_count);

    arr_add(regions, xava_cairo_module_calculate_artwork_region(handle, &options.cover));
    arr_add(regions, xava_cairo_module_calculate_text_region(handle, &options.artist));
    arr_add(regions, xava_cairo_module_calculate_text_region(handle, &options.title));

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

struct region xava_cairo_module_draw_artwork(
        cairo_t          *cr,
        XAVA             *xava,
        artwork          *artwork) {
    // avoid unsafety
    if(artwork->image->ready == false)
        return (struct region){ 0 };

    cairo_surface_t *art_surface = cairo_image_surface_create_for_data(
            artwork->image->image_data,
            CAIRO_FORMAT_RGB24, artwork->image->w, artwork->image->h,
            cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, artwork->image->w));

    float actual_scale_x = (float)xava->outer.w / artwork->image->w;
    float actual_scale_y = (float)xava->outer.h / artwork->image->h;

    float scale_x;
    float scale_y;

    float crop_x = 0.0, crop_y = 0.0;

    switch(artwork->display_mode) {
        case ARTWORK_FULLSCREEN:
            scale_x = actual_scale_x;
            scale_y = actual_scale_y;
            break;
        case ARTWORK_CROP_CENTER:
            if(artwork->image->w > artwork->image->h) {
                crop_x = 0.5 * (artwork->image->w-artwork->image->h);
            }
            if(artwork->image->h > artwork->image->w) {
                crop_y = 0.5 * (artwork->image->h-artwork->image->w);
            }
        case ARTWORK_PRESERVE:
            if(actual_scale_x > actual_scale_y) {
                scale_x = actual_scale_y*artwork->size;
                scale_y = actual_scale_y*artwork->size;
            } else {
                scale_x = actual_scale_x*artwork->size;
                scale_y = actual_scale_x*artwork->size;
            }
            break;
        case ARTWORK_SCALED:
            scale_x = actual_scale_x*artwork->size;
            scale_y = actual_scale_y*artwork->size;
            break;
    }

    // this is necessary because performance otherwise is god-awful
    switch(artwork->texture_mode) {
        case ARTWORK_NEAREST:
            cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
            break;
        case ARTWORK_BEST:
            cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BEST);
            break;
    }

    // scale image
    cairo_scale(cr, scale_x, scale_y);

    // pixel scale x/y
    float ps_x = 1.0/scale_x;
    float ps_y = 1.0/scale_y;

    float offset_x = (xava->outer.w/scale_x - artwork->image->w + crop_x*2.0)*artwork->x;
    float offset_y = (xava->outer.h/scale_y - artwork->image->h + crop_y*2.0)*artwork->y;

    // set artwork as brush
    cairo_set_source_surface(cr, art_surface, offset_x-crop_x, offset_y-crop_y);

    // overwrite dem pixels
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle(cr, floor(offset_x), floor(offset_y),
            floor(artwork->image->w - crop_x*2.0),
            floor(artwork->image->h - crop_y*2.0));

    // draw artwork
    cairo_fill(cr);

    // restore old scale
    cairo_scale(cr, ps_x, ps_y);

    // free artwork surface
    cairo_surface_destroy(art_surface);

    return (struct region) {
        .w = artwork->image->w*scale_x,
        .h = artwork->image->h*scale_y,
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

    int font_size = text->size;
xava_cairo_module_resize_font:

    // set text properties
    cairo_select_font_face(cr,
                        text->font,
                        text->slant,
                        text->weight);
    cairo_set_font_size(cr, font_size);

    // calculate text extents
    cairo_text_extents_t extents;
    cairo_text_extents(cr, text->text, &extents);

    // shrink the text if it's too big
    if(extents.width > xava->outer.w*text->max_x) {
        font_size--;
        goto xava_cairo_module_resize_font;
    }

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
        struct options           *options,
        struct region            *new_region) {

    XAVA *xava = handle->xava;

    artwork *cover =  &options->cover;
    text    *title =  &options->title;
    text    *artist = &options->artist;

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, xava->outer.w, xava->outer.h);
    cairo_t *new_context = cairo_create(surface);

    cairo_set_source_rgba(new_context, 0.0, 0.0, 0.0, 0.0);

    cairo_set_operator(new_context, CAIRO_OPERATOR_SOURCE);
    cairo_set_antialias(new_context, CAIRO_ANTIALIAS_BEST);
    cairo_paint(new_context);

    struct region regions[region_count];

    regions[0] = xava_cairo_module_draw_artwork(
            new_context, handle->xava, cover);

    regions[1] = xava_cairo_module_draw_text(
            new_context, handle->xava, title);

    regions[2] = xava_cairo_module_draw_text(
            new_context, handle->xava, artist);

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

    new_region->x = min_x;
    new_region->y = min_y;
    new_region->w = max_x - min_x;
    new_region->h = max_y - min_y;

    return surface;
}

// assume that the entire screen's being overwritten
EXP_FUNC void               xava_cairo_module_draw_full  (xava_cairo_module_handle* handle) {

    struct media_data *data;
    data = xava_util_media_data_thread_data(media_data_thread);

    // artwork has been updated, draw the new one
    if(data->version != last_version) {
        if(data->version > 1) {
            cairo_surface_destroy(surface);
        }

        options.artist.text = data->artist;
        options.title.text  = data->title;
        options.cover.image = &data->cover;

        surface = xava_cairo_module_draw_new_media_screen(
                handle, &options, &surface_region);

        last_version = data->version;

        xavaLog("Artwork update number: %d", data->version);
    }

    // skip drawing if no artwork is available
    if(options.cover.image == NULL)
        return;

    if(options.cover.image->ready == false)
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

    redraw_everything = false;
}

// informs the thread that it should redraw
EXP_FUNC void               xava_cairo_module_clear      (xava_cairo_module_handle* handle) {
    redraw_everything = true;
}

EXP_FUNC void               xava_cairo_module_draw_region(xava_cairo_module_handle* handle) {
    struct media_data *data;
    data = xava_util_media_data_thread_data(media_data_thread);

    if(data->version != last_version)
        redraw_everything = true;

    // break if nothing changes
    if(redraw_everything == false)
        return;

    xava_cairo_region* regions = xava_cairo_module_regions(handle);
    XAVA*              xava = handle->xava;

    int min_x = xava->outer.w,
        min_y = xava->outer.h,
        max_x = 0, max_y = 0;

    // region bound dark magic
    for(int i = 0; i < arr_count(regions); i++) {
        if(min_x > regions[i].x)
           min_x = regions[i].x;
        if(min_y > regions[i].y)
           min_y = regions[i].y;
        if(max_x < regions[i].x+regions[i].w)
           max_x = regions[i].x+regions[i].w;
        if(max_y < regions[i].y+regions[i].h)
           max_y = regions[i].y+regions[i].h;
    }

    cairo_set_source_rgba(handle->cr,
            ARGB_R_32(handle->xava->conf.bgcol)/255.0,
            ARGB_G_32(handle->xava->conf.bgcol)/255.0,
            ARGB_B_32(handle->xava->conf.bgcol)/255.0,
            handle->xava->conf.background_opacity);

    cairo_rectangle(handle->cr, min_x, min_y, max_x-min_x, max_y-min_y);
    cairo_fill(handle->cr);

    arr_free(regions);

    xava_cairo_module_draw_full(handle);
}

// no matter what condition, this ensures a safe write
EXP_FUNC void               xava_cairo_module_draw_safe  (xava_cairo_module_handle* handle) {
    xava_cairo_module_draw_full(handle);
}

EXP_FUNC void               xava_cairo_module_cleanup    (xava_cairo_module_handle* handle) {
    xava_util_media_data_thread_destroy(media_data_thread);
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
