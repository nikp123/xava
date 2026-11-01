#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include <kiss-mpris.h>

#include "shared.h"

#include "media_info.h"

#define TRACK_ID_LENGTH 1024

struct media_info_thread {
    CURL             *curl;
    char              last_track_id[TRACK_ID_LENGTH];
    long int          last_timestamp;
    pthread_t         thread;
    bool              alive;
    XAVA_MEDIA_INFO   info;
};

void media_info_update(
        XAVA_MEDIA_INFO_THREAD *thread) {
    kiss_mpris_options options;
    mpris_properties  properties;

    options.player_count = 0;
    options.status       = MPRIS_PLAYER_ANY_PLAYING;
    properties           = get_mpris_player_status(&options);

    thread->last_timestamp = xavaGetTime();

    bool should_update = false;

    // check track ID for mismatches
    if(strncmp(properties.metadata.track_id, thread->last_track_id, TRACK_ID_LENGTH))
        should_update = true;

    // check album name for mismatches
    if(strncmp(properties.metadata.album, thread->info.album, MUSIC_DATA_STRING_LENGTH))
        should_update = true;

    // check song artist for mismatches
    if(strncmp(properties.metadata.artist, thread->info.artist, MUSIC_DATA_STRING_LENGTH))
        should_update = true;

    // check song title for mismatches
    if(strncmp(properties.metadata.title, thread->info.title, MUSIC_DATA_STRING_LENGTH))
        should_update = true;

    // check song genre for mismatches
    if(strncmp(properties.metadata.genre, thread->info.genre, MUSIC_DATA_STRING_LENGTH))
        should_update = true;

    if(should_update) {
        // this complicated mess turns empty art_url's with a file attached to said file
        if(strlen(properties.metadata.art_url) == 0 &&
            strncmp(properties.metadata.url, URI_HEADER_FILE,
                strlen(URI_HEADER_FILE)) == 0) {
            strncpy(properties.metadata.art_url,
                    URI_HEADER_MUSIC, MUSIC_DATA_STRING_LENGTH);
            strncat(properties.metadata.art_url,
                    &properties.metadata.url[strlen(URI_HEADER_FILE)],
                    MUSIC_DATA_STRING_LENGTH-1);
        }

        xava_util_artwork_update(properties.metadata.art_url,
                &thread->info.cover, thread->curl);

        strncpy(thread->last_track_id, properties.metadata.track_id, TRACK_ID_LENGTH);

        strncpy(thread->info.album,  properties.metadata.album, MUSIC_DATA_STRING_LENGTH);
        strncpy(thread->info.artist, properties.metadata.artist, MUSIC_DATA_STRING_LENGTH);
        strncpy(thread->info.title,  properties.metadata.title, MUSIC_DATA_STRING_LENGTH);
        strncpy(thread->info.genre,  properties.metadata.genre, MUSIC_DATA_STRING_LENGTH);

        thread->info.version++;
    }
}

void* media_info_thread_runner(void* ptr) {
    XAVA_MEDIA_INFO_THREAD *thread = ptr;
    thread->curl = curl_easy_init();

    while(thread->alive) {
        media_info_update(thread);

        // sleep for 1 second but interruptable every 50ms
        for(int i = 0; i < 10 && thread->alive; i++) {
            xavaSleep(100, 0);
        }
    }

    xava_util_artwork_destroy(&thread->info.cover);
    curl_easy_cleanup(thread->curl);
    return NULL;
}

XAVA_MEDIA_INFO* xavaMediaInfo(XAVA_MEDIA_INFO_THREAD *thread) {
    return &((struct media_info_thread*)thread)->info;
}

XAVA_MEDIA_INFO_THREAD* xavaMediaInfoCreate(void) {
    struct media_info_thread *thread;
    MALLOC_SELF(thread, 1);

    thread->info.cover.ready = false;
    thread->info.version = 0;

    thread->alive = true;
    pthread_create(&thread->thread, NULL, media_info_thread_runner, thread);

    return thread;
}

void xavaMediaInfoDestroy(XAVA_MEDIA_INFO_THREAD *thread) {
    thread->alive = false;
    pthread_join(thread->thread, NULL);
    free(thread);
}

