// Audio sensitivity and volume varies greatly between 
// different audio, audio systems and operating systems
// This value is used to properly calibrate the sensitivity
// for a certain platform or setup in the Makefile
#ifndef XAVA_PREDEFINED_SENS_VALUE
	#define XAVA_PREDEFINED_SENS_VALUE 0.0005
#endif

// This is changed by the CMake build file, don't touch :)
#ifndef XAVA_DEFAULT_INPUT
	#define XAVA_DEFAULT_INPUT "pulseaudio"
#endif
#ifndef XAVA_DEFAULT_OUTPUT
	#define XAVA_DEFAULT_OUTPUT "x11"
#endif

#ifdef INIPARSER
	#include "../lib/iniparser/src/iniparser.h"
#else
	#include <iniparser.h>
#endif

int xavaMkdir(char *dir);
int xavaGetConfigDir(char *configPath);
char *xavaGetInstallDir(void);
unsigned long xavaSleep(unsigned long oldTime, int framerate);

// This funcion is in config.h
// Yes, I commited a sin. Don't shoot, please.
dictionary *get_config_pointer(void);

// Shared audio data sturct
struct audio_data {
	double *audio_out_r;
	double *audio_out_l;
	int format;
	unsigned int rate;
	char *source;				// alsa device, fifo path or pulse source
	int im;						// input mode alsa, fifo or pulse
	int channels;
	int terminate;				// shared variable used to terminate audio thread
	char error_message[1024];
	int inputsize, fftsize;
};

typedef enum XAVA_GRAHPICAL_EVENT {
	XAVA_REDRAW, XAVA_IGNORE, XAVA_RESIZE, XAVA_RELOAD,
	XAVA_QUIT
} XG_EVENT;
