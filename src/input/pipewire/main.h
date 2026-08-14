#ifndef PIPEWIRE_MAIN_H
#define PIPEWIRE_MAIN_H
#include "shared.h"
#include <pipewire/pipewire.h>

#define MAX_SOURCE_NAME_LEN 64

enum source_type {
    APP_NAME,
    APP_ID,
    MONITOR,
    NONE,
};

struct source_pair {
    enum source_type type;
    char name[MAX_SOURCE_NAME_LEN];
    int target_id;
};

struct pwdata {
    struct pw_main_loop *loop;
    struct pw_stream *stream;
    struct spa_hook streamListener;
    const struct spa_pod *params[1];

    // We need context in order to observe
    // changes in the sinks/sources that occur
    // Think of it as a pipewire handle
    struct pw_context *context;
    struct pw_core *core;
    struct pw_registry *registry;
    struct spa_hook pwRegistryListener;

    XAVA_AUDIO *audio;

    // PipeWire needs SPA parameters to be defined
    // using strings, these are just temporary buffers
    // which hold said information
    struct audio_str {
        char rate[32];
        char channels[32];
        char latencymax[32];
    } audio_str;

    struct source_pair *sources;

    bool autoconnect;
};

void registerCallback(struct pwdata *pwdata);
void deregisterCallback(struct pwdata *pwdata);
struct source_pair* extract_sources_from_string(char *str);

#endif

