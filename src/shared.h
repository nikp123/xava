#ifndef __XAVA_SHARED_H
#define __XAVA_SHARED_H

#include <stdlib.h>
#include <stdbool.h>

// Audio sensitivity and volume varies greatly between
// different audio, audio systems and operating systems
// This value is used to properly calibrate the sensitivity
// for a certain platform or setup in the Makefile
#ifndef XAVA_PREDEFINED_SENS_VALUE
    #define XAVA_PREDEFINED_SENS_VALUE 0.0005
#endif

// Some common comparision macros
#ifndef MIN
    #define MIN(x,y) ((x)>(y) ? (y):(x))
#endif
#ifndef MAX
    #define MAX(x,y) ((x)>(y) ? (x):(y))
#endif
#ifndef DIFF
    #define DIFF(x,y) (MAX((x),(y))-MIN((x),(y)))
#endif

#define UNUSED(x) (void)(x)

#define CALLOC_SELF(x, y) (x)=calloc((y), sizeof(*x))
#define MALLOC_SELF(x, y)  (x)=malloc(sizeof(*x)*(y))
#define REALLOC_SAFE(x, size){\
   void (*z) = realloc((x), MAX(1, size)); \
   xavaBailCondition(!(z), "Failed to reallocate memory"); \
   (x) = (z); \
}
#define REALLOC_SELF(x, y) {\
    void (*z) = realloc((x), sizeof(*x)*MAX(1, y)); \
    xavaBailCondition(!(z), "Failed to reallocate memory"); \
    (x) = (z); \
}

#ifdef __WIN32__
    #define EXP_FUNC __declspec(dllexport) __attribute__ ((visibility ("default")))
#else
    #define EXP_FUNC __attribute__ ((visibility ("default")))
#endif

#include "shared/util/types.h"
// this array thing is so useful, let's just force it upon everything. thank liv
#include "shared/util/array.h"
#include "shared/module/module.h"
#include "shared/log.h"
#include "shared/ionotify.h"
#include "shared/config/config.h"
#include "shared/config/pywal.h"
#include "shared/io/io.h"
#include "shared/util/version.h"


// configuration parameters
typedef struct XAVA_CONFIG {
    // for internal use only
    XAVA_CONFIG_OPTION(f64,  sens);
    XAVA_CONFIG_OPTION(i32,  fixedbars);
    XAVA_CONFIG_OPTION(bool, autobars);
    XAVA_CONFIG_OPTION(bool, stereo);
    XAVA_CONFIG_OPTION(bool, autosens);

    XAVA_CONFIG_OPTION(u32, fftsize);

    // input/output related options

    // 1 - colors
    XAVA_CONFIG_OPTION(char*, color);
    XAVA_CONFIG_OPTION(char*, bcolor);           // pointer to color string
    // col = foreground color, bgcol = background color
    XAVA_CONFIG_OPTION(u32, col);
    XAVA_CONFIG_OPTION(u32, bgcol);                 // ARGB 32-bit value
    XAVA_CONFIG_OPTION(f64, foreground_opacity);
    XAVA_CONFIG_OPTION(f64, background_opacity);    // range 0.0-1.0

    // 2 - gradients
    XAVA_CONFIG_OPTION(char**, gradients); // array of pointers to color string

    // 3 - timing
    XAVA_CONFIG_OPTION(i32, framerate); // limit xava to a specific framerate
                                        // (can be changed mid-run)
    XAVA_CONFIG_OPTION(i32, vsync);     // 0 = disabled, while enabled, XAVA
                                        // will rely upon the timer function
                                        // provided by your Vsync call
                                        // (PLEASE DESIGN YOUR IMPLEMENTATION
                                        // LIKE THIS)
                                        // Positive values = divisors for your
                                        // monitors real refresh-rate
                                        // -1 = Adaptive Vsync

    // 4 - geometry
    XAVA_CONFIG_OPTION(u32, bw);
    XAVA_CONFIG_OPTION(u32, bs);        // bar width and spacing
    XAVA_CONFIG_OPTION(u32, w);
    XAVA_CONFIG_OPTION(u32, h);         // configured window width and height
    XAVA_CONFIG_OPTION(i32, x);
    XAVA_CONFIG_OPTION(i32, y);         // x and y padding

    XAVA_CONFIG_OPTION(char*, winA);    // pointer to a string of alignment

    // 5 - audio
    XAVA_CONFIG_OPTION(u32, inputsize); // size of the input audio buffer
                                                    // must be a power of 2
    XAVA_CONFIG_OPTION(u32, samplerate);    // the rate at which the audio is sampled
    XAVA_CONFIG_OPTION(u32, samplelatency); // input will try to keep to copy chunks of this size

    // 6 - special flags
    struct {
        XAVA_CONFIG_OPTION(bool, fullscreen);
        XAVA_CONFIG_OPTION(bool, transparency);
        XAVA_CONFIG_OPTION(bool, border);
        XAVA_CONFIG_OPTION(bool, beneath);
        XAVA_CONFIG_OPTION(bool, interact);
        XAVA_CONFIG_OPTION(bool, taskbar);
        XAVA_CONFIG_OPTION(bool, holdSize);

        // not real config options, soom to be moved to the XAVA handle
        XAVA_CONFIG_OPTION(bool, skipFilter);
        XAVA_CONFIG_OPTION(bool, ignoreWindowSize);
    } flag;
} XAVA_CONFIG;

