#ifdef INIPARSER
	#include "../lib/iniparser/src/iniparser.h"
#else
	#include <iniparser.h>
#endif

#ifdef SNDIO
	#include <sndio.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <math.h>

#include "output/graphical.h"
#include "config.h"

// display output imports
#ifdef XLIB
#include "output/graphical_x.h"
#endif
#ifdef SDL
#include "output/graphical_sdl.h"
#endif
#ifdef WIN
#include "output/graphical_win.h"
#endif

// inode watching is a Linux(TM) feature
// so watch out when you're compiling it
#ifdef __linux__
#include "misc/inode_watcher.h"
#endif

static struct supported {
	size_t count;
	int *numbers;
	const char **names;
} support;

struct supported createSupported(void) {
	struct supported new;
	new.names = malloc(1);
	new.numbers = malloc(1);
	new.count = 0;
	return new;
}

void appendSupported(struct supported *support, char *appendName, int appendNum) {
	const char **names = realloc(support->names, (++support->count)*sizeof(char*));
	names[support->count-1] = appendName;
	support->names=names;
	int *numbers = realloc(support->numbers, support->count*sizeof(int*));
	numbers[support->count-1] = appendNum;
	support->numbers=numbers;
}

void deleteSupported(struct supported *support) {
	free(support->names);
	free(support->numbers);
	support->count = 0;
}

static char *inputMethod, *outputMethod, *channels;

int validate_color(char *checkColor)
{
	int validColor = 0;
	if (checkColor[0] == '#' && strlen(checkColor) == 7) {
		// 0 to 9 and a to f
		for (int i = 1; checkColor[i]; ++i) {
			if (!isdigit(checkColor[i])) {
				if (tolower(checkColor[i]) >= 'a' && tolower(checkColor[i]) <= 'f') {
					validColor = 1;
				} else {
					validColor = 0;
					break;
				}
			} else {
				validColor = 1;
			}
		}
	} else {
		if ((strcmp(checkColor, "black") == 0) || \
			(strcmp(checkColor, "red") == 0) || \
			(strcmp(checkColor, "green") == 0) || \
			(strcmp(checkColor, "yellow") == 0) || \
			(strcmp(checkColor, "blue") == 0) || \
			(strcmp(checkColor, "magenta") == 0) || \
			(strcmp(checkColor, "cyan") == 0) || \
			(strcmp(checkColor, "white") == 0) || \
			(strcmp(checkColor, "default") == 0)) validColor = 1;
	}
	return validColor;
}

