#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include <kiss-mpris.h>

#include "../../../../shared.h"

#include "media_data.h"

#define TRACK_ID_LENGHT 1024

struct media_data_thread {
    CURL             *curl;
    char              last_track_id[TRACK_ID_LENGHT];
    long int          last_timestamp;
    pthread_t         thread;
    bool              alive;
    struct media_data data;
};

void media_data_update
        (struct media_data_thread *data) {
    kiss_mpris_options options;
    mpris_properties  properties;

    options.player_count = 0;
    options.status       = MPRIS_PLAYER_ANY;
    properties           = get_mpris_player_status(&options);

    data->last_timestamp = xavaGetTime();

    bool should_update = false;

    // check track ID for mismatches
    if(strncmp(properties.metadata.track_id, data->last_track_id, TRACK_ID_LENGHT))
        should_update = true;

    // check album name for mismatches
    if(strncmp(properties.metadata.album, data->data.album, MUSIC_DATA_STRING_LENGHT))
        should_update = true;

    // check song artist for mismatches
    if(strncmp(properties.metadata.artist, data->data.artist, MUSIC_DATA_STRING_LENGHT))
        should_update = true;

    // check song title for mismatches
    if(strncmp(properties.metadata.title, data->data.title, MUSIC_DATA_STRING_LENGHT))
        should_update = true;

    if(should_update) {
        xava_util_artwork_update(properties.metadata.art_url,
                &data->data.cover, data->curl);

        strncpy(data->last_track_id, properties.metadata.track_id, TRACK_ID_LENGHT);

        strncpy(data->data.album, properties.metadata.album, MUSIC_DATA_STRING_LENGHT);
        strncpy(data->data.artist, properties.metadata.artist, MUSIC_DATA_STRING_LENGHT);
        strncpy(data->data.title, properties.metadata.title, MUSIC_DATA_STRING_LENGHT);

        data->data.version++;
    }
}

void* media_data_thread_runner(void* ptr) {
    struct media_data_thread *data = ptr;
    data->curl = curl_easy_init();

    while(data->alive) {
        media_data_update(data);

        // sleep for 5 seconds but interruptable every 50ms
        for(int i = 0; i < 100 && data->alive; i++) {
            xavaSleep(50, 0);
        }
    }

    xava_util_artwork_destroy(&data->data.cover);
    curl_easy_cleanup(data->curl);
    return NULL;
}

struct media_data *
    xava_util_media_data_thread_data(struct media_data_thread *thread) {
    return &thread->data;
}

struct media_data_thread *
    xava_util_media_data_thread_create(void) {
    struct media_data_thread *value;
    MALLOC_SELF(value, 1);

    value->data.cover.ready = false;
    value->data.version = 0;

    value->alive = true;
    pthread_create(&value->thread, NULL, media_data_thread_runner, value);

    return value;
}

void xava_util_media_data_thread_destroy(struct media_data_thread *data) {
    data->alive = false;
    pthread_join(data->thread, NULL);
    free(data);
}

