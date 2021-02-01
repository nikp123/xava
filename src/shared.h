// Audio sensitivity and volume varies greatly between 
// different audio, audio systems and operating systems
// This value is used to properly calibrate the sensitivity
// for a certain platform or setup in the Makefile
#ifndef XAVA_PREDEFINED_SENS_VALUE
	#define XAVA_PREDEFINED_SENS_VALUE 0.0005
#endif

int xavaMkdir(char *dir);
int xavaGetConfigDir(char *configPath);
char *xavaGetInstallDir(void);
unsigned long xavaSleep(unsigned long oldTime, int framerate);

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

