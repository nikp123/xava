#ifndef XAVA_UTIL_MEDIA_INFO_ARTWORK_H
#define XAVA_UTIL_MEDIA_INFO_ARTWORK_H

#include <stdio.h>
#include <stdint.h>
#include <curl/curl.h>

struct            artwork {
    unsigned char   *file_data;
    unsigned char   *image_data;
    bool             ready;
    size_t           size;
    int              w, h, c;
};

void xava_util_artwork_destroy(struct artwork *artwork);
void xava_util_artwork_update(const char *url,
        struct artwork *artwork, CURL *curl);

#endif

