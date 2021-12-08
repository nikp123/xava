#ifndef XAVA_CAIRO_MODULE_MEDIA_INFO_ARTWORK_H
#define XAVA_CAIRO_MODULE_MEDIA_INFO_ARTWORK_H

#include <stdio.h>
#include <stdint.h>
#include <cairo/cairo.h>
#include <curl/curl.h>

struct            artwork {
    // these are unsafe (NO TOUCHIES >:()
    unsigned char   *file_data;
    unsigned char   *image_data;
    bool             ready;
    size_t           size;
    int              w, h, c;

    // you can have this
    cairo_surface_t *surface;
};

void xava_cairo_module_destroy_artwork(struct artwork *artwork);
void xava_cairo_module_update_artwork(const char *url,
        struct artwork *artwork, CURL *curl);

#endif

