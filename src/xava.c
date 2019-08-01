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

#include <fftw3.h>
#define max(a,b) \
	 ({ __typeof__ (a) _a = (a); \
			 __typeof__ (b) _b = (b); \
		 _a > _b ? _a : _b; })
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <dirent.h>

#ifdef ALSA
#include "input/alsa.h"
#endif

#ifdef PULSE
#include "input/pulse.h"
#endif

#ifdef XLIB
#include "output/graphical_x.h"
#endif

#ifdef SDL
#include "output/graphical_sdl.h"
#endif

#ifdef SNDIO
#include "input/sndio.h"
#endif

#ifdef PORTAUDIO
#include "input/portaudio.h"
#endif

#ifdef SHMEM
#include "input/shmem.h"
#endif

#ifdef WIN
#include "output/graphical_win.h"
#include "input/wasapi.h"
#endif

#ifdef INIPARSER
	#include "../lib/iniparser/src/iniparser.h"
#else
	#include <iniparser.h>
#endif

#include "output/graphical.h"
#include "output/raw.h"
#include "input/fifo.h"
#include "config.h"

#ifdef __GNUC__
// curses.h or other sources may already define
#undef  GCC_UNUSED
#define GCC_UNUSED __attribute__((unused))
#else
#define GCC_UNUSED /* nothing */
#endif

#if defined(__unix__)||defined(__APPLE__)
	struct termios oldtio, newtio;
#endif

long oldTime; // used to calculate frame times

int rc;
int M;
int kys = 0;

// whether we should reload the config or not
int should_reload = 0;

// general: cleanup
void cleanup() {
	switch(p.om) {
		#ifdef XLIB
		case 5:
			cleanup_graphical_x();
			break;
		#endif
		#ifdef SDL
		case 6:
			cleanup_graphical_sdl();
			break;
		#endif
		#ifdef WIN
		case 7:
			cleanup_graphical_win();
			break;
		#endif
		default: break;
	}
}

unsigned long xavaSleep(unsigned long oldTime, int framerate) {
	unsigned long newTime = 0;
	if(framerate) {
	#ifdef WIN
		SYSTEMTIME time;
		GetSystemTime(&time);
		newTime = time.wSecond*1000+time.wMilliseconds;
		if(newTime-oldTime<1000/framerate&&newTime>oldTime && p.vsync)
			Sleep(1000/framerate-(newTime-oldTime));
		GetSystemTime(&time);
		return time.wSecond*1000+time.wMilliseconds;
	#else
		struct timeval tv;
		gettimeofday(&tv, NULL);
		newTime = tv.tv_sec*1000+tv.tv_usec/1000;
		if(oldTime+1000/framerate>newTime && p.vsync)
			usleep((1000/framerate+oldTime-newTime)*1000);
		gettimeofday(&tv, NULL);
		return tv.tv_sec*1000+tv.tv_usec/1000;
	#endif
	}
	#ifdef WIN
	Sleep(oldTime);
	#else
	usleep(oldTime*1000);
	#endif
	return 0;
}

#if defined(__unix__)||defined(__APPLE__)
// general: handle signals
void sig_handler(int sig_no) {
	if (sig_no == SIGINT) {
		printf("CTRL-C pressed -- goodbye\n");
		kys=1;
		return;
	}
}
#endif


#ifdef ALSA
static bool is_loop_device_for_sure(const char * text) {
	const char * const LOOPBACK_DEVICE_PREFIX = "hw:Loopback,";
	return strncmp(text, LOOPBACK_DEVICE_PREFIX, strlen(LOOPBACK_DEVICE_PREFIX)) == 0;
}

static bool directory_exists(const char * path) {
	DIR * const dir = opendir(path);
	bool exists;// = dir != NULL;
	if (dir == NULL) exists = false;
	else exists = true;
	closedir(dir);
	return exists;
}

#endif

int * separate_freq_bands(fftw_complex *out, int bars, int lcf[200],
			 int hcf[200], float k[200], int channel, double sens, double ignore, int fftsize) {
	int o,i;
	double peak[201];
	static int fl[200];
	static int fr[200];
	double y[fftsize / 2 + 1];
	double temp;

	// process: separate frequency bands
	for (o = 0; o < bars; o++) {
		peak[o] = 0;

		// process: get peaks
		for (i = lcf[o]; i <= hcf[o]; i++) {
			//getting r of compex
			y[i] = hypot(out[i][0], out[i][1]);
			peak[o] += y[i]; //adding upp band
		}


		peak[o] = peak[o] / (hcf[o]-lcf[o] + 1); //getting average
		temp = peak[o] * sens * k[o] / 100000; //multiplying with k and sens
		if (temp <= ignore) temp = 0;
		if (channel == 1) fl[o] = temp;
		else fr[o] = temp;
	}

	if (channel == 1) return fl;
	else return fr;
} 


