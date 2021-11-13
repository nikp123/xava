#include "shared/ionotify.h"
#define TRUE 1
#define FALSE 0

#define _XOPEN_SOURCE_EXTENDED
#include <locale.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else
#include <stdlib.h>
#endif

#include <stdio.h>
#include <stdbool.h>
#if defined(__unix__)||defined(__APPLE__)
    #include <termios.h>
#endif
#include <math.h>
#include <fcntl.h>

#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <dirent.h>
#include <unistd.h>

#ifndef PACKAGE
    #define PACKAGE "INSERT_PROGRAM_NAME"
    #warning "Package name not defined!"
#endif

#ifndef VERSION
    #define VERSION "INSERT_PROGRAM_VERSION"
    #warning "Package version not defined!"
#endif

#include "config.h"
#include "shared.h"

static void*    (*xavaInput)                     (void*); // technically it's "struct audio_data*"
                                                          // but the compiler complains :(
static void     (*xavaInputLoadConfig)           (struct XAVA_HANDLE*);

static void     (*xavaOutputLoadConfig)          (struct XAVA_HANDLE*);
static int      (*xavaInitOutput)                (struct XAVA_HANDLE*);
static void     (*xavaOutputClear)               (struct XAVA_HANDLE*);
static int      (*xavaOutputApply)               (struct XAVA_HANDLE*);
static XG_EVENT (*xavaOutputHandleInput)         (struct XAVA_HANDLE*);
static void     (*xavaOutputDraw)                (struct XAVA_HANDLE*);
static void     (*xavaOutputCleanup)             (struct XAVA_HANDLE*);

static void     (*xavaFilterLoadConfig) (struct XAVA_HANDLE*);
static int      (*xavaFilterInit)                (struct XAVA_HANDLE*);
static int      (*xavaFilterApply)               (struct XAVA_HANDLE*);
static int      (*xavaFilterLoop)                (struct XAVA_HANDLE*);
static int      (*xavaFilterCleanup)             (struct XAVA_HANDLE*);

char *configPath;

static _Bool kys = 0, should_reload = 0;

// teh main XAVA handle (made just to not piss off MinGW)
static struct XAVA_HANDLE xava;

// XAVA magic variables, too many of them indeed
static pthread_t p_thread;

void handle_ionotify_call(XAVA_IONOTIFY_EVENT event, const char *filename,
        int id, struct XAVA_HANDLE *xava) {
    switch(event) {
        case XAVA_IONOTIFY_CHANGED:
        case XAVA_IONOTIFY_DELETED:
            should_reload = 1;
            break;
        case XAVA_IONOTIFY_ERROR:
            xavaBail("ionotify event errored out! Bailing...");
            break;
        case XAVA_IONOTIFY_CLOSED:
            xavaLog("ionotify socket closed");
            break;
        default:
            xavaLog("Unrecognized ionotify call occured!");
            break;
    }
    return;
}

// general: cleanup
void cleanup(void) {
    struct config_params *p     = &xava.conf;
    struct audio_data    *audio = &xava.audio;

    xavaIONotifyKill(xava.ionotify);

    // telling audio thread to terminate
    audio->terminate = 1;

    // waiting for all background threads and other stuff to terminate properly
    xavaSleep(100, 0);

    // kill the audio thread
    pthread_join(p_thread, NULL);

    xavaOutputCleanup(&xava);
    if(!p->skipFilterF)
        xavaFilterCleanup(&xava);

    // destroy modules
    xava_module_free(p->inputModule);
    xava_module_free(p->outputModule);
    xava_module_free(p->filterModule);

    // color information
    free(p->gradient_colors);

    // config file path
    //free(configPath); don't free as the string is ONLY allocated during bootup

    // cleanup remaining FFT buffers (abusing C here)
    switch(audio->channels) {
        case 2:
            free(audio->audio_out_r);
        default:
            free(audio->audio_out_l);
            break;
    }

    // clean the config
    xavaConfigClose(xava.default_config.config);
}

#if defined(__unix__)||defined(__APPLE__)
// general: handle signals
void sig_handler(int sig_no) {
    switch(sig_no) {
        case SIGUSR1:
            should_reload = true;
            break;
        case SIGINT:
            xavaLog("CTRL-C pressed -- goodbye\n");
            kys=1;
            return;
        case SIGTERM:
            xavaLog("Process termination requested -- goodbye\n");
            kys=1;
            return;
    }
}
#endif