typedef struct XAVA_FILTER {
    struct {
        void (*load_config) (XAVA*);
        int  (*init)        (XAVA*);
        int  (*apply)       (XAVA*);
        int  (*loop)        (XAVA*);
        int  (*cleanup)     (XAVA*);
    } func;

    XAVAMODULE *module;
    // i know void pointers are footguns but consider that this
    // pointer type is GURANTEED to be mutable
    void       *data;
} XAVA_FILTER;

typedef struct XAVA_OUTPUT {
    struct {
        void     (*load_config)  (XAVA*);
        int      (*init)         (XAVA*);
        void     (*clear)        (XAVA*);
        int      (*apply)        (XAVA*);
        XG_EVENT (*handle_input) (XAVA*);
        void     (*draw)         (XAVA*);
        void     (*cleanup)      (XAVA*);
    } func;

    XAVAMODULE *module;
    void       *data;
} XAVA_OUTPUT;

// Shared audio data sturct
typedef struct XAVA_AUDIO {
    struct {
        void     (*load_config) (XAVA*);
        void*    (*loop)        (void*);
    } func;

    XAVAMODULE *module;
    void       *data;

    float        *audio_out_r;
    float        *audio_out_l;
    int32_t      format;
    uint32_t     rate;
    char         *source;               // alsa device, fifo path or pulse source
    uint32_t     channels;
    bool         terminate;             // shared variable used to terminate audio thread
    char         error_message[1024];
    uint32_t     inputsize, fftsize;    // inputsize and fftsize
    uint32_t     latency;               // try to keep (this) latency in samples
} XAVA_AUDIO;

// XAVA handle
typedef struct XAVA {
    // variables that XAVA outputs
    uint32_t bars;        // number of output bars
    uint32_t rest;        // number of screen units until first bar
    uint32_t *f, *fl;    // array to bar data (f = current, fl = last frame)

    // signals to the renderer thread to safely stop rendering (if needed)
    bool pauseRendering;

    // handles to both config variables and the audio state
    XAVA_AUDIO  audio;   // TODO: rename to input when brave enough
    XAVA_FILTER filter;
    XAVA_OUTPUT output;

    XAVA_CONFIG conf;

    struct config {
        // handle to the config file itself
        xava_config_source        config;
    } default_config;

    // visualizer size INSIDE of the window
    struct dimensions {
        int32_t  x, y; // display offset in the visualizer window
        uint32_t w, h; // window dimensions
    } inner, outer;

    // since inner screen space may not match with bar space
    // this exists to bridge that gap
    struct bar_space {
        uint32_t w, h;
    } bar_space;

    XAVAIONOTIFY ionotify;
} XAVA;

#endif
