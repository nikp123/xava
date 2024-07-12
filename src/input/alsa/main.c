#include <stdbool.h>

#include <alsa/asoundlib.h>
#include <sys/types.h>
#include <dirent.h>

#include "shared.h"

// input: ALSA

static void initialize_audio_parameters(snd_pcm_t** handle, XAVA_AUDIO* audio,
snd_pcm_uframes_t* frames) {
    // alsa: open device to capture audio
    int err = snd_pcm_open(handle, audio->source, SND_PCM_STREAM_CAPTURE, 0);
    xavaBailCondition(err<0, "Error opening ALSA stream: %s", snd_strerror(err));
    xavaSpam("Opening ALSA stream successful");

    snd_pcm_hw_params_t* params;
    snd_pcm_hw_params_alloca(&params); // assembling params
    snd_pcm_hw_params_any(*handle, params); // setting defaults or something

    // interleaved mode right left right left
    snd_pcm_hw_params_set_access(*handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    // trying to set 16bit
    snd_pcm_hw_params_set_format(*handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(*handle, params, audio->channels);
    unsigned int sample_rate = audio->rate;

    // trying our rate
    snd_pcm_hw_params_set_rate_near(*handle, params, &sample_rate, NULL);

    // number of frames pr read
    snd_pcm_hw_params_set_period_size_near(*handle, params, frames, NULL);
    err = snd_pcm_hw_params(*handle, params); // attempting to set params
    xavaBailCondition(err<0, "Failed to set hardware parameters: %s",
            snd_strerror(err));

    xavaBailCondition((err = snd_pcm_prepare(*handle))<0,
            "Failed to prepare audio interface for use: %s", snd_strerror(err));

    // getting actual format
    snd_pcm_hw_params_get_format(params, (snd_pcm_format_t*)&sample_rate);
    // converting result to number of bits
    if (sample_rate <= 5)
        audio->format = 16;
    else if (sample_rate <= 9 )
        audio->format = 24;
    else
        audio->format = 32;
    snd_pcm_hw_params_get_rate(params, &audio->rate, NULL);
    snd_pcm_hw_params_get_period_size(params, frames, NULL);
    // snd_pcm_hw_params_get_period_time(params, &sample_rate, &dir);
}

static int get_certain_frame(signed char* buffer, int buffer_index, int adjustment) {
    // using the 10 upper bits this would give me a vert res of 1024, enough...
    int temp = buffer[buffer_index + adjustment - 1] << 2;
    int lo = buffer[buffer_index + adjustment - 2] >> 6;
    if (lo < 0)
        lo = abs(lo) + 1;
    if (temp >= 0)
        temp += lo;
    else
        temp -= lo;
    return temp;
}

static void fill_audio_outs(XAVA_AUDIO* audio, signed char* buffer,
    const int size) {
    int radj = audio->format / 4; // adjustments for interleaved
    int ladj = audio->format / 8;
    static int audio_out_buffer_index = 0;
    // sorting out one channel and only biggest octet
    for (int buffer_index = 0; buffer_index < size; buffer_index += ladj * 2) {
        // first channel
        int tempr = get_certain_frame(buffer, buffer_index, radj);
        // second channel
        int templ = get_certain_frame(buffer, buffer_index, ladj);

        // mono: adding channels and storing it in the buffer
        if (audio->channels == 1)
            audio->audio_out_l[audio_out_buffer_index] = (templ + tempr) / 2;
        else { // stereo storing channels in buffer
            audio->audio_out_l[audio_out_buffer_index] = templ;
            audio->audio_out_r[audio_out_buffer_index] = tempr;
        }

        ++audio_out_buffer_index;
        audio_out_buffer_index %= audio->inputsize;
    }
}

static bool is_loop_device_for_sure(const char * text) {
    const char * const LOOPBACK_DEVICE_PREFIX = "hw:Loopback,";
    return strncmp(text, LOOPBACK_DEVICE_PREFIX, strlen(LOOPBACK_DEVICE_PREFIX)) == 0;
}

static bool directory_exists(const char * path) {
    DIR * const dir = opendir(path);
    bool exists;// = dir != NULL;
    if (dir == NULL) exists = 0;
    else exists = 1;
    closedir(dir);
    return exists;
}

EXP_FUNC void* xavaInput(void* data) {
    int err;
    XAVA_AUDIO* audio = (XAVA_AUDIO*)data;
    snd_pcm_t* handle;
    snd_pcm_uframes_t buffer_size;
    snd_pcm_uframes_t period_size;
    snd_pcm_uframes_t frames = audio->latency;

    if(is_loop_device_for_sure(audio->source)) {
        if(directory_exists("/sys/")) {
            xavaBailCondition(!directory_exists("/sys/module/snd_aloop/"),
                    "Linux kernel module 'snd_aloop' does not seem to be loaded!\n"
                    "Maybe run \"sudo modprobe snd_aloop\"");
        }
    }

    initialize_audio_parameters(&handle, audio, &frames);
    snd_pcm_get_params(handle, &buffer_size, &period_size);

    int16_t buf[period_size];
    frames = period_size / ((audio->format / 8) * audio->channels);
    audio->latency = period_size;\

    xavaSpam("channels: %d, samplerate: %d, latency: %d",
            audio->channels, audio->rate, audio->latency);

    // frames * bits/8 * channels
    //const int size = frames * (audio->format / 8) * audio->channels;
    signed char* buffer = malloc(period_size);
    uint32_t n = 0;

    while (1) {
        switch (audio->format) {
            case 16:
                err = snd_pcm_readi(handle, buf, frames);

                if(audio->channels == 1) {
                    for(snd_pcm_uframes_t i = 0; i < frames; i++) {
                        audio->audio_out_l[n] = buf[i];
                        n++;
                        if(n == audio->inputsize - 1) n = 0;
                    }
                } else if(audio->channels == 2) {
                    for(uint32_t i = 0; i < frames * 2; i += 2) {
                        //stereo storing channels in buffer
                        audio->audio_out_l[n] = buf[i];
                        audio->audio_out_r[n] = buf[i + 1];
                    }
                }
                break;
            default:
                err = snd_pcm_readi(handle, buffer, frames);
                fill_audio_outs(audio, buffer, period_size);
                break;
        }

        if(err == -EPIPE) {
            /* EPIPE means overrun */
            xavaError("Buffer overrun detected!\n");
            snd_pcm_prepare(handle);
        } else if(err < 0) {
            xavaError("Read error: %s", snd_strerror(err));
        } else if(err != (int)frames) {
            xavaError("Short read. Read %d instead of %d frames!", err, (int)frames);
        }

        if (audio->terminate == 1) {
            free(buffer);
            snd_pcm_close(handle);
            return NULL;
        }
    }
}

EXP_FUNC void xavaInputLoadConfig(XAVA *xava) {
    XAVA_AUDIO *audio = &xava->audio;
    xava_config_source config = xava->default_config.config;
    audio->source = (char*)xavaConfigGetString(config, "input", "source", "Loopback,1");
}