// general: entry point
int main(int argc, char **argv)
{
    // general: define variables
    //int thr_id;

    int sleep = 0;
    int i, c, silence;
    char *usage = "\n\
Usage : " PACKAGE " [options]\n\
Visualize audio input in a window. \n\
\n\
Options:\n\
    -p          specify path to the config file\n\
    -v          print version\n\
\n\
Keys:\n\
        Up        Increase sensitivity\n\
        Down      Decrease sensitivity\n\
        Left      Decrease number of bars\n\
        Right     Increase number of bars\n\
        r         Reload config\n\
        c         Cycle foreground color\n\
        b         Cycle background color\n\
        q         Quit\n\
\n\
as of 0.4.0 all options are specified in config file, see in '/home/username/.config/xava/' \n";

    unsigned long oldTime = 0;

    setlocale(LC_ALL, "C");
    setbuf(stdout,NULL);
    setbuf(stderr,NULL);

    // general: handle Ctrl+C
    #if defined(__unix__)||defined(__APPLE__)
        struct sigaction action;
        memset(&action, 0, sizeof(action));
        action.sa_handler = &sig_handler;
        sigaction(SIGINT, &action, NULL);
        sigaction(SIGTERM, &action, NULL);
        sigaction(SIGUSR1, &action, NULL);
    #endif

    configPath = NULL;

    // general: handle command-line arguments
    while ((c = getopt (argc, argv, "p:vh")) != -1) {
        switch (c) {
            case 'p': // argument: fifo path
                configPath = optarg;
                break;
            case 'h': // argument: print usage
                printf ("%s", usage);
                return 1;
            case '?': // argument: print usage
                printf ("%s", usage);
                return 1;
            case 'v': // argument: print version
                printf (PACKAGE " " VERSION "\n");
                return 0;
            default:  // argument: no arguments; exit
                abort ();
        }
    }

    // general: main loop
    while (1) {
        // extract the shorthand sub-handles
        struct config_params     *p     = &xava.conf;
        struct audio_data        *audio = &xava.audio;

        // initialize ioNotify engine
        xava.ionotify = xavaIONotifySetup();

        // load config
        configPath = load_config(configPath, &xava);

        // attach that config to the IONotify thing
        struct xava_ionotify_watch_setup thing;
        thing.xava_ionotify_func = &handle_ionotify_call;
        thing.filename = configPath;
        thing.ionotify = xava.ionotify;
        thing.id = 1;
        thing.xava = &xava;
        xavaIONotifyAddWatch(&thing);

        // load symbols
        xavaInput             = get_symbol_address(p->inputModule, "xavaInput");
        xavaInputLoadConfig   = get_symbol_address(p->inputModule, "xavaInputLoadConfig");

        xavaInitOutput        = get_symbol_address(p->outputModule, "xavaInitOutput");
        xavaOutputClear       = get_symbol_address(p->outputModule, "xavaOutputClear");
        xavaOutputApply       = get_symbol_address(p->outputModule, "xavaOutputApply");
        xavaOutputHandleInput = get_symbol_address(p->outputModule, "xavaOutputHandleInput");
        xavaOutputDraw        = get_symbol_address(p->outputModule, "xavaOutputDraw");
        xavaOutputCleanup     = get_symbol_address(p->outputModule, "xavaOutputCleanup");
        xavaOutputLoadConfig  = get_symbol_address(p->outputModule, "xavaOutputLoadConfig");

        if(!p->skipFilterF) {
            xavaFilterInit       = get_symbol_address(p->filterModule, "xavaFilterInit");
            xavaFilterApply      = get_symbol_address(p->filterModule, "xavaFilterApply");
            xavaFilterLoop       = get_symbol_address(p->filterModule, "xavaFilterLoop");
            xavaFilterCleanup    = get_symbol_address(p->filterModule, "xavaFilterCleanup");
            xavaFilterLoadConfig = get_symbol_address(p->filterModule, "xavaFilterLoadConfig");
        }

        // we're loading this first because I want output modes to adjust audio
        // "renderer/filter" properties

        // load output config
        xavaOutputLoadConfig(&xava);

        // set up audio properties BEFORE the input is initialized
        audio->inputsize = p->inputsize;
        audio->fftsize   = p->fftsize;
        audio->format    = -1;
        audio->terminate = 0;
        audio->channels  = 1 + p->stereo;
        audio->rate      = p->samplerate;
        audio->latency   = p->samplelatency;

        // load input config
        xavaInputLoadConfig(&xava);

        // load filter config
        if(!p->skipFilterF)
            xavaFilterLoadConfig(&xava);

        // setup audio garbo
        MALLOC_SELF(audio->audio_out_l, p->fftsize+1);
        if(p->stereo)
            MALLOC_SELF(audio->audio_out_r, p->fftsize+1);
        if(p->stereo) {
            for (i = 0; i < audio->fftsize; i++) {
                audio->audio_out_l[i] = 0;
                audio->audio_out_r[i] = 0;
            }
        } else for(i=0; i < audio->fftsize; i++) audio->audio_out_l[i] = 0;

        // thr_id = below
        pthread_create(&p_thread, NULL, xavaInput, (void*)audio);

        bool reloadConf = false;

        if(!p->skipFilterF)
            xavaBailCondition(xavaFilterInit(&xava),
                    "Failed to initialize filter! Bailing...");

        xavaBailCondition(xavaInitOutput(&xava),
                "Failed to initialize output! Bailing...");

        xavaIONotifyStart(xava.ionotify);

        while(!reloadConf) { //jumbing back to this loop means that you resized the screen
            // handle for user setting too many bars
            if (p->fixedbars) {
                p->autobars = 0;
                if (p->fixedbars * p->bw + p->fixedbars * p->bs - p->bs > p->w) p->autobars = 1;
            }

            //getting orignial numbers of barss incase of resize
            if (p->autobars == 1)  {
                xava.bars = (p->w + p->bs) / (p->bw + p->bs);

                //if (p->bs != 0) bars = (w - bars * p->bs + p->bs) / bw;
            } else xava.bars = p->fixedbars;

            if (xava.bars < 1) xava.bars = 1; // must have at least 1 bars

            if (p->stereo) { // stereo must have even numbers of bars
                if (xava.bars%2) xava.bars--;
            }

            if(!p->skipFilterF)
                xavaFilterApply(&xava);

            xavaOutputApply(&xava);

            bool resizeWindow = false;
            bool redrawWindow = false;

            while  (!resizeWindow) {
                switch(xavaOutputHandleInput(&xava)) {
                    case XAVA_QUIT:
                        cleanup();
                        return EXIT_SUCCESS;
                    case XAVA_RELOAD:
                        should_reload = 1;
                        break;
                    case XAVA_RESIZE:
                        resizeWindow = TRUE;
                        break;
                    case XAVA_REDRAW:
                        redrawWindow = TRUE;
                        break;
                    default:
                        // ignore other values
                        break;
                }

                if (should_reload) {
                    reloadConf = true;
                    resizeWindow = true;
                    should_reload = 0;
                }

                //if (cont == 0) break;

                // process: populate input buffer and check if input is present
                silence = 1;

                for (i = 0; i < audio->fftsize; i++) {
                    if(audio->audio_out_l[i]) {
                        silence = 0;
                        break;
                    }
                }

                if(p->stereo) {
                    for (i = 0; i < audio->fftsize; i++) {
                        if(audio->audio_out_r[i]) {
                            silence = 0;
                            break;
                        }
                    }
                }

                if (silence == 1) {
                    sleep++;
                } else {
                    sleep = 0;
                    xavaSpamCondition(xava.pauseRendering, "Resuming from sleep");
                }

                // process: if input was present for the last 5 seconds apply FFT to it
                if (sleep < p->framerate * 5) {
                    xava.pauseRendering = false;
                } else if(xava.pauseRendering) {
                    // unless the user requested that the program ends
                    if(kys||should_reload) sleep = 0;

                    // wait 100ms, then check sound again.
                    xavaSleep(100, 0);
                    continue;
                } else { // if in sleep mode wait and continue
                    xavaSpam("Going to sleep!");

                    // signal to any potential rendering threads to stop
                    xava.pauseRendering = true;
                }

                // Calculate the result through filters
                if(!p->skipFilterF) {
                    xavaFilterLoop(&xava);

                    // zero values causes divided by zero segfault
                    // and set max height
                    for (i = 0; i < xava.bars; i++) {
                        if(xava.f[i] < 1)
                            xava.f[i] = 1;
                        else if(xava.f[i] > p->h)
                            xava.f[i] = p->h;
                    }
                }

                // output: draw processed input
                if(redrawWindow) {
                    xavaOutputClear(&xava);

                    // audio output is unallocated without the filter
                    if(!p->skipFilterF)
                        memset(xava.fl, 0x00, sizeof(int)*xava.bars);

                    redrawWindow = FALSE;
                }
                xavaOutputDraw(&xava);

                if(!p->vsync) // the window handles frametimes instead of XAVA
                    oldTime = xavaSleep(oldTime, p->framerate);

                // save previous bar values
                if(!p->skipFilterF)
                    memcpy(xava.fl, xava.f, sizeof(int)*xava.bars);

                if(kys) {
                    resizeWindow=1;
                    reloadConf=1;
                }

                // checking if audio thread has exited unexpectedly
                xavaBailCondition(audio->terminate,
                        "Audio thread has exited unexpectedly.\nReason: %s",
                        audio->error_message);
            } // resize window
        } // reloading config

        // get rid of everything else
        cleanup();

        // since this is an infinite loop we need to break out of it
        if(kys) break;
    }
    return 0;
}
