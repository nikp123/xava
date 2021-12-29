#ifndef XAVA_UTIL_MEDIA_INFO_MEDIA_DATA
#define XAVA_UTIL_MEDIA_INFO_MEDIA_DATA

#include "artwork.h"

#define MUSIC_DATA_STRING_LENGTH 1024

struct media_data {
    struct artwork cover;
    char           album [MUSIC_DATA_STRING_LENGTH];
    char           artist[MUSIC_DATA_STRING_LENGTH];
    char           title [MUSIC_DATA_STRING_LENGTH];
    char           genre [MUSIC_DATA_STRING_LENGTH];

    uint64_t       version;
};

struct media_data_thread;

struct media_data *
xava_util_media_data_thread_data(struct media_data_thread *thread);
struct media_data_thread *
xava_util_media_data_thread_create(void);
void
xava_util_media_data_thread_destroy(struct media_data_thread *data);

#endif

