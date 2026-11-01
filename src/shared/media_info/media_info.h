#ifndef XAVA_UTIL_MEDIA_INFO_MEDIA_INFO
#define XAVA_UTIL_MEDIA_INFO_MEDIA_INFO

#include "artwork.h"

#define MUSIC_DATA_STRING_LENGTH 1024

typedef struct media_info {
    struct artwork cover;
    char           album [MUSIC_DATA_STRING_LENGTH];
    char           artist[MUSIC_DATA_STRING_LENGTH];
    char           title [MUSIC_DATA_STRING_LENGTH];
    char           genre [MUSIC_DATA_STRING_LENGTH];

    uint64_t       version;
} XAVA_MEDIA_INFO;

typedef struct media_info_thread XAVA_MEDIA_INFO_THREAD;

XAVA_MEDIA_INFO *xavaMediaInfo(XAVA_MEDIA_INFO_THREAD *thread);
XAVA_MEDIA_INFO_THREAD *xavaMediaInfoCreate(void);
void xavaMediaInfoDestroy(XAVA_MEDIA_INFO_THREAD *thread);

#endif