void validate_config(void* params)
{
	struct config_params *p = (struct config_params *)params;
	
	// validate: input method
	p->im = 0;
	if (strcmp(inputMethod, "alsa") == 0) {
		p->im = 1;
		#ifndef ALSA
			fprintf(stderr,
				"xava was built without alsa support, install alsa dev files\n"
				"and run make clean && ./configure && make again\n");
			exit(EXIT_FAILURE);
		#endif
	}
	if (strcmp(inputMethod, "fifo") == 0) {
		p->im = 2;
	}
	if (strcmp(inputMethod, "pulse") == 0) {
		p->im = 3;
		#ifndef PULSE
			fprintf(stderr,
				"xava was built without pulseaudio support, install pulseaudio dev files\n"
				"and run make clean && ./configure && make again\n");
			exit(EXIT_FAILURE);
		#endif
	}
	if (strcmp(inputMethod, "sndio") == 0) {
		p->im = 4;
		#ifndef SNDIO
			fprintf(stderr, "xava was built without sndio support\n");
			exit(EXIT_FAILURE);
		#endif
	}
	if (strcmp(inputMethod, "portaudio") == 0) {
		p->im = 5;
		#ifndef PORTAUDIO
			fprintf(stderr, "xava was built without portaudio support\n");
			exit(EXIT_FAILURE);
		#endif
	}
	if (strcmp(inputMethod, "shmem") == 0) {
		p->im = 6;
		#ifndef SHMEM
			fprintf(stderr, "xava was built without shmem support\n");
			exit(EXIT_FAILURE);
		#endif
	}
	if (strcmp(inputMethod, "wasapi") == 0) {
		p->im = 7;
		#ifndef WIN
			fprintf(stderr, "xava was built without WASAPI support\n");
			exit(EXIT_FAILURE);
		#endif
	}
	if (p->im == 0) {
		fprintf(stderr,
			"input method '%s' is not supported, supported methods are: %s\n",
						inputMethod);
		exit(EXIT_FAILURE);
	}

	// validate: output method
	p->om = 0;
	support = createSupported();
	#ifdef XLIB
		appendSupported(&support, X11_DISPLAY_NAME, X11_DISPLAY_NUM);
	#endif
	#ifdef SDL
		appendSupported(&support, SDL_DISPLAY_NAME, SDL_DISPLAY_NUM);
	#endif
	#ifdef WIN
		appendSupported(&support, WIN32_DISPLAY_NAME, WIN32_DISPLAY_NUM);
	#endif
	for(size_t i = 0; i < support.count; i++) {
		if(!strcmp(outputMethod, support.names[i])) {
			p->om = support.numbers[i];
			break;
		}
	}
	if(!p->om) {
		fprintf(stderr, "Output method %s doesn't exist in this build of "PACKAGE"\n"
						"Change it or recompile with the proper dependencies installed!\n", outputMethod);
		exit(EXIT_FAILURE);
	}
	deleteSupported(&support);

	// validate: output channels
	p->stereo = -1;
	if (strcmp(channels, "mono") == 0) p->stereo = 0;
	if (strcmp(channels, "stereo") == 0) p->stereo = 1;
	if (p->stereo == -1) {
		fprintf(stderr,
			"output channels %s is not supported, supported channelss are: 'mono' and 'stereo'\n",
						channels);
		exit(EXIT_FAILURE);
	}
	
	// validate: bars
	p->autobars = 1;
	if (p->fixedbars > 0) p->autobars = 0;
	if (p->fixedbars > 200) p->fixedbars = 200;
	if (p->bw > 200) p->bw = 200;
	if (p->bw < 1) p->bw = 1;
	
	// validate: framerate
	if (p->framerate < 1) {
		fprintf(stderr,
			"framerate can't lower than 1!\n");
		exit(EXIT_FAILURE);
	}
	
	// validate: framerate
	if (p->vsync != 0) {
		switch(p->om) {
		#ifdef SDL
			case SDL_DISPLAY_NUM:
				p->vsync = 0;
				break;
		#endif
		#ifdef XLIB
			case X11_DISPLAY_NUM:
				if(!GLXmode) p->vsync = 0;
				break;
		#endif
		}
		if (p->vsync < -1) {
			fprintf(stderr,
				"Vsync cannot be below -1, no such Vsync mode exists!\n");
			exit(EXIT_FAILURE);
		}
	}
	
	// validate: color
	if (!validate_color(p->color)) {
		fprintf(stderr, "The value for 'foreground' is invalid.\n"
			"It can be either one of the 7 named colors or a HTML color of the form '#xxxxxx'.\n");
		exit(EXIT_FAILURE);
	}
	
	// validate: background color
	if (!validate_color(p->bcolor)) {
		fprintf(stderr, "The value for 'background' is invalid.\n"
			"It can be either one of the 7 named colors or a HTML color of the form '#xxxxxx'.\n");
		exit(EXIT_FAILURE);
	}
	
	for(unsigned int i = 0; i < p->gradients; i++){
		if (!validate_color(p->gradient_colors[i])) {
			fprintf(stderr, "The first gradient color is invalid.\n"
				"It must be HTML color of the form '#xxxxxx'.\n");
			exit(EXIT_FAILURE);
		}
	}
	
	// In case color is not html format set bgcol and col to predefinedint values
	p->col = 6; // default cyan if invalid

	const char *colors[8] = {"black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"};
	for(unsigned int i = 0; i < 8; i++) {
		if(!strcmp(p->color, colors[0])) p->col = i;
		if(!strcmp(p->bcolor, colors[0])) p->bgcol = i;
	}

	if(p->foreground_opacity > 1.0 || p->foreground_opacity < 0.0) {
		fprintf(stderr, "foreground_opacity cannot be above 1.0 or below 0.0\n");
		exit(EXIT_FAILURE);
	}
	if(p->background_opacity > 1.0 || p->foreground_opacity < 0.0) {
		fprintf(stderr, "background_opacity cannot be above 1.0 or below 0.0\n");
		exit(EXIT_FAILURE);
	}

	// validate: gravity
	p->gravity = p->gravity / 100;
	if (p->gravity < 0) {
		p->gravity = 0;
	} 

	// validate: oddoneout
	if(p->stereo&&p->oddoneout) {
		fprintf(stderr, "oddoneout cannot work with stereo mode on\n");
		exit(EXIT_FAILURE);
	}
	
	// validate: integral
	p->integral = p->integral / 100;
	if (p->integral < 0) {
		p->integral = 0;
	} else if (p->integral > 1) {
		p->integral = 1;
	}

	// validate: cutoff
	if (p->lowcf == 0 ) p->lowcf++;
	if (p->lowcf > p->highcf) {
		fprintf(stderr,
			"lower cutoff frequency can't be higher than higher cutoff frequency\n");
		exit(EXIT_FAILURE);
	}

	// setting sens
	p->sens = p->sens / 100;

	// validate: window settings
	// validate: alignment
	_Bool foundAlignment = 0;
	const char *alignments[13] = {"top_left", "top_right", "bottom_left", "bottom_right", "left",
								 "right", "top", "bottom", "center", "none"};
	for(int i = 0; i < 10; i++) {
		if(!strcmp(p->winA, alignments[i])) {
			foundAlignment = 1;
			break;
		}
	}
	if(!foundAlignment)
		fprintf(stderr, "The value for alignment is invalid, '%s'!", p->winA);

	#ifdef XLIB
	if(p->om == X11_DISPLAY_NUM) {
		if(p->iAmRoot && p->gradients) {
			fprintf(stderr, "rootwindow and gradients don't work!\n");
			exit(EXIT_FAILURE);
		}
		#ifdef GLX
		if(p->iAmRoot && GLXmode) {
			fprintf(stderr, "rootwindow and OpenGL don't work!\n");
			exit(EXIT_FAILURE);
		}
		#endif
	}
	#endif

	// validate: shadow
	if(sscanf(p->shadow_color, "#%x", &p->shdw_col) != 1)
	{
		fprintf(stderr, "shadow color is improperly formatted!\n");
		exit(EXIT_FAILURE);
	}
}

