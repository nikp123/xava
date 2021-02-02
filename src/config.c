#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <math.h>
#include <limits.h>

#include "output/graphical.h"
#include "config.h"
#include "shared.h"

// inode watching is a Linux(TM) feature
// so watch out when you're compiling it
#if defined(__linux__)||defined(WIN)
#include "misc/inode_watcher.h"
#endif

static const char *colorStrings[8] = {"black", "red", "green", "yellow", 
										"blue", "magenta", "cyan", "white"};

struct config_params p;
static dictionary* ini;

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

unsigned int parse_color(char *colorStr, int defaultColor) {
	unsigned int retColor = INT_MAX;
	if(colorStr[0] == '#') {
		sscanf(colorStr, "#%x", &retColor);
	} else {
		if(!strcmp(colorStr, "default"))
			return colorNumbers[defaultColor];

		for(size_t i = 0; i < COLOR_NUM; i++) {
			if(!strcmp(colorStr, colorStrings[i])) {
				retColor = i;
				break;
			}
		}
		retColor = colorNumbers[retColor];
	}
	return retColor;
}

dictionary *get_config_pointer(void) {
	return ini;
}

void validate_config(void* params, dictionary *ini) {
	struct config_params *p = (struct config_params *)params;

	// validate: input method
	p->inputModule = load_input_module(inputMethod);
	if(!is_module_valid(p->inputModule)) {
		fprintf(stderr, "Input method '%s' does not exist, exiting...\n",
				inputMethod);
		fprintf(stderr, "Details:\n");
		print_module_error();
		exit(EXIT_FAILURE);
	}

	// validate: output method
	p->outputModule = load_output_module(outputMethod);
	if(!is_module_valid(p->outputModule)) {
		fprintf(stderr, "Output method '%s' does not exist, exiting...\n",
				outputMethod);
		fprintf(stderr, "Details:\n");
		print_module_error();
		exit(EXIT_FAILURE);
	}

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
	if (p->vsync < -1) {
		fprintf(stderr,
			"Vsync cannot be below -1, no such Vsync mode exists!\n");
		exit(EXIT_FAILURE);
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

	// validate: gradient colors
	for(unsigned int i = 0; i < p->gradients; i++){
		if (!validate_color(p->gradient_colors[i])) {
			fprintf(stderr, "The first gradient color is invalid.\n"
				"It must be HTML color of the form '#xxxxxx'.\n");
			exit(EXIT_FAILURE);
		}
	}

	// actually parse colors
	p->col = parse_color(p->color, DEF_FG_COL);    // default cyan if invalid
	p->bgcol = parse_color(p->bcolor, DEF_BG_COL); // default black if invalid

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
		#if defined(__unix__)
			char *configFile = "config";
		#elif defined(WIN)||defined(__APPLE__)
			// editing files without an extension on windows is a pain
			char *configFile = "config.cfg";
		#endif
		if(xavaGetConfigDir(configPath)) {
			fprintf(stderr, "No HOME found (ERR_HOMELESS), exiting...");
			exit(EXIT_FAILURE);
		}

		// config: create directory
		xavaMkdir(configPath);

		// config: adding default filename file
		strcat(configPath, configFile);

		fp = fopen(configPath, "r");
		if (!fp) {
			#if defined(__unix__)||defined(__APPLE__)
				char *configFile = "config.example";
			#endif
			printf("Default config doesn't exist!\n"
					"Trying to find a default config file...");

			char *installPath = xavaGetInstallDir();
			// don't trust sizeof(), it's evil
			char *targetFile = malloc(strlen(installPath)+strlen(configFile)+1);
			strcpy(targetFile, installPath);
			free(installPath);
			strcat(targetFile, configFile);

			// because the program is not priviledged, read-only only
			FILE *source = fopen(targetFile, "r");

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
			free(targetFile);
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
	ini = iniparser_load(configPath);
	inputMethod = (char *)iniparser_getstring(ini, "input:method", XAVA_DEFAULT_INPUT);

	// config: output
	outputMethod = (char *)iniparser_getstring(ini, "output:method", XAVA_DEFAULT_OUTPUT);
	channels =  (char *)iniparser_getstring(ini, "output:channels", "mono");

	p->inputsize = (int)exp2((float)iniparser_getint(ini, "smoothing:input_size", 12));
	p->fftsize = (int)exp2((float)iniparser_getint(ini, "smoothing:fft_size", 14));
	p->monstercat = 1.5 * iniparser_getdouble(ini, "smoothing:monstercat", 0.0);
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
			char ini_config[33];
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
	p->sens = iniparser_getdouble(ini, "general:sensitivity", 100.0) *
		XAVA_PREDEFINED_SENS_VALUE; // check shared.h for details
	p->autosens = iniparser_getboolean(ini, "general:autosens", 1);
	p->overshoot = iniparser_getint(ini, "general:overshoot", 0);
	p->lowcf = iniparser_getint(ini, "general:lower_cutoff_freq", 26);
	p->highcf = iniparser_getint(ini, "general:higher_cutoff_freq", 15000);

	// config: window
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
	validate_config(params, ini);

	#if defined(__linux__)||defined(__WIN32__)
		// spawn a thread which will check if the file had been changed in any way
		// to inform the main process that it needs to reload
		watchFile(configPath);
	#endif
}

void clean_config() {
	iniparser_freedict(ini);
}
