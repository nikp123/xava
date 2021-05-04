#ifndef __XAVA_SHARED_H
#define __XAVA_SHARED_H

#include <stdbool.h>

#define EXP_FUNC __attribute__ ((visibility ("default")))

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

#ifdef INIPARSER
	#include "../lib/iniparser/src/iniparser.h"
#else
	#include <iniparser.h>
#endif

#include <stdint.h>
#include <stdbool.h>
#include "module.h"
#include "log.h"

// XAVA event stuff
typedef enum XAVA_GRAHPICAL_EVENT {
	XAVA_REDRAW, XAVA_IGNORE, XAVA_RESIZE, XAVA_RELOAD,
	XAVA_QUIT
} XG_EVENT;

typedef struct XAVA_GRAHPICAL_EVENT_STACK {
	int pendingEvents;
	XG_EVENT *events;
} XG_EVENT_STACK;

extern void            pushXAVAEventStack    (XG_EVENT_STACK *stack, XG_EVENT event);
extern XG_EVENT        popXAVAEventStack     (XG_EVENT_STACK *stack);
extern XG_EVENT_STACK *newXAVAEventStack     ();
extern void            destroyXAVAEventStack (XG_EVENT_STACK *stack);
extern _Bool           pendingXAVAEventStack (XG_EVENT_STACK *stack);
extern _Bool           isEventPendingXAVA    (XG_EVENT_STACK *stack, XG_EVENT event);

int xavaMkdir(char *dir);
int xavaGetConfigDir(char *configPath);
char *xavaGetInstallDir(void);
unsigned long xavaSleep(unsigned long oldTime, int framerate);

// This funcion is in config.h
// Yes, I commited a sin. Don't shoot, please.
dictionary *get_config_pointer(void);

// Shared audio data sturct
struct audio_data {
	double			*audio_out_r;
	double			*audio_out_l;
	int				format;
	unsigned int	rate;
	char			*source;				// alsa device, fifo path or pulse source
	int				channels;
	bool			terminate;				// shared variable used to terminate audio thread
	char			error_message[1024];
	int				inputsize, fftsize;		// inputsize and fftsize
};

// configuration parameters
struct config_params {
	// for internal use only
	double monstercat, integral, gravity, ignore, sens, logScale, logBegin, logEnd,
 eqBalance; 
	double* smooth;
	uint32_t lowcf, highcf, fftsize;
	int32_t smcount, overshoot, fixedbars, waves;
	bool autobars, stereo, autosens, oddoneout; 
	XAVAMODULE *inputModule, *outputModule, *filterModule;

	// input/output related options

	// 1 - colors
	char*  color, *bcolor, *shadow_color;			// pointer to color string
	char** gradient_colors;							// array of pointers to color string
	uint32_t col, bgcol, shdw_col;					// ARGB 32-bit value
	double foreground_opacity, background_opacity;	// range 0.0-1.0

	// 2 - shadows/gradients
	// shdw = shadow, col = foreground color, bgcol = background color
	uint32_t shdw;									// 0 for no shadows while subsequent
													// values describe the size in pixels
	uint32_t gradients;								// 0 for none, subsequent values increase

	// 3 - timing
	int32_t framerate;								// limit xava to a specific framerate 
													// (can be changed mid-run)
	int32_t vsync;									// 0 = disabled, while enabled, XAVA
													// will rely upon the timer function
													// provided by your Vsync call 
													// (PLEASE DESIGN YOUR IMPLEMENTATION
													// LIKE THIS)
													// Positive values = divisors for your
													// monitors real refresh-rate
													// -1 = Adaptive Vsync

	// 4 - geometry
	uint32_t bw, bs;								// bar width and height
	int32_t w, h;									// display width and height
	int wx, wy;										// window position on the desktop in
													// x and y coordinates
	char *winA;										// pointer to a string of alignment

	// 5 - audio
	uint32_t inputsize;								// size of the input audio buffer
													// must be a power of 2

	// 6 - special flags 
	_Bool fullF, transF, borderF, bottomF, interactF, taskbarF;
	// fullF = fullscreen toggle, transF = transparency toggle,
	// borderF = window border toggle, interactF = interaction
	// toggle, taskbarF = taskbar icon toggle
};

// this is cringe but my linker isn't cooperating,
// so this will have to do
struct function_pointers {
	void            (*pushXAVAEventStack)    (XG_EVENT_STACK *stack, XG_EVENT event);
	XG_EVENT        (*popXAVAEventStack)     (XG_EVENT_STACK *stack);
	XG_EVENT_STACK* (*newXAVAEventStack)     ();
	void            (*destroyXAVAEventStack) (XG_EVENT_STACK *stack);
	_Bool           (*pendingXAVAEventStack) (XG_EVENT_STACK *stack);
	_Bool           (*isEventPendingXAVA)    (XG_EVENT_STACK *stack, XG_EVENT event);
};

// XAVA handle 
struct XAVA_HANDLE {
	// variables that XAVA outputs
	int bars;		// number of output bars
	int rest;		// number of screen units until first bar
	int *f, *fl;	// array to bar data (f = current, fl = last frame)

	// signals to the renderer thread to safely stop rendering (if needed)
	bool pauseRendering;

	// handles to both config variables and the audio state
	struct audio_data audio;
	struct config_params conf;
	struct function_pointers func;
};

#endif
