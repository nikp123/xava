#define BUFSIZE 1024
#define FFTSIZE 16384

struct audio_data {
	int audio_out_r[FFTSIZE];
	int audio_out_l[FFTSIZE];
	int format;
	unsigned int rate;
	char *source;				// alsa device, fifo path or pulse source
	int im;						// input mode alsa, fifo or pulse
	int channels;
	int terminate;				// shared variable used to terminate audio thread
	char error_message[1024];
};

// header files for fifo, part of cava
void* input_fifo(void* data);