void load_config(char *configPath, void* params)
{
	struct config_params *p = (struct config_params *)params;
	FILE *fp;

	//config: creating path to default config file
	if (configPath[0] == '\0') {
		#if defined(__unix__)||defined(__APPLE__)
			char *configFile = "config";
			char *configHome = getenv("XDG_CONFIG_HOME");
		#elif defined(WIN)
			// editing files without an extension on windows is a pain
			char *configFile = "config.cfg";
			char *configHome = getenv("APPDATA");
		#endif
		// don't worry, this will never happen on windows unless you are running like 98
		if (configHome != NULL) {
			sprintf(configPath,"%s/%s/", configHome, PACKAGE);
		} else {
			configHome = getenv("HOME");
			if (configHome != NULL) {
				sprintf(configPath,"%s/%s/%s/", configHome, ".config", PACKAGE);
			} else {
				printf("No HOME found (ERR_HOMELESS), exiting...");
				exit(EXIT_FAILURE);
			}
		}

		// config: create directory
		#if defined(__unix__)||defined(__APPLE__)
			mkdir(configPath, 0770);
		#else
			mkdir(configPath);
		#endif

		// config: adding default filename file
		strcat(configPath, configFile);

		fp = fopen(configPath, "rb+");
		if (!fp) {
			#if defined(__unix__)||defined(__APPLE__)
				char *configFile = "config.example";
			#endif
			printf("Default config doesn't exist!\n"
					"Trying to find a default config file...");

			// Finding the config file path
			#if defined(__unix__)||defined(__APPLE__)
				// UNIX-like OS-es, but not Apple
				char targetFile[255];
				sprintf(targetFile, "%s/share/%s/%s", PREFIX, PACKAGE, configFile);
				FILE *source = fopen(targetFile, "r");
			#elif defined(__WIN32__)
				// Windows

				// Windows uses widechars internally
				WCHAR wpath[MAX_PATH];
				char path[MAX_PATH];

				// Get path of where the executable is installed
				// Assuming that the user has installed the program 
				// and that the config file is in the same directory
				// as the program
				HMODULE hModule = GetModuleHandleW(NULL);
				GetModuleFileNameW(hModule, wpath, MAX_PATH);
				wcstombs(path, wpath, MAX_PATH);

				// Hardcoded things pain me, but writing a 100 line-long
				// string replace function for windows is a no-no.
				//
				// This is why you use C++ for code like this.
				//
				// xava.exe => config.cfg
				strcpy(&path[strlen(path)-8], configFile);

				// because the program is not priviledged, read-only only
				FILE *source = fopen(path, "r");
			#endif

			if(!source) {
				// inipaser magic time triggered here
				fprintf(stderr, "FAIL\nDefault configuration file doesn't exist. "
					"Please provide one if you can!\n");
			} else {
				fp = fopen(configPath, "w");
				if(!fp) {
					fprintf(stderr, "FAIL\n"
						"Couldn't create config file! The program will now end.\n");
					exit(EXIT_FAILURE);
				}

				// Copy file in the most C way possible
				short c = fgetc(source); 
				while (c != EOF) { 
					fputc(c, fp); 
					c = fgetc(source); 
				}
				fclose(source);
				fclose(fp);
				printf("DONE\n");
			}
		}
	} else { //opening specified file
		fp = fopen(configPath, "rb+");
		if (fp) {
			fclose(fp);
		} else {
			printf("Unable to open file '%s', exiting...\n", configPath);
			exit(EXIT_FAILURE);
		}
	}

	// config: parse ini
	dictionary* ini;
	ini = iniparser_load(configPath);

	// setting fifo to default if no other input modes supported
	inputMethod = (char *)iniparser_getstring(ini, "input:method", "fifo"); 

	// setting alsa to default if supported
	#ifdef ALSA
		inputMethod = (char *)iniparser_getstring(ini, "input:method", "alsa"); 
	#endif

	// portaudio is priority 2 (sorted by difficulty)
	#ifdef PORTAUDIO
		inputMethod = (char *)iniparser_getstring(ini, "input:method", "portaudio");
	#endif

	// pulse is basically autoconfig (so priority 1)
	#ifdef PULSE
		inputMethod = (char *)iniparser_getstring(ini, "input:method", "pulse");
	#endif

	// WASAPI is basically the best for windows (sorted by difficulty)
	#ifdef WIN
		inputMethod = (char *)iniparser_getstring(ini, "input:method", "wasapi");
	#endif


	// config: output
	// use macros to define the default output
	#if defined(__linux__)||defined(__APPLE__)||defined(__unix__)
		#if defined(XLIB)
			#define DEFAULT_OUTPUT X11_DISPLAY_NAME
		#elif defined(SDL)
			#define DEFAULT_OUTPUT SDL_DISPLAY_NAME
		#endif
	#elif defined(__WIN32__)
		#define DEFAULT_OUTPUT WIN32_DISPLAY_NAME
	#endif
	outputMethod = (char *)iniparser_getstring(ini, "output:method", DEFAULT_OUTPUT);
	channels =  (char *)iniparser_getstring(ini, "output:channels", "mono");

	p->inputsize = (int)exp2((float)iniparser_getint(ini, "smoothing:input_size", 12));
	p->fftsize = (int)exp2((float)iniparser_getint(ini, "smoothing:fft_size", 14));
	p->monstercat = 1.5 * iniparser_getdouble(ini, "smoothing:monstercat", 1.2);
	p->waves = iniparser_getint(ini, "smoothing:waves", 0);
	p->integral = iniparser_getdouble(ini, "smoothing:integral", 85);
	p->gravity = 50.0 * iniparser_getdouble(ini, "smoothing:gravity", 100);
	p->ignore = iniparser_getdouble(ini, "smoothing:ignore", 0);
	p->logScale = iniparser_getdouble(ini, "smoothing:log", 1.55);
	p->oddoneout = iniparser_getboolean(ini, "smoothing:oddoneout", 1);
	p->eqBalance = iniparser_getdouble(ini, "smoothing:eq_balance", 0.67);

	p->color = (char *)iniparser_getstring(ini, "color:foreground", "default");
	p->bcolor = (char *)iniparser_getstring(ini, "color:background", "default");
	p->foreground_opacity = iniparser_getdouble(ini, "color:foreground_opacity", 1.0);
	p->background_opacity = iniparser_getdouble(ini, "color:background_opacity", 0.0);

	p->gradients = iniparser_getint(ini, "color:gradient_count", 0);
	if(p->gradients) {
		if(p->gradients < 2){
			printf("\nAtleast two colors must be given as gradient!\n");
			exit(EXIT_FAILURE);
		}
		if(p->gradients > 8){
			printf("\nMaximum 8 colors can be specified as gradient!\n");
			exit(EXIT_FAILURE);
		}
		p->gradient_colors = (char **)malloc(sizeof(char*) * p->gradients);
		for(int i = 0;i < p->gradients;i++){
			char ini_config[23];
			sprintf(ini_config, "color:gradient_color_%d", (i + 1));
			p->gradient_colors[i] = (char *)iniparser_getstring(ini, ini_config, NULL);
			if(p->gradient_colors[i] == NULL){
				printf("\nGradient color not specified : gradient_color_%d\n", (i + 1));
				exit(EXIT_FAILURE);
			}
		}
	}

	p->fixedbars = iniparser_getint(ini, "general:bars", 0);
	p->bw = iniparser_getint(ini, "general:bar_width", 13);
	p->bs = iniparser_getint(ini, "general:bar_spacing", 5);
	p->framerate = iniparser_getint(ini, "general:framerate", 60);
	p->vsync = iniparser_getboolean(ini, "general:vsync", 1);
	p->sens = iniparser_getint(ini, "general:sensitivity", 100);
	p->autosens = iniparser_getboolean(ini, "general:autosens", 1);
	p->overshoot = iniparser_getint(ini, "general:overshoot", 0);
	p->lowcf = iniparser_getint(ini, "general:lower_cutoff_freq", 26);
	p->highcf = iniparser_getint(ini, "general:higher_cutoff_freq", 15000);

	// config: window
	#ifdef GLX
		GLXmode = iniparser_getboolean(ini, "window:opengl", 1);
	#endif

	#ifdef XLIB
		// nikp123 causes a nuclear disaster 2019 (colorized)
		p->iAmRoot = iniparser_getboolean(ini, "window:rootwindow", 0);
	#endif


	p->w = iniparser_getint(ini, "window:width", 1180);
	p->h = iniparser_getint(ini, "window:height", 300);

	p->winA = (char *)iniparser_getstring(ini, "window:alignment", "none");
	p->wx = iniparser_getint(ini, "window:x_padding", 0);
	p->wy = iniparser_getint(ini, "window:y_padding", 0);
	p->fullF = iniparser_getboolean(ini, "window:fullscreen", 0);
	p->transF = iniparser_getboolean(ini, "window:transparency", 1);
	p->borderF = iniparser_getboolean(ini, "window:border", 0);
	p->bottomF = iniparser_getboolean(ini, "window:keep_below", 1);
	p->interactF = iniparser_getboolean(ini, "window:interactable", 1);
	p->winPropF = iniparser_getboolean(ini, "window:set_win_props", 0);
	p->taskbarF = iniparser_getboolean(ini, "window:taskbar_icon", 1);

	// config: shadow
	p->shdw = iniparser_getint(ini, "shadow:size", 7);
	p->shadow_color = (char *)iniparser_getstring(ini, "shadow:color", "#ff000000");
	if(sscanf(p->shadow_color, "#%x", &p->shdw_col) != 1)
	{
		fprintf(stderr, "shadow color is improperly formatted!\n");
		exit(EXIT_FAILURE);
	}

	// read & validate: eq
	p->smcount = iniparser_getsecnkeys(ini, "eq");
	if (p->smcount > 0) {
		p->smooth = malloc(p->smcount*sizeof(double));
		#ifndef LEGACYINIPARSER
		const char *keys[p->smcount];
		iniparser_getseckeys(ini, "eq", keys);
		#else
		char **keys = iniparser_getseckeys(ini, "eq");
		#endif
		for (int sk = 0; sk < p->smcount; sk++) {
			p->smooth[sk] = iniparser_getdouble(ini, keys[sk], 1);
		}
	} else {
		p->smcount = 64; //back to the default one
		p->smooth = malloc(p->smcount*sizeof(double));
		for(int i=0; i<64; i++) p->smooth[i]=1.0f;
	}

	// config: input
	p->im = 0;
	if (strcmp(inputMethod, "alsa") == 0) {
		p->im = 1;
		p->audio_source = (char *)iniparser_getstring(ini, "input:source", "hw:Loopback,1");
	}
	if (strcmp(inputMethod, "fifo") == 0) {
		p->im = 2;
		p->audio_source = (char *)iniparser_getstring(ini, "input:source", "/tmp/mpd.fifo");
	}
	if (strcmp(inputMethod, "pulse") == 0) {
		p->im = 3;
		p->audio_source = (char *)iniparser_getstring(ini, "input:source", "auto");
	}
	#ifdef SNDIO
	if (strcmp(inputMethod, "sndio") == 0) {
		p->im = 4;
		p->audio_source = (char *)iniparser_getstring(ini, "input:source", SIO_DEVANY);
	}
	#endif
	#ifdef PORTAUDIO
	if (strcmp(inputMethod, "portaudio") == 0) {
		p->im = 5;
		p->audio_source = (char *)iniparser_getstring(ini, "input:source", "auto");
	}
	#endif
	#ifdef SHMEM
	if (strcmp(inputMethod, "shmem") == 0) {
		p->im = 6;
		p->audio_source = (char *)iniparser_getstring(ini, "input:source", "/squeezelite-00:00:00:00:00:00");
	}
	#endif
	#ifdef WIN
	if (strcmp(inputMethod, "wasapi") == 0) {
		p->im = 7;
		p->audio_source = (char *)iniparser_getstring(ini, "input:source", "loopback");
	}
	#endif

	validate_config(params);
	//iniparser_freedict(ini);

	#ifdef __linux__
		// spawn a thread which will check if the file had been changed in any way
		// to inform the main process that it needs to reload
		watchFile(configPath);
	#endif
}
