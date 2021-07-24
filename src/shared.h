#ifndef __XAVA_SHARED_H
#define __XAVA_SHARED_H

#include <stdlib.h>
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

#define CALLOC_SELF(x, y) (x)=calloc((y), sizeof(*x))
#define MALLOC_SELF(x, y)  (x)=malloc(sizeof(*x)*(y))
#define REALLOC_SELF(x, y) { void (*z)=realloc((x), sizeof(*x)*(y)); \
	xavaBailCondition(!(z), "Failed to reallocate memory"); (x)=(z); }

#include <stdint.h>
#include <stdbool.h>
#include "shared/module.h"
#include "shared/log.h"
#include "shared/ionotify.h"
#include "shared/config.h"
#include "shared/io.h"

// Shared audio data sturct
struct audio_data {
	float			*audio_out_r;
	float			*audio_out_l;
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
	double sens; 
	int32_t fixedbars;
	bool autobars, stereo, autosens; 
	XAVAMODULE *inputModule, *outputModule, *filterModule;
	uint32_t fftsize;

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

	struct config {
		// handle to the config file itself
		XAVACONFIG config;
		XAVAIONOTIFY ionotify;
	} default_config;
};

#endif
