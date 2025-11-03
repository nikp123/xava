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

/**
    This is the module responsible for turning your raw audio data into something that is
    displayable on a screen. 

    To nerdify the statement: You turn your audio samples into screen units.

    Not exactly the definition of filter, I know. But I wrote this before I took my
    signals processing class.

    It's usually designed to be an FFT, but can be any type of audio processing really.

    You can safely assume that XAVA_INPUT is doing it's job and all the things it's supposed
    to give you, it does.
**/
typedef struct XAVA_FILTER {
    struct {
        // This function processes the filters own configuration value and spits out any
        // inconsistencies before it has a chance to even run.
        // The state given here isn't valid for the lifetime of the program and should
        // be almost entirely ignored. Spawn your file watcher threads in init instead.
        // Same story as XAVA_OUTPUTs' load_config
        void (*load_config) (XAVA*);

        // Initialize the static parts of the filter.
        // Meaning: Parts that don't really change throughout the runtime.
        // Your FFT engine and other things that only need to be initialized once
        // for the lifetime of the program and are not expected to change.
        int  (*init)        (XAVA*);

        // Heres where you initialize/update things that are dynamic
        // Assumes init has already ran before. This function will get called upon
        // every XAVA event justifying any particular change, like the number of available
        // bars to render to. You need to be able to handle that.
        int  (*apply)       (XAVA*);

        // Heres your processing loop, this is where you actually do the thing.
        // Assumes both init and apply ran at least once, in that order.
        int  (*loop)        (XAVA*);

        // Release all resources used by this module. Your module is expected to
        // be able to initialize once again without any memory leaks or bugs.
        // Keep that in mind. Don't try to optimize restarts here, the whole point
        // is to have the program fully reset without relaunching the main executable.
        int  (*cleanup)     (XAVA*);
    } func;

    // The internal way XAVA tracks this module. It's not your job as a module dev to poke
    // this around. You may explode the whole program by doing so.
    XAVAMODULE *module;
    
    // Read about XAVA_OUTPUTs' data. I don't want duplicate explainations.
    void       *data;
} XAVA_FILTER;

/**
    I should really make a design document, but here goes:

    This structure represents everything having to deal with outputs in general.
    The func structure contains function pointers of the output that's being currently used.
    The module is the handle from which the function pointers are being pulled from.

    void *data is interesting, because here, it's supposed to hold arbritary runtime data
    for said module. BY DESIGN, this should only ever be allocated by the module itself and
    NOTHING ELSE should touch it.
    Think of it as a saner way to have stack variables which hopefully do not conflict with
    the rest of the project. This needs to be a void* because every module has different needs
    and we can't possibly cover that without overengineering a type. This will have to do.

    Yes, we DO want to get rid of stack variables because their behaviour can mess stuff up
    and we do not want that.
**/
typedef struct XAVA_OUTPUT {
    struct {
        /**
            This function is supposed to handle the configuration being loaded in while
            the module itself is being set up. Any runtime data being here is INHERENTLY
            UNSAFE and thus should be avoided. This means, NO FILE WATCHERS here. Those
            rely on an **runtime** event being fired for the processing to happen. Any
            reference being given here is only valid for the lifetime of this function.

            "rust infected my brain and now i have to think about these things" aah

            Note for myself: Consider maybe having seperate application config and state
            structures, that way this confusion would be less likely to occur, even to
            yourself let alone other developers contributing to this project.
        **/
        void     (*load_config)  (XAVA*);

        /**
            This function is only ran once. It's supposed to initialize whatever output
            you're going to use. But only the static parts.

            What are the static parts? Think of your window handles and your OpenGL context(s).
            Those do not change throughout the runtime and are only created once per the
            lifetime of the program.

            You should be able to have *some* of the live application state data here,
            because by this point XAVA knows what modules are available and is actively
            listening for events and responding to them. And the internal processing buffers
            should be alive by this point.
         */
        int      (*init)         (XAVA*);

        /**
            Clears the output buffer. As simple as it gets really.
            Do however note the difference between this and apply.
            Very important to not mix these up.

            This also implies that the signal being displayed was reset to a zero(ed) state.
         **/
        void     (*clear)        (XAVA*);
        /**
            Apply sets up and/or changes active parts, as opposed to static ones.
            (See init function for explaination)
            This would be your texture buffers and your geometry meshes, for example.
            These get swapped every time a user changes the window size, moves the window,
            provides an input event or such. You should ABSOLUTELY handle those here,
            because if you don't the application will stop being responsive and your
            users won't like you.

            Be careful about dynamic allocations here, you can easily introduce memory leaks
            or memory corruption bugs here. Remember, this function runs on every user
            introduced change, meaning it should be pretty fast as well. You REALLY don't
            want to hang around here for more than 10s of milliseconds.

            Don't put stuff like shader compilations here. Those are way too slow, init
            should be a better fit. And if you want the app to respond to user changes
            of the shaders, better to restart than to handle those dynamically, honestly.
            The whole startup process takes under a second most of the time, unless you're
            on a really slow computer.

            Also: Apply assumes clear was run as well, err.. I think.
         **/
        int      (*apply)        (XAVA*);
        /**
            Handle input, handles input..... Ok, this is a function that you as a developer 
            of an output module allow your users to control the visaulizer with.

            Handle your mouse events, window drags, window resizes, keyboard strokes...etc. here.
            And return the proper XG_EVENT type corresponding to what you want XAVA to do and
            most importantly your **apply** funciton. Because it's the one that catches whatever
            gets spat out of this function.
        **/
        XG_EVENT (*handle_input) (XAVA*);

        /**
            Render a visualizer frame. This assumes both init and apply were ran AT LEAST once.

            It really is as simple as that.
            The delay and timing logic is handled internally, please don't do that here.

            And yea do everything fast here.
        **/
        void     (*draw)         (XAVA*);

        /**
            Once the mothership has decided to sink, this function gets called.

            Safely remove and unallocate every resource that you got or made ever in order
            to prevent a memory leak.

            But isn't the program exiting? Not quite, once you clean up it should be reasonably
            expected that your module can be safely reinitialized again after this function ran
            by doing the same steps as it took to initialize this module the first time over.

            Don't be a smartass, don't try to save execution time here. Let init do the same
            steps it would do the first time. This way we ensure a consistent state every
            time we start up.
        **/
        void     (*cleanup)      (XAVA*);
    } func;

    // The module handle that XAVA uses internally to track the executable binary of your module
    // You as the application developer should **never** have to touch this
    XAVAMODULE *module;

    /** 
        This is the fun one, this is the "state handle" that XAVA is giving you.

        Absolfuckinglutely avoid using globally defined variables whenever you can 
        or I will hunt you down.

        Yes this could be quite anything you imagine, however you're expected to take great
        care of what you got because its the ONLY thing that you got. And yea, I expect 
        all of your functions to safely handle this, otherwise we got a problem.
    **/
    void       *data;
} XAVA_OUTPUT;

