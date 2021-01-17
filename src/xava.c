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

#ifdef INIPARSER
	#include "../lib/iniparser/src/iniparser.h"
#else
	#include <iniparser.h>
#endif

#if defined(__linux__)||defined(WIN)
	#include "misc/inode_watcher.h"
#endif

#include "output/graphical.h"
#include "input/fifo/main.h"
#include "config.h"
#include "shared.h"

static void* (*xavaInput)(void*);
static void (*xavaInputHandleConfiguration)(void*, void*);

static void (*xavaOutputHandleConfiguration)(void*);
static int (*xavaInitOutput)(void);
static void (*xavaOutputClear)(void);
static int (*xavaOutputApply)(void);
static XG_EVENT (*xavaOutputHandleInput)(void);
static void (*xavaOutputDraw)(int, int, int*, int*);
static void (*xavaOutputCleanup)(void);

static _Bool kys = 0, should_reload = 0;
static float lastSens;

// for sharing XAVA-s internal state
// ...or possibly for signal handling :thonk:
struct state_params s;

// XAVA magic variables, too many of them indeed
static float *fc = NULL, *fre, *fpeak, *k;
static int *f, *lcf, *hcf, *fmem, *flast, *flastd, *fall, *fl, *fr;

static double *peak;
static pthread_t p_thread;
static struct audio_data audio;
static fftw_plan pl, pr;

// general: cleanup
void cleanup(void) {
	#if defined(__linux__)||defined(__WIN32__)
		// we need to do this since the inode watcher is a seperate thread
		destroyFileWatcher();
	#endif

	// telling audio thread to terminate 
	audio.terminate = 1;

	// waiting for all background threads and other stuff to terminate properly
	xavaSleep(100, 0);

	// kill the audio thread
	pthread_join(p_thread, NULL);

	xavaOutputCleanup();

	// destroy modules
	destroy_module(p.inputModule);
	destroy_module(p.outputModule);

	// clean up XAVA internal variables
	free(fc);
	free(fre);
	free(fpeak);
	free(k);
	free(f);
	free(lcf);
	free(hcf);
	free(fmem);
	free(flast);
	free(flastd);
	free(fall);
	free(fl);
	free(fr);
	free(peak);
	fc = NULL;

	fftw_destroy_plan(pl);
	fftw_destroy_plan(pr);
	fftw_cleanup();

	free(audio.source);
	free(p.smooth);

	// cleanup remaining FFT buffers (abusing C here)
	switch(audio.channels) {
		case 2:
			free(audio.audio_out_r);
		default:
			free(audio.audio_out_l);
			break;
	}

	// clean the config
	clean_config();
}

