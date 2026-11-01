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

// probably never used but kept in case they eventually are
#define URI_HEADER_FILE  "file://"
#define URI_HEADER_HTTPS "https://"
#define URI_HEADER_HTTP  "http://"
#define URI_HEADER_MUSIC "music-file://"

void xava_util_artwork_destroy(struct artwork *artwork);
void xava_util_artwork_update(const char *url,
        struct artwork *artwork, CURL *curl);

// for internal use ONLY
bool xava_util_artwork_update_by_audio_file(const char *url,
        struct artwork *artwork);
#endif