/**
    Ideally this should be renamed to XAVA_INPUT but for now it's audio
    because thats the only thing its capturing so far.

    This is your input source, your music being blasted through your ears.

    We need a way to capture that and this is what it does.
**/
typedef struct XAVA_AUDIO {
    // Functions that you should define inside of your module
    struct {
        /**
            This processes the input module specifc configuration
            and denotes any changes in its internal buffers.
            Use the provided values in this struct for said purpose.
            Don't set up file watchers here for the same reason 
            you shouldn't for outputs as well.

            The XAVA handle given here is not guranteed to be thread-safe.
        **/
        void     (*load_config) (XAVA*);

        /**
            This is a better location to add file watchers.

            Anyway, here you should spawn a thread that listens to the audio
            and updates the buffers appropriately.
        **/
        void*    (*loop)        (void*);
    } func;

    /**
        How XAVA interally tracks the binary of your input module.
        As a module developer you should not have to touch this.
    **/
    XAVAMODULE *module;

    /**
        This is where you keep misc module data.
        Look into the XAVA_OUTPUTs' equivalent for a more detailed
        explaination of whats the idea behind this.
    **/
    void         *data;

    /**
        The captured audio samples in both left and right samples form
        The size of these buffers is defined by inputsize
    **/
    float        *audio_out_r;
    float        *audio_out_l;

    // The size of the buffer (in samples) holding your incoming samples
    uint32_t     inputsize;

    /**
        The size of the FFT that gets rendered by the filter module.
        I have no fucking idea why this is here, should be very muched moved away
        because it's not relevant here.
    **/
    uint32_t     fftsize;

    /**
        Sample format. Not really defined atm, this should be fixed to mean
        something (when I get to it)
    **/
    int32_t      format;

    /**
        Sample rate of the incoming audio. This is to signal to the XAVA logic
        how it should handle this incoming audio.
    **/
    uint32_t     rate;

    // Source device/name of the incoming audio as defined by the configuration.
    char         *source;

    /**
        Number of captured channels. I do not get the logic behind this either, will
        have to investigate what's the intention behind this (TODO)
    **/
    uint32_t     channels;

    /**
        You as the input module should keep the input latency down to this many samples.
        Think of it as a way for the user to control the input latency (if that is even
        possible). Reducing this increases CPU load massively, but should improve visualizer
        responsiveness (to an extent). Increasing this will cause audio lag, but it should
        also help with the CPU usage.
        In more simple terms: This is how many samples available samples you copy at a time.
    **/
    uint32_t     latency;

    /**
        Inside of the thread you spawn in your loop function, if this ever gets true, you
        should quit as soon as you can. If you don't, you WILL CRASH.
    **/
    bool         terminate;

    // Don't ask. Netiher do I understand.
    char         error_message[1024];
} XAVA_AUDIO;

// XAVA handle
typedef struct XAVA {
    // variables that XAVA outputs
    uint32_t bars;        // number of output bars
    uint32_t rest;        // number of screen units until first bar
    uint32_t *f, *fl;     // array to bar data (f = current, fl = last frame)

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

    xava_ionotify ionotify;
} XAVA;

#endif
