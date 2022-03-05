#include <bits/floatn-common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <cubeb/cubeb.h>

#include "shared.h"

struct cubeb_data {
    XAVA_AUDIO *audio;
    double accumulator;

    // C-based cringe
    struct audio_str {
        char rate[32];
        char channels[32];
    } audio_str;

    long progress;
    bool autoconnect;
} cubeb_data;

char *cubeb_strerr(int error) {
    switch(error) {
        case CUBEB_OK:                        return "No error";
        case CUBEB_ERROR:                     return "Unclassifed error";
        case CUBEB_ERROR_INVALID_FORMAT:      return "Invalid format";
        case CUBEB_ERROR_INVALID_PARAMETER:   return "Invalid parameter";
        case CUBEB_ERROR_NOT_SUPPORTED:       return "Not supported";
        case CUBEB_ERROR_DEVICE_UNAVAILABLE:  return "Device not available";
    }
    return "Unknown error, probably a bug!";
}

long data_cb(cubeb_stream *stm, void *data,
        const void * input_buffer, void * output_buffer, long nframes) {
    struct cubeb_data *cubeb = data;
    XAVA_AUDIO *audio = cubeb->audio;

    int16_t *in = input_buffer;

    for (int i = 0; i < nframes; ++i) {
        audio->audio_out_l[cubeb->progress] = in[i];
        if(cubeb->progress < audio->inputsize)
            cubeb->progress++;
        else cubeb->progress = 0;
    }
    return nframes;
}

void state_cb(cubeb_stream * stm, void *data, cubeb_state state) {
    //xavaSpam("state=%d\n", state);
}

EXP_FUNC void* xavaInput(void *audiodata) {
    cubeb_data.audio = audiodata;
    cubeb_data.progress = 0;

    cubeb_set_log_callback(CUBEB_LOG_VERBOSE, NULL);

    cubeb * app_ctx;
    cubeb_init(&app_ctx, "XAVA", NULL);

    int rv;
    uint32_t rate;
    uint32_t latency_frames;
    uint64_t ts;

    rv = cubeb_get_preferred_sample_rate(app_ctx, &rate);
    xavaBailCondition(rv != CUBEB_OK, "Could not get preferred sample-rate: %s", cubeb_strerr(rv));

    cubeb_stream_params input_params;
    input_params.format = CUBEB_SAMPLE_S16LE;
    input_params.rate = rate;
    input_params.channels = 1;
    input_params.layout = CUBEB_LAYOUT_UNDEFINED;
    input_params.prefs = CUBEB_STREAM_PREF_LOOPBACK;

    rv = cubeb_get_min_latency(app_ctx, &input_params, &latency_frames);
    xavaBailCondition(rv != CUBEB_OK, "Could not get minimum latency: %s", cubeb_strerr(rv));

    cubeb_stream * stm;
    rv = cubeb_stream_init(app_ctx, &stm, "Audio capture process",
            NULL, &input_params,
            NULL, NULL,
            latency_frames,
            data_cb, state_cb,
            &cubeb_data);
    xavaBailCondition(rv != CUBEB_OK, "Could not open the stream: %s", cubeb_strerr(rv));

    rv = cubeb_stream_start(stm);
    xavaBailCondition(rv != CUBEB_OK, "Could not start the stream: %s", cubeb_strerr(rv));

    while(1) {
        //cubeb_stream_get_position(stm, &ts);
        //xavaSpam("time=%llu\n", ts);

        // the ideal CHAD loop, thanks mozilla
        xavaSleep(5, 0);
        if(cubeb_data.audio->terminate == 1) break;
    }

    rv = cubeb_stream_stop(stm);
    xavaBailCondition(rv != CUBEB_OK, "Could not stop the stream: %s", cubeb_strerr(rv));

    cubeb_stream_destroy(stm);
    cubeb_destroy(app_ctx);

    return 0;
}

EXP_FUNC void xavaInputLoadConfig(XAVA *xava) {
    XAVA_AUDIO *audio = &xava->audio;
    xava_config_source config = xava->default_config.config;

    // default to ignore or change to keep 'em searching
    audio->source = (char*)xavaConfigGetString(config, "input", "source", "default");
}