int * monstercat_filter (int * f, int bars, int waves, double monstercat) {
	int z;

	// process [smoothing]: monstercat-style "average"
	int m_y, de;
	if (waves > 0) {
		for (z = 0; z < bars; z++) { // waves
			f[z] = f[z] / 1.25;
			//if (f[z] < 1) f[z] = 1;
			for (m_y = z - 1; m_y >= 0; m_y--) {
				de = z - m_y;
				f[m_y] = max(f[z] - pow(de, 2), f[m_y]);
			}
			for (m_y = z + 1; m_y < bars; m_y++) {
				de = m_y - z;
				f[m_y] = max(f[z] - pow(de, 2), f[m_y]);
			}
		}
	} else if (monstercat > 0) {
		for (z = 0; z < bars; z++) {
			//if (f[z] < 1)f[z] = 1;
			for (m_y = z - 1; m_y >= 0; m_y--) {
				de = z - m_y;
				f[m_y] = max(f[z] / pow(monstercat, de), f[m_y]);
			}
			for (m_y = z + 1; m_y < bars; m_y++) {
				de = m_y - z;
				f[m_y] = max(f[z] / pow(monstercat, de), f[m_y]);
			}
		}
	}

	return f;
}


// general: entry point
int main(int argc, char **argv)
{
	// general: define variables
	pthread_t  p_thread;
	int thr_id GCC_UNUSED;
	float fc[200];
	float fre[200];
	int f[200], lcf[200], hcf[200];
	int *fl, *fr;
	int fmem[200];
	int flast[200];
	int flastd[200];
	int sleep = 0;
	int i, n, o, height, c, rest, inAtty, silence;
	#if defined(__unix__)||defined(__APPLE__)
		int fp, fptest;
	#endif
	//int cont = 1;
	int fall[200];
	//float temp;
	float fpeak[200];
	float k[200];
	float g;
	char configPath[255];
	char *usage = "\n\
Usage : " PACKAGE " [options]\n\
Visualize audio input in a window. \n\
\n\
Options:\n\
	-p          path to config file\n\
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

	char ch = '\0';
	int bars = 25;
	char supportedInput[255] = "'fifo'";
	int sourceIsAuto = 1;
	double smh;
	double *inl,*inr;
	unsigned long oldTime = 0;
	
	//int maxvalue = 0;

	struct audio_data audio;

	configPath[0] = '\0';

	setlocale(LC_ALL, "C");
	setbuf(stdout,NULL);
	setbuf(stderr,NULL);

	// general: handle Ctrl+C
	#if defined(__unix__)||defined(__APPLE__)
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = &sig_handler;
	sigaction(SIGINT, &action, NULL);
	#endif

	// general: handle command-line arguments
	while ((c = getopt (argc, argv, "p:vh")) != -1) {
		switch (c) {
			case 'p': // argument: fifo path
				snprintf(configPath, sizeof(configPath), "%s", optarg);
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

		n = 0;
	}

	#ifdef ALSA
	strcat(supportedInput,", 'alsa'");
	#endif
	#ifdef PULSE
	strcat(supportedInput,", 'pulse'");
	#endif
	#ifdef SNDIO
	strcat(supportedInput,", 'sndio'");
	#endif
	#ifdef PORTAUDIO
	strcat(supportedInput,", 'portaudio'");
	#endif
	#ifdef SHMEM
	strcat(supportedInput,", 'shmem'");
	#endif
	#ifdef WIN
	strcat(supportedInput,", 'wasapi'");
	#endif

	// general: main loop
	while (1) {

		//config: load
		load_config(configPath, supportedInput, (void *)&p);

		//input: init
		audio.source = malloc(1 +  strlen(p.audio_source));
		strcpy(audio.source, p.audio_source);

		audio.inputsize = p.inputsize;
		audio.fftsize = p.fftsize;
		audio.audio_out_l = malloc(sizeof(int)*p.fftsize+1);
		inl = malloc(sizeof(double)*(p.fftsize+2));	
		if(p.stereo) {
			audio.audio_out_r = malloc(sizeof(int)*p.fftsize+1);
			inr = malloc(sizeof(double)*(p.fftsize+2));	
		}
		M = p.fftsize;
		audio.format = -1;
		audio.rate = 0;
		audio.terminate = 0;
		audio.channels = 1+p.stereo;

		if(p.stereo) {
			for (i = 0; i < M; i++) {
				audio.audio_out_l[i] = 0;
				audio.audio_out_r[i] = 0;
			}
		} else for(i=0; i<M; i++) audio.audio_out_l[i] = 0;

		//fft: planning to rock
		fftw_complex outl[M/2+1], outr[M/2+1];
		fftw_plan pl = fftw_plan_dft_r2c_1d(M, inl, outl, FFTW_MEASURE), pr;
		if(p.stereo) pr = fftw_plan_dft_r2c_1d(M, inr, outr, FFTW_MEASURE);

		#ifdef ALSA
		// input_alsa: wait for the input to be ready
		if (p.im == 1) {
			if (is_loop_device_for_sure(audio.source)) {
				if (directory_exists("/sys/")) {
					if (! directory_exists("/sys/module/snd_aloop/")) {
						cleanup();
						fprintf(stderr,
						"Linux kernel module \"snd_aloop\" does not seem to  be loaded.\n"
						"Maybe run \"sudo modprobe snd_aloop\".\n");
						exit(EXIT_FAILURE);
					}
				}
			}

			thr_id = pthread_create(&p_thread, NULL, input_alsa, (void *)&audio); //starting alsamusic listener

			n = 0;

			while (audio.format == -1 || audio.rate == 0) {
				xavaSleep(1000, 0);
				n++;
				if (n > 2000) {
				#ifdef DEBUG
					cleanup();
					fprintf(stderr,
					"could not get rate and/or format, problems with audio thread? quiting...\n");
					exit(EXIT_FAILURE);
				#endif
				}
			}
		#ifdef DEBUG
			printf("got format: %d and rate %d\n", audio.format, audio.rate);
		#endif
		}
		#endif

		#if defined(__unix__)||defined(__APPLE__)
		if (p.im == 2) {
			//starting fifomusic listener
			thr_id = pthread_create(&p_thread, NULL, input_fifo, (void*)&audio); 
			audio.rate = 44100;
		}
		#endif

		#ifdef PULSE
		if (p.im == 3) {
			if (strcmp(audio.source, "auto") == 0) {
				getPulseDefaultSink((void*)&audio);
				sourceIsAuto = 1;
				}
			else sourceIsAuto = 0;
			//starting pulsemusic listener
			thr_id = pthread_create(&p_thread, NULL, input_pulse, (void*)&audio); 
			audio.rate = 44100;
		}
		#endif

		#ifdef SNDIO
		if (p.im == 4) {
			thr_id = pthread_create(&p_thread, NULL, input_sndio, (void*)&audio);
			audio.rate = 44100;
		}
		#endif

		#ifdef PORTAUDIO
		if (p.im == 5) {
			thr_id = pthread_create(&p_thread, NULL, input_portaudio, (void*)&audio);
			audio.rate = 44100;
		}
		#endif

		#ifdef SHMEM
		if (p.im == 6) {
			thr_id = pthread_create(&p_thread, NULL, input_shmem, (void*)&audio);
			audio.rate = 44100;
		}
		#endif

		#ifdef WIN
		if (p.im == 7) {
			thr_id = pthread_create(&p_thread, NULL, input_wasapi, (void*)&audio);
			audio.rate = 44100;
		}
		#endif

		if (p.highcf > audio.rate / 2) {
			cleanup();
			fprintf(stderr,
				"higher cuttoff frequency can't be higher then sample rate / 2"
			);
				exit(EXIT_FAILURE);
		}

		bool reloadConf = false;
		bool senseLow = true;

		// open XLIB window and set everything up
		#ifdef XLIB
		if(p.om == 5) if(init_window_x(argv, argc)) exit(EXIT_FAILURE);
		#endif

		// setting up sdl
		#ifdef SDL
		if(p.om == 6) if(init_window_sdl()) exit(EXIT_FAILURE);
		#endif

		#ifdef WIN
		if(p.om == 7) if(init_window_win()) exit(EXIT_FAILURE);
		#endif

		while(!reloadConf) {//jumbing back to this loop means that you resized the screen
			for (i = 0; i < 200; i++) {
				flast[i] = 0;
				flastd[i] = 0;
				fall[i] = 0;
				fpeak[i] = 0;
				fmem[i] = 0;
				f[i] = 0;
			}

			height = (p.h - 1) * (1+7*(p.om==4));

			#if defined(__unix__)||defined(__APPLE__)
			// output open file/fifo for raw output
			if (p.om == 4) {
				if (strcmp(p.raw_target,"/dev/stdout") != 0) {

					//checking if file exists
					if(access(p.raw_target, F_OK) != -1) {
						//testopening in case it's a fifo
						fptest = open(p.raw_target, O_RDONLY | O_NONBLOCK, 0644);
						if (fptest == -1) {
							printf("could not open file %s for writing\n", p.raw_target);
							exit(1);
						}
					} else {
						printf("creating fifo %s\n",p.raw_target);
						if (mkfifo(p.raw_target, 0664) == -1) {
							printf("could not create fifo %s\n", p.raw_target);
							exit(1);
						}
						//fifo needs to be open for reading in order to write to it
						fptest = open(p.raw_target, O_RDONLY | O_NONBLOCK, 0644); 
					}
				}

				fp = open(p.raw_target, O_WRONLY | O_NONBLOCK | O_CREAT, 0644);
				if (fp == -1) {
					printf("could not open file %s for writing\n",p.raw_target);
					exit(1);
				}
				printf("open file %s for writing raw ouput\n",p.raw_target);

				//width must be hardcoded for raw output.
				p.w = 200;

				if (strcmp(p.data_format, "binary") == 0)
					height = pow(2, p.bit_format) - 1;
				else height = p.ascii_range;
			}
			#endif

			// draw X11 background
			#ifdef XLIB
			if(p.om == 5) apply_window_settings_x();
			#endif
			#ifdef SDL
			if(p.om == 6) apply_window_settings_sdl();
			#endif
			#ifdef WIN
			if(p.om == 7) apply_win_settings();
			#endif

			//handle for user setting too many bars
			if (p.fixedbars) {
				p.autobars = 0;
				if (p.fixedbars * p.bw + p.fixedbars * p.bs - p.bs > p.w) p.autobars = 1;
			}

			//getting orignial numbers of barss incase of resize
			if (p.autobars == 1)  {
				bars = (p.w + p.bs) / (p.bw + p.bs);

				//if (p.bs != 0) bars = (w - bars * p.bs + p.bs) / bw;
			} else bars = p.fixedbars;

			if (bars < 1) bars = 1; // must have at least 1 bars
			if (bars > 200) bars = 200; // cant have more than 200 bars

			if (p.stereo) { //stereo must have even numbers of bars
				if (bars%2 != 0) bars--;
			} else if(p.oddoneout) { // and oddoneout nees to have odd number of bars
				if (bars%2 == 0) bars--;
			}

			// process [smoothing]: calculate gravity
			g = p.gravity * ((float)height / 2160) * (60 / (float)p.framerate);

			//checks if there is stil extra room, will use this to center
			rest = (p.w - bars * p.bw - bars * p.bs + p.bs) / 2;
			if (rest < 0)rest = 0;

			#ifdef DEBUG
				printw("height: %d width: %d bars:%d bar width: %d rest: %d\n",
							 w,
							 h, bars, p.bw, rest);
			#endif

			if (p.stereo) bars = bars / 2; // in stereo onle half number of bars per channel

			if ((p.smcount > 0) && (bars > 0)) {
				smh = (double)(((double)p.smcount)/((double)bars));
			}

			// since oddoneout requires every odd bar, why not split them in half?
			const int calcbars = p.oddoneout ? bars/2+bars%2 : bars;

			// frequency constant that we'll use for logarithmic progression of frequencies
			double freqconst = log(p.highcf-p.lowcf)/log(pow(calcbars, p.logScale));
			//freqconst = -2;

			// process: calculate cutoff frequencies
			for (n=0; n < calcbars; n++) {
				fc[n] = pow(powf(n, (p.logScale-1.0)*((double)n+1.0)/((double)calcbars)+1.0),
								 freqconst)+p.lowcf;
				fre[n] = fc[n] / (audio.rate / 2); 
				//remember nyquist!, pr my calculations this should be rate/2 
				//and  nyquist freq in M/2 but testing shows it is not... 
				//or maybe the nq freq is in M/4

				//lfc stores the lower cut frequency foo each bar in the fft out buffer
				lcf[n] = fre[n] * (M /2);

				if (n != 0) {
					//hfc holds the high cut frequency for each bar
					hcf[n-1] = lcf[n]-1; 
				}

				#ifdef DEBUG
					if (n != 0) {
						mvprintw(n,0,"%d: %f -> %f (%d -> %d) \n", n, 
							fc[n - 1], fc[n], lcf[n - 1],
								hcf[n - 1]);
							}
				#endif
			}
			hcf[n-1] = p.highcf*M/audio.rate;

			// process: weigh signal to frequencies height and EQ
			for (n = 0; n < calcbars; n++) {
				k[n] = pow(fc[n], p.eqBalance);
				k[n] *= (float)height /  100; 
				k[n] *= p.smooth[(int)floor(((double)n) * smh)];
				}

			if (p.stereo) bars = bars * 2;

			bool resizeWindow = false;

			while  (!resizeWindow) {
				if(should_reload) break;
				#ifdef XLIB
				if(p.om == 5)
				{
					switch(get_window_input_x())
					{
						case -1:
							cleanup(); 
							return EXIT_SUCCESS;
						case 1:
							should_reload = 1;
							break;
						case 2:
							resizeWindow = TRUE;
							break;
						case 3:
							clear_screen_x();
							memset(flastd, 0x00, sizeof(int)*200);
							break;
					}
				}
				#endif
				#ifdef SDL
				if(p.om == 6)
				{
					switch(get_window_input_sdl())
					{
						case -1:
							cleanup(); 
							return EXIT_SUCCESS;
						case 1:
							should_reload = 1;
							break;
						case 2:
							resizeWindow = 1;
							break;
						case 3:
							clear_screen_sdl();
							memset(flastd, 0x00, sizeof(int)*200);
							break;
					}
				}
				#endif
				#ifdef WIN
				if(p.om == 7)
				{
					switch(get_window_input_win())
					{
						case -1:
							cleanup(); 
							return EXIT_SUCCESS;
						case 1:
							should_reload = 1;
							break;
						case 2:
							resizeWindow = true;
							break;
						case 3:
							clear_screen_win();
							//memset(flastd, 0x00, sizeof(int)*200);
							break;
					}
				}
				#endif

				if (should_reload) {
					reloadConf = true;
					resizeWindow = true;
					should_reload = 0;
				}

				//if (cont == 0) break;

				#ifdef DEBUG
					//clear();
					refresh();
				#endif

				// process: populate input buffer and check if input is present
				silence = 1;
				for (i = 0; i < (2 * (M / 2 + 1)); i++) {
					if (i < M) {
						inl[i] = audio.audio_out_l[i];
						if (p.stereo) inr[i] = audio.audio_out_r[i];
						if (p.stereo ? inl[i] || inr[i] : inl[i]) silence = 0;
					} else {
						inl[i] = 0;
						if (p.stereo) inr[i] = 0;
					}
				}

				if (silence == 1) sleep++;
				else sleep = 0;

				// process: if input was present for the last 5 seconds apply FFT to it
				if (sleep < p.framerate * 5) {

					// process: execute FFT and sort frequency bands
					if (p.stereo) {
						fftw_execute(pl);
						fftw_execute(pr);

						fl = separate_freq_bands(outl,calcbars,lcf,hcf, k, 1, 
							p.sens, p.ignore, M);
						fr = separate_freq_bands(outr,calcbars,lcf,hcf, k, 2, 
							p.sens, p.ignore, M);
					} else {
						fftw_execute(pl);
						fl = separate_freq_bands(outl,calcbars,lcf,hcf, k, 1, 
							p.sens, p.ignore, M);
					}
				} else { //**if in sleep mode wait and continue**//
					#ifdef DEBUG
						printw("no sound detected for 3 sec, going to sleep mode\n");
					#endif
					//wait 1 sec, then check sound again.
					xavaSleep(1000, 0);
					continue;
				}

				if(p.oddoneout) {
					//memset(fl+sizeof(float)*(bars/2), 0.0f, sizeof(float)*bars);
					i=bars/2+1;
					do {
						i--;
						fl[i*2-1+bars%2]=fl[i];
						if(i!=0) fl[i]=0;
					} while(i!=0);
				}

				// process [smoothing]
				if (p.monstercat) {
					if (p.stereo) {
						fl = monstercat_filter(fl, bars / 2, p.waves,
							p.monstercat);
						fr = monstercat_filter(fr, bars / 2, p.waves,
							p.monstercat);
					} else {
						fl = monstercat_filter(fl, calcbars, p.waves, p.monstercat);
					}
				}

				//preperaing signal for drawing
				for (o = 0; o < bars; o++) {
					if (p.stereo) {
						if (o < bars / 2) {
							f[o] = fl[bars / 2 - o - 1];
						} else {
							f[o] = fr[o - bars / 2];
						}
					} else {
						f[o] = fl[o];
					}
				}


				// process [smoothing]: falloff
				if (g > 0) {
					for (o = 0; o < bars; o++) {
						if (f[o] < flast[o]) {
							f[o] = fpeak[o] - (g * fall[o] * fall[o]);
							fall[o]++;
						} else  {
							fpeak[o] = f[o];
							fall[o] = 0;
						}
						flast[o] = f[o];
					}
				}

				// process [smoothing]: integral
				if (p.integral > 0) {
					for (o = 0; o < bars; o++) {
						f[o] = fmem[o] * p.integral + f[o];
						fmem[o] = f[o];

						int diff = (height + 1) - f[o]; 
						if (diff < 0) diff = 0;
						double div = 1 / (diff + 1);
						//f[o] = f[o] - pow(div, 10) * (height + 1); 
						fmem[o] = fmem[o] * (1 - div / 20); 

						#ifdef DEBUG
							mvprintw(o,0,"%d: f:%f->%f (%d->%d), k-value: %f, peak:%d \n",
								o, fc[o], fc[o + 1], lcf[o], hcf[o], k[o], f[o]);
						#endif
					}
				}

				// process [oddoneout]
				if(p.oddoneout) {
					for(i=1; i<bars; i+=2) {
						f[i] = f[i+1]/2 + f[i-1]/2;
					}
				}


				// zero values causes divided by zero segfault
				for (o = 0; o < bars; o++) {
					if (f[o] < 1) {
						f[o] = 1;
						if (p.om == 4) f[o] = 0;
					}
					//if(f[o] > maxvalue) maxvalue = f[o];
				}

				//checking maxvalue I keep forgetting its about 10000
				//printf("%d\n",maxvalue); 

				//autmatic sens adjustment
				if (p.autosens) {
					for (o = 0; o < bars; o++) {
						if (f[o] > height ) {
							senseLow = false;
							p.sens = p.sens * 0.985;
							break;
						}
						if (senseLow && !silence) p.sens = p.sens * 1.01;
					if (o == bars - 1) p.sens = p.sens * 1.002;
					}
				}

				// output: draw processed input
				#ifndef DEBUG
					switch (p.om) {
						case 4:
							#if defined(__unix__)||defined(__APPLE__)
							rc = print_raw_out(bars, fp, p.is_bin, 
								p.bit_format, p.ascii_range, p.bar_delim,
								 p.frame_delim,f);
							break;
							#endif
						case 5:
						{
							#ifdef XLIB
							// this prevents invalid access
							if(should_reload||reloadConf) break;
							draw_graphical_x(bars, rest, f, flastd);
							break;
							#endif
						}
						case 6:
						{
							#ifdef SDL
							if(reloadConf) break;
							draw_graphical_sdl(bars, rest, f, flastd);
							break;
							#endif
						}
						case 7:
						{
							#ifdef WIN
							if(reloadConf) break;
							draw_graphical_win(bars, rest, f);
							break;
							#endif
						}
					}

					// window has been resized breaking to recalibrating values
					if (rc == -1) resizeWindow = true;

					oldTime = xavaSleep(oldTime, p.framerate);
				#endif

				for (o = 0; o < bars; o++) {
					flastd[o] = f[o];
				}

				if(kys) {
					resizeWindow=1;
					reloadConf=1;
				}

				//checking if audio thread has exited unexpectedly
				if(audio.terminate == 1) {
					cleanup();
					fprintf(stderr,
					"Audio thread exited unexpectedly. %s\n", audio.error_message);
					exit(EXIT_FAILURE); 
				} 
			}//resize window

		}//reloading config
		xavaSleep(100, 0);

		//**telling audio thread to terminate**//
		audio.terminate = 1;
		pthread_join( p_thread, NULL);

		free(p.smooth);
		if (sourceIsAuto) free(audio.source);

		// cleanup remaining FFT buffers
		if(audio.channels==2) {
			free(audio.audio_out_r);
			free(inr);
		}
		free(audio.audio_out_l);
		free(inl);
   
		cleanup();

		if(kys) exit(EXIT_SUCCESS);

		//fclose(fp);
	}
}
