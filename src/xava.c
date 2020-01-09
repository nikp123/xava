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

#ifdef __linux__
	#include "misc/inode_watcher.h"
#endif

#include "output/graphical.h"
#include "input/fifo.h"
#include "config.h"
#include "shared.h"

#ifdef __GNUC__
// curses.h or other sources may already define
#undef  GCC_UNUSED
#define GCC_UNUSED __attribute__((unused))
#else
#define GCC_UNUSED /* nothing */
#endif

static void* (*xavaInput)(void*);

static int (*xavaInitOutput)(void);
static void (*xavaOutputClear)(void);
static int (*xavaOutputApply)(void);
static int (*xavaOutputHandleInput)(void);
static void (*xavaOutputDraw)(int, int, int*, int*);
static void (*xavaOutputCleanup)(void);

static _Bool kys = 0, should_reload = 0;

// general: cleanup
void cleanup() {
	#ifdef __linux__
		// we need to do this since the inode watcher is a seperate thread
		destroyFileWatcher();
	#endif

	xavaOutputCleanup();
}

#if defined(__unix__)||defined(__APPLE__)
// general: handle signals
void sig_handler(int sig_no) {
	switch(sig_no) {
		case SIGINT:
			printf("CTRL-C pressed -- goodbye\n");
			kys=1;
			return;
		case SIGTERM:
			printf("Process termination requested -- goodbye\n");
			kys=1;
			return;
	}
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
		temp = peak[o] * sens * k[o] / 800000; //multiplying with k and sens
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
	int i, n, o, height, c, rest, silence;
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

	int bars = 25;
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
	sigaction(SIGTERM, &action, NULL);
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

	// general: main loop
	while (1) {
		//config: load
		load_config(configPath, (void *)&p);

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
		audio.format = -1;
		audio.rate = 0;
		audio.terminate = 0;
		audio.channels = 1+p.stereo;

		if(p.stereo) {
			for (i = 0; i < audio.fftsize; i++) {
				audio.audio_out_l[i] = 0;
				audio.audio_out_r[i] = 0;
			}
		} else for(i=0; i < audio.fftsize; i++) audio.audio_out_l[i] = 0;

		//fft: planning to rock
		fftw_complex outl[audio.fftsize/2+1], outr[audio.fftsize/2+1];
		fftw_plan pl = fftw_plan_dft_r2c_1d(audio.fftsize, inl, outl, FFTW_MEASURE), pr;
		if(p.stereo) pr = fftw_plan_dft_r2c_1d(audio.fftsize, inr, outr, FFTW_MEASURE);

		switch(p.im) {
			#ifdef ALSA
			case ALSA_INPUT_NUM:
				xavaInput = &input_alsa;
				break;
			#endif
			#if defined(__unix__)||defined(__APPLE__)
			case FIFO_INPUT_NUM:
				audio.rate = 44100;
				xavaInput = &input_fifo;
				break;
			#endif
			#ifdef PULSE
			case PULSE_INPUT_NUM:
				if(strcmp(audio.source, "auto") == 0) {
					getPulseDefaultSink((void*)&audio);
					sourceIsAuto = 1;
				} else sourceIsAuto = 0;
				audio.rate = 44100;
				xavaInput = &input_pulse;
				break;
			#endif
			#ifdef SNDIO
			case SNDIO_INPUT_NUM:
				audio.rate = 44100;
				xavaInput = &input_sndio;
				break;
			#endif
			#ifdef PORTAUDIO
			case PORTAUDIO_INPUT_NUM:
				audio.rate = 44100;
				xavaInput = input_portaudio;
				break;
			#endif
			#ifdef SHMEM
			case SHMEM_INPUT_NUM:
				audio.rate = 44100;
				xavaInput = input_shmem;
				break;
			#endif
			#ifdef WIN
			case WASAPI_INPUT_NUM:
				audio.rate = 44100;
				xavaInput = input_wasapi;
				break;
			#endif

		}

		switch(p.om) {
			#ifdef XLIB
			case X11_DISPLAY_NUM:
				xavaInitOutput = &init_window_x;
				xavaOutputClear = &clear_screen_x;
				xavaOutputApply = &apply_window_settings_x;
				xavaOutputHandleInput = &get_window_input_x;
				xavaOutputDraw = &draw_graphical_x;
				xavaOutputCleanup = &cleanup_graphical_x;
				break;
			#endif
			#ifdef SDL
			case SDL_DISPLAY_NUM:
				xavaInitOutput = &init_window_sdl;
				xavaOutputClear = &clear_screen_sdl;
				xavaOutputApply = &apply_window_settings_sdl;
				xavaOutputHandleInput = &get_window_input_sdl;
				xavaOutputDraw = &draw_graphical_sdl;
				xavaOutputCleanup = &cleanup_graphical_sdl;
				break;
			#endif
			#ifdef WIN
			case WIN32_DISPLAY_NUM:
				xavaInitOutput = &init_window_win;
				xavaOutputClear = &clear_screen_win;
				xavaOutputApply = &apply_win_settings;
				xavaOutputHandleInput = &get_window_input_win;
				xavaOutputDraw = &draw_graphical_win;
				xavaOutputCleanup = &cleanup_graphical_win;
				break;
			#endif
		}

		thr_id = pthread_create(&p_thread, NULL, xavaInput, (void*)&audio);
		if (p.highcf > audio.rate / 2) {
			cleanup();
			fprintf(stderr,
				"higher cuttoff frequency can't be higher then sample rate / 2"
			);
				exit(EXIT_FAILURE);
		}

		bool reloadConf = false;
		bool senseLow = true;

		if(xavaInitOutput())
			exit(EXIT_FAILURE);

		while(!reloadConf) {//jumbing back to this loop means that you resized the screen
			for (i = 0; i < 200; i++) {
				flast[i] = 0;
				flastd[i] = 0;
				fall[i] = 0;
				fpeak[i] = 0;
				fmem[i] = 0;
				f[i] = 0;
			}

			height = (p.h - 1);

			xavaOutputApply();

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
				lcf[n] = fre[n] * (audio.fftsize /2);

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
			hcf[n-1] = p.highcf*audio.fftsize/audio.rate;

			// process: weigh signal to frequencies height and EQ
			for (n = 0; n < calcbars; n++) {
				k[n] = pow(fc[n], p.eqBalance);
				k[n] *= (float)height / 100;
				k[n] *= p.smooth[(int)floor(((double)n) * smh)];
			}

			if (p.stereo) bars = bars * 2;

			bool resizeWindow = false;
			bool redrawWindow = false;

			while  (!resizeWindow) {
				switch(xavaOutputHandleInput()) {
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
						redrawWindow = TRUE;
						break;
				}
				#ifdef __linux__
					// check for updates in the config file
					should_reload = getFileStatus();
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
				for (i = 0; i < (2 * (audio.fftsize / 2 + 1)); i++) {
					if (i < audio.fftsize) {
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
							p.sens, p.ignore, audio.fftsize);
						fr = separate_freq_bands(outr,calcbars,lcf,hcf, k, 2, 
							p.sens, p.ignore, audio.fftsize);
					} else {
						fftw_execute(pl);
						fl = separate_freq_bands(outl,calcbars,lcf,hcf, k, 1, 
							p.sens, p.ignore, audio.fftsize);
					}
				} else { //**if in sleep mode wait and continue**//
					#ifdef DEBUG
						printw("no sound detected for 5 sec, going to sleep mode\n");
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
					for(i=bars-1; i>1; i-=2) {
						int sum = f[i+1]/2 + f[i-1]/2;
						if(sum>f[i]) f[i] = sum;
					}
				}


				// zero values causes divided by zero segfault
				// and set max height
				for (o = 0; o < bars; o++) {
					if(f[o] < 1) f[o] = 1;
					if(f[o] > p.h) f[o] = p.h;
				}

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
				if(should_reload||reloadConf) break;
				if(redrawWindow) {
					xavaOutputClear();
					memset(flastd, 0x00, sizeof(int)*200);
					redrawWindow = FALSE;
				}
				xavaOutputDraw(bars, rest, f, flastd);

				if(!p.vsync) // the window handles frametimes instead of XAVA
					oldTime = xavaSleep(oldTime, p.framerate);

				// save previous bar values
				memcpy(flastd, f, sizeof(int)*200);

				if(kys) {
					resizeWindow=1;
					reloadConf=1;
				}

				// checking if audio thread has exited unexpectedly
				if(audio.terminate == 1) {
					cleanup();
					fprintf(stderr,
					"Audio thread exited unexpectedly. %s\n", audio.error_message);
					exit(EXIT_FAILURE); 
				} 
			}//resize window
		}//reloading config

		// telling audio thread to terminate 
		audio.terminate = 1;

		// waiting for all background threads and other stuff to terminate properly
		xavaSleep(100, 0);

		// kill the audio thread
		pthread_join( p_thread, NULL);

		// get rid of everything else  
		cleanup();

		// internal variables
		free(p.smooth);
		if(sourceIsAuto) free(audio.source);

		// cleanup remaining FFT buffers (abusing C here)
		switch(audio.channels) {
			case 2:
				free(audio.audio_out_r);
				free(inr);
			default:
				free(audio.audio_out_l);
				free(inl);
				break;
		}

		// since this is an infinite loop we need to break out of it
		if(kys) break;
	}
	return 0;
}
