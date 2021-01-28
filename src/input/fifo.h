#define FIFO_INPUT_NUM 2
#define FIFO_INPUT_NAME "fifo"

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

