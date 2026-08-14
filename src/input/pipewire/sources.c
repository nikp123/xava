#include <assert.h>

#include <pipewire/pipewire.h>
#include <pipewire/stream.h>
#include <spa/utils/dict.h>

#include "main.h"
#include "pipewire/core.h"
#include "pipewire/stream.h"

struct source_pair* extract_sources_from_string(char *str) {
    char *pair_token;
    char *rest = str; // For strtok_r

    struct source_pair *list;

    arr_init(list);

    while ((pair_token = strtok_r(rest, ",", &rest)) != NULL) {
        // Process each pair_token
        char *key_token;
        char *value_token;
        char *pair_rest = pair_token; // For strtok_r within the pair

        key_token = strtok_r(pair_rest, ":", &pair_rest);
        value_token = strtok_r(pair_rest, ":", &pair_rest);

        struct source_pair pair;

        if(!strcmp(key_token, "name")) {
            pair.type = APP_NAME;
            xavaBailCondition(value_token == NULL,
                "App pipewire source type must have app name set (example: \"name:spotify\")");
        } else if(!strcmp(key_token, "id")) {
            pair.type = APP_ID;
            xavaBailCondition(value_token == NULL,
                "ID pipewire source type must have app name set (example: \"id:com.github.nikp123"PACKAGE"\")");
        } else if(!strcmp(key_token, "monitor")) {
            pair.type = MONITOR;
        } else {
            xavaLog("Unrecognized pipewire source type \"%s\"", value_token);
            pair.type = NONE;
        }
        pair.target_id = -1; // We haven't found a matching device

        strcpy(pair.name, value_token ? value_token : "");

        arr_add(list, pair);

        //xavaSpam("Key: %s, Value: %s\n", key_token, value_token);
    }
    return list;
}

static void on_state_changed(void *data,
                             enum pw_stream_state old,
                             enum pw_stream_state state,
                             const char *error)
{
    struct pwdata *pwdata = data;

    xavaSpam("stream state: %s -> %s (%s)",
           pw_stream_state_as_string(old),
           pw_stream_state_as_string(state),
           error ? error : "ok");

    if (state == PW_STREAM_STATE_UNCONNECTED) {
        xavaSpam("Stream disconnected!");
        struct source_pair *pairs = pwdata->sources;
        if(pairs == NULL) return;
        for(uint32_t i = 0; i < arr_count(pairs); i++) {
            int id = pwdata->sources[i].target_id;
            if(id == -1) continue;

            xavaLog("Made choice number %d, ID=%d seems ideal", i+1, id);
            pw_stream_connect(pwdata->stream, PW_DIRECTION_INPUT, id,
            PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_DONT_RECONNECT, pwdata->params, 1);
            break;
        }
    } else if (state == PW_STREAM_STATE_STREAMING) {
        xavaSpam("Stream connected/streaming!");
    }
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .state_changed = on_state_changed,
};

void registryEventGlobal(
    void *data, uint32_t id, uint32_t permissions, const char *type,
    uint32_t version, const struct spa_dict *props)
{
    struct pwdata *pwdata = data;

    // Drop invalid props and types
    if(type == NULL || props == NULL)
        return;

    // We only want to process node information
    if(strcmp(type, PW_TYPE_INTERFACE_Node))
        return;

    const char* mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    // Filter out for only audio applications
    if(mediaClass == NULL)
        return;

    bool yes = false;
    size_t i = 0;
    for(; i < arr_count(pwdata->sources); i++) {
        const enum source_type type = pwdata->sources[i].type;
        switch(type) {
            case NONE:
                break;
            case APP_ID:
            case APP_NAME: {
                // Filter out for only audio applications
                if(strcmp(mediaClass, "Stream/Output/Audio"))
                    break;

                const char* appName = spa_dict_lookup(props,
                    (type == APP_ID) ? PW_KEY_APP_ID : PW_KEY_APP_NAME);

                if(appName && !strcmp(appName, pwdata->sources[i].name)) {
                    yes = true;
                    pwdata->sources[i].target_id = id;
                }
                break;
            }
            case MONITOR:
                xavaSpam("Tried monitor!");
                // Filter out for only audio outputs
                if(strcmp(mediaClass, "Audio/Sink"))
                    break;

                yes = true;
                pwdata->sources[i].target_id = id;
                break;
        }
        if(yes) break;
    }

    if(yes) {
        const char* appName = spa_dict_lookup(props, PW_KEY_APP_NAME);
        xavaSpam("Found pipewire sink %s (id=%d) and made it choice %d",
            appName ? appName : "NO_NAME",
            id,
            i+1);

        pw_stream_disconnect(pwdata->stream);
        pw_stream_connect(pwdata->stream, PW_DIRECTION_INPUT, id,
               PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS, pwdata->params, 1);
    }
}

static const struct pw_registry_events REGISTRY_EVENTS = {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = registryEventGlobal,
};

void registerCallback(struct pwdata *pwdata) {
    pwdata->context = pw_context_new(
        pw_main_loop_get_loop(pwdata->loop),
        NULL,
        0);
    assert(pwdata->context);

    pwdata->core = pw_context_connect(pwdata->context, NULL, 0);
    assert(pwdata->core);

    pwdata->registry = pw_core_get_registry(pwdata->core, PW_VERSION_REGISTRY, 0);
    xavaBailCondition(!pwdata->registry, "Failed to obtain pipewire registry, cannot monitor Audio device events. Failing.");

    // Listen for new device or application hotplugs
    pw_registry_add_listener(
        pwdata->registry, &(pwdata->pwRegistryListener), &REGISTRY_EVENTS, pwdata
    );

    // Listen for disconnect/connect events
    pw_stream_add_listener(pwdata->stream, &pwdata->streamListener, &stream_events, pwdata);
}

void deregisterCallback(struct pwdata *pwdata) {
    pw_proxy_destroy((struct pw_proxy*)pwdata->registry);
    pw_core_disconnect(pwdata->core);
    pw_context_destroy(pwdata->context);
}
