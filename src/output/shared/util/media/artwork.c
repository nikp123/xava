#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "../../../../shared.h"

#include "artwork.h"

size_t xava_util_download_artwork(void *ptr, size_t size, size_t nmemb,
        struct artwork *artwork) {
    if(artwork->file_data == NULL) {
        artwork->file_data = malloc(size*nmemb);
        artwork->size = size*nmemb;
        memcpy(artwork->file_data, ptr, nmemb*size);
    } else {
        void *new = realloc(artwork->file_data, artwork->size+nmemb*size);
        artwork->file_data = new;
        memcpy(&artwork->file_data[artwork->size], ptr, nmemb*size);
        artwork->size += size*nmemb;
    }

    return nmemb*size;
}

void xava_util_artwork_destroy(struct artwork *artwork) {
    // reset artwork if already allocated
    if(artwork->ready) {
        // needs to be set early because stupid computers
        artwork->ready = false;
        free(artwork->file_data);
        free(artwork->image_data);
    }
    artwork->ready = false;
    artwork->size = 0;
    artwork->image_data = NULL;
    artwork->file_data = NULL;
}

void xava_util_artwork_update(const char *url,
        struct artwork *artwork, CURL *curl) {
    CURLcode res;

    xava_util_artwork_destroy(artwork);

    curl_easy_setopt(curl, CURLOPT_URL,           url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     artwork);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
            xava_util_download_artwork);

    res = curl_easy_perform(curl);

    if(res != CURLE_OK) {
        xavaLog("Failed to download '%s'", url);
        return;
    }

    if(artwork->size == 0) {
        xavaLog("Failed to download '%s'", url);
        return;
    }

    int w, h, c;
    artwork->image_data = stbi_load_from_memory(
            artwork->file_data, artwork->size, &w, &h, &c, 4);

    // update size and color info
    artwork->w = w;
    artwork->h = h;
    artwork->c = c;

    // correct color order
    for(unsigned char *ptr = artwork->image_data;
            ptr < artwork->image_data+w*h*4; ptr+=4) {
        ptr[0] ^= ptr[2];
        ptr[2] ^= ptr[0];
        ptr[0] ^= ptr[2];
    }

    artwork->ready = true;
}