#if defined(__unix__)||defined(__APPLE__)
// general: handle signals
void sig_handler(int sig_no) {
	switch(sig_no) {
		case SIGUSR1:
			should_reload = true;
			break;
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

void separate_freq_bands(fftw_complex *out, int bars, int channel, double sens, double ignore, int fftsize) {
	int o,i;
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
} 


void monstercat_filter(int bars, int waves, double monstercat, int *data) {
	int z;

	// process [smoothing]: monstercat-style "average"
	int m_y, de;
	if (waves > 0) {
		for (z = 0; z < bars; z++) { // waves
			data[z] = data[z] / 1.25;
			//if (f[z] < 1) f[z] = 1;
			for (m_y = z - 1; m_y >= 0; m_y--) {
				de = z - m_y;
				data[m_y] = max(data[z] - pow(de, 2), data[m_y]);
			}
			for (m_y = z + 1; m_y < bars; m_y++) {
				de = m_y - z;
				data[m_y] = max(data[z] - pow(de, 2), data[m_y]);
			}
		}
	} else if (monstercat > 0) {
		for (z = 0; z < bars; z++) {
			//if (f[z] < 1)f[z] = 1;
			for (m_y = z - 1; m_y >= 0; m_y--) {
				de = z - m_y;
				data[m_y] = max(data[z] / pow(monstercat, de), data[m_y]);
			}
			for (m_y = z + 1; m_y < bars; m_y++) {
				de = m_y - z;
				data[m_y] = max(data[z] / pow(monstercat, de), data[m_y]);
			}
		}
	}
}


// general: entry point
int main(int argc, char **argv)
{
	// general: define variables
	//int thr_id;

	int sleep = 0;
	int i, n, o, height, c, rest, silence;
	//int cont = 1;
	//float temp;
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
	double smh = 0.0;
	unsigned long oldTime = 0;

	//int maxvalue = 0;

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
	sigaction(SIGUSR1, &action, NULL);
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
		// load config
		load_config(configPath, (void *)&p);

		// load symbols
		xavaInput                    = get_symbol_address(p.inputModule, "xavaInput");
		xavaInputHandleConfiguration = get_symbol_address(p.inputModule, "xavaInputHandleConfiguration");

		xavaInitOutput                = get_symbol_address(p.outputModule, "xavaInitOutput");
		xavaOutputClear               = get_symbol_address(p.outputModule, "xavaOutputClear");
		xavaOutputApply               = get_symbol_address(p.outputModule, "xavaOutputApply");
		xavaOutputHandleInput         = get_symbol_address(p.outputModule, "xavaOutputHandleInput");
		xavaOutputDraw                = get_symbol_address(p.outputModule, "xavaOutputDraw");
		xavaOutputCleanup             = get_symbol_address(p.outputModule, "xavaOutputCleanup");
		xavaOutputHandleConfiguration = get_symbol_address(p.outputModule, "xavaOutputHandleConfiguration");

		// load input config
		xavaInputHandleConfiguration(get_config_pointer(), (void*)&audio);

		// load output config
		xavaOutputHandleConfiguration(get_config_pointer());

		audio.inputsize = p.inputsize;
		audio.fftsize = p.fftsize;
		audio.audio_out_l = malloc(sizeof(double)*p.fftsize+1);
		if(p.stereo) {
			audio.audio_out_r = malloc(sizeof(double)*p.fftsize+1);
		}
		audio.format = -1;
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

		pl = fftw_plan_dft_r2c_1d(audio.fftsize, audio.audio_out_l, outl, FFTW_MEASURE);
		if(p.stereo) pr = fftw_plan_dft_r2c_1d(audio.fftsize, audio.audio_out_r, outr, FFTW_MEASURE);

		// thr_id = below
		pthread_create(&p_thread, NULL, xavaInput, (void*)&audio);
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

		while(!reloadConf) { //jumbing back to this loop means that you resized the screen
			height = (p.h - 1);

			// handle for user setting too many bars
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

			if (p.stereo) { // stereo must have even numbers of bars
				if (bars%2) bars--;
			} else if(p.oddoneout) { // and oddoneout needs to have odd number of bars
				if (!(bars%2)) bars--;
			}

			// if fc is not cleared that means that the variables are not initialized
			if(fc) {
				// reallocation time
				void *temp;
				temp = realloc(fc,sizeof(float)*bars+1);fc=temp;
				temp = realloc(fre,sizeof(float)*bars+1);fre=temp;
				temp = realloc(fpeak,sizeof(float)*bars+1);fpeak=temp;
				temp = realloc(k,sizeof(float)*bars+1);k=temp;
				temp = realloc(f,sizeof(int)*bars+1);f=temp;
				temp = realloc(lcf,sizeof(int)*bars+1);lcf=temp;
				temp = realloc(hcf,sizeof(int)*bars+1);hcf=temp;
				temp = realloc(fmem,sizeof(int)*bars+1);fmem=temp;
				temp = realloc(flast,sizeof(int)*bars+1);flast=temp;
				temp = realloc(flastd,sizeof(int)*bars+1);flastd=temp;
				temp = realloc(fall,sizeof(int)*bars+1);fall=temp;
				temp = realloc(fl,sizeof(int)*bars+1);fl=temp;
				temp = realloc(fr,sizeof(int)*bars+1);fr=temp;
				temp = malloc(sizeof(double)*(bars+1)+1);peak=temp;
			} else {
				fc = malloc(sizeof(float)*bars+1);
				fre = malloc(sizeof(float)*bars+1);
				fpeak = malloc(sizeof(float)*bars+1);
				k = malloc(sizeof(float)*bars+1);
				f = malloc(sizeof(int)*bars+1);
				lcf = malloc(sizeof(int)*bars+1);
				hcf = malloc(sizeof(int)*bars+1);
				fmem = malloc(sizeof(int)*bars+1);
				flast = malloc(sizeof(int)*bars+1);
				flastd = malloc(sizeof(int)*bars+1);
				fall = malloc(sizeof(int)*bars+1);
				fl = malloc(sizeof(int)*bars+1);
				fr = malloc(sizeof(int)*bars+1);
				peak = malloc(sizeof(double)*(bars+1)+1);
			} 

			for (i = 0; i < bars; i++) {
				flast[i] = 0;
				flastd[i] = 0;
				fall[i] = 0;
				fpeak[i] = 0;
				fmem[i] = 0;
				f[i] = 0;
				lcf[i] = 0;
				hcf[i] = 0;
				fl[i] = 0;
				fr[i] = 0;
				fc[i] = .0;
				fre[i] = .0;
				fpeak[i] = .0;
				k[i] = .0;
			}

			xavaOutputApply();

			// process [smoothing]: calculate gravity
			g = p.gravity * ((float)height / 2160) * (60 / (float)p.framerate);

			// checks if there is stil extra room, will use this to center
			rest = (p.w - bars * p.bw - bars * p.bs + p.bs) / 2;
			if (rest < 0)rest = 0;

			// TODO
			//#ifdef DEBUG
			//	printw("height: %d width: %d bars:%d bar width: %d rest: %d\n",
			//				 w,
			//				 h, bars, p.bw, rest);
			//#endif

			if (p.stereo) bars = bars / 2; // in stereo onle half number of bars per channel

			if ((p.smcount > 0) && (bars > 0)) {
				smh = (double)(((double)p.smcount)/((double)bars));
			}

			// since oddoneout requires every odd bar, why not split them in half?
			const int calcbars = p.oddoneout ? bars/2+1 : bars;

			// frequency constant that we'll use for logarithmic progression of frequencies
			double freqconst = log(p.highcf-p.lowcf)/log(pow(calcbars, p.logScale));
			//freqconst = -2;

			// process: calculate cutoff frequencies
			for (n=0; n < calcbars; n++) {
				fc[n] = pow(powf(n, (p.logScale-1.0)*((double)n+1.0)/((double)calcbars)+1.0),
								 freqconst)+p.lowcf;
				fre[n] = fc[n] / (audio.rate / 2); 
				// Remember nyquist!, pr my calculations this should be rate/2 
				// and  nyquist freq in M/2 but testing shows it is not... 
				// or maybe the nq freq is in M/4

				//lfc stores the lower cut frequency foo each bar in the fft out buffer
				lcf[n] = floor(fre[n] * (audio.fftsize/2));

				if (n != 0) {
					//hfc holds the high cut frequency for each bar

					// I know it's not precise, but neither are integers
					// You can see why in https://github.com/nikp123/xava/issues/29
					// I did reverse the "next_bar_lcf-1" change
					hcf[n-1] = lcf[n]; 
				}

				#ifdef DEBUG
					if (n != 0) {
						printf("%d: %f -> %f (%d -> %d) \n", 
							n, fc[n - 1], fc[n], lcf[n - 1], hcf[n - 1]);
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
				#if defined(__linux__)||defined(__WIN32__)
					// check for updates in the config file
					if(!should_reload)
						should_reload = getFileStatus();
				#endif

				if (should_reload) {
					reloadConf = true;
					resizeWindow = true;
					should_reload = 0;
				}

				//if (cont == 0) break;

				// process: populate input buffer and check if input is present
				silence = 1;

				for (i = 0; i < audio.fftsize+2; i++) {
					if(audio.audio_out_l[i]) {
						silence = 0;
						break;
					}
					if(p.stereo&&audio.audio_out_r[i]) {
						silence = 0;
						break;
					}
				}

					// so apparently sens can INDEED reach infinity
					// so I've decided to limit how high sens can rise
					// to prevent sens from reaching infinity again
					if(!silence)
						lastSens = p.sens;

				if (silence == 1) sleep++;
				else sleep = 0;

				// process: if input was present for the last 5 seconds apply FFT to it
				if (sleep < p.framerate * 5) {

					// process: execute FFT and sort frequency bands
					if (p.stereo) {
						fftw_execute(pl);
						fftw_execute(pr);

						separate_freq_bands(outl, calcbars, 1, p.sens, p.ignore, audio.fftsize);
						separate_freq_bands(outr, calcbars, 2, p.sens, p.ignore, audio.fftsize);
					} else {
						fftw_execute(pl);
						separate_freq_bands(outl, calcbars, 1, p.sens, p.ignore, audio.fftsize);
					}

					s.pauseRendering = false;
				} else { // if in sleep mode wait and continue
					// TODO
					//#ifdef DEBUG
					//	printw("no sound detected for 5 sec, going to sleep mode\n");
					//#endif
					// wait 100ms, then check sound again.
					xavaSleep(100, 0);

					// signal to any potential rendering threads to stop
					s.pauseRendering = true;

					// unless the user requested that the program ends
					if(kys||should_reload) sleep = 0;
					continue;
				}

				if(p.oddoneout) {
					//memset(fl+sizeof(float)*(bars/2), 0.0f, sizeof(float)*bars);
					for(i=bars/2; i>0; i--) {
						fl[i*2-1+bars%2]=fl[i];
						fl[i]=0;
					}
				}

				// process [smoothing]
				if (p.monstercat) {
					if (p.stereo) {
						monstercat_filter(bars / 2, p.waves, p.monstercat, fl);
						monstercat_filter(bars / 2, p.waves, p.monstercat, fr);
					} else {
						monstercat_filter(calcbars, p.waves, p.monstercat, fl);
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

						// TODO
						//#ifdef DEBUG
						//	mvprintw(o,0,"%d: f:%f->%f (%d->%d), k-value: %f, peak:%d \n",
						//		o, fc[o], fc[o + 1], lcf[o], hcf[o], k[o], f[o]);
						//#endif
					}
				}

				// process [oddoneout]
				if(p.oddoneout) {
					for(i=1; i<bars-1; i+=2) {
						f[i] = f[i+1]/2 + f[i-1]/2;
					}
					for(i=bars-3; i>1; i-=2) {
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

				// automatic sens adjustment
				if (p.autosens&&!silence) {
					// don't adjust on complete silence
					// as when switching tracks for example
					for (o = 0; o < bars; o++) {
						if (f[o] > height ) {
							senseLow = false;
							p.sens *= 0.985;
							break;
						}
						if (senseLow && !silence) {
							p.sens *= 1.01;
							// impose artificial limit on sens growth
							if(p.sens > lastSens*2) p.sens = lastSens*2;
						}
					}
				}

				// output: draw processed input
				if(redrawWindow) {
					xavaOutputClear();
					memset(flastd, 0x00, sizeof(int)*bars);
					redrawWindow = FALSE;
				}
				xavaOutputDraw(bars, rest, f, flastd);

				if(!p.vsync) // the window handles frametimes instead of XAVA
					oldTime = xavaSleep(oldTime, p.framerate);

				// save previous bar values
				memcpy(flastd, f, sizeof(int)*bars);

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
			} // resize window
		} // reloading config

		// get rid of everything else  
		cleanup();

		// since this is an infinite loop we need to break out of it
		if(kys) break;
	}
	return 0;
}
