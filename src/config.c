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
#if defined(__linux__)||defined(__WIN32__)
#include "misc/inode_watcher.h"
#endif

static const char *colorStrings[8] = {"black", "red", "green", "yellow", 
										"blue", "magenta", "cyan", "white"};

static dictionary* ini;

static char *inputMethod, *outputMethod, *filterMethod, *channels;

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

void validate_config(struct XAVA_HANDLE *hand, dictionary *ini) {
	struct config_params *p = &hand->conf;

	// validate: input method
	p->inputModule = load_input_module(inputMethod);
	xavaBailCondition(!is_module_valid(p->inputModule),
			"Input method '%s' could not load.\nReason: %s",
			inputMethod, get_module_error(p->inputModule));

	// validate: output method
	p->outputModule = load_output_module(outputMethod);
	xavaBailCondition(!is_module_valid(p->outputModule),
			"Output method '%s' could not load.\nReason: %s",
			outputMethod, get_module_error(p->outputModule));

	// validate: filter method
	p->filterModule = load_filter_module(filterMethod);
	xavaBailCondition(!is_module_valid(p->filterModule),
			"Filter method '%s' could not load.\nReason: %s",
			filterMethod, get_module_error(p->outputModule));

	// validate: output channels
	int stereo = -1;
	if (strcmp(channels, "mono") == 0)   stereo = 0;
	if (strcmp(channels, "stereo") == 0) stereo = 1;
	xavaBailCondition(stereo == -1, "Output channels '%s' is not supported,"
			" supported channels are: 'mono' and 'stereo'", channels);

	p->stereo = stereo ? 1 : 0; // makes the C compilers happy :D

	// validate: bars
	p->autobars = 1;
	if (p->fixedbars > 0) p->autobars = 0;
	if (p->fixedbars > 200) p->fixedbars = 200;
	if (p->bw > 200) p->bw = 200;
	if (p->bw < 1) p->bw = 1;

	// validate: framerate
	xavaBailCondition(p->framerate < 1, "Framerate cannot be below 1!");

	// validate: framerate
	xavaBailCondition(p->vsync < -1, "VSync cannot be below -1! "
			"No such VSync mode exists!\n");

	// validate: color
	xavaBailCondition(!validate_color(p->color),
			"The value for 'foreground' is invalid!\n"
			"It can be either one of the 7 named colors or a HTML color of"
			" the form '#xxxxxx'");

	// validate: background color
	xavaBailCondition(!validate_color(p->bcolor),
			"The value for 'background' is invalid!\n"
			"It can be either one of the 7 named colors or a HTML color of"
			" the form '#xxxxxx'");

	// validate: gradient colors
	for(unsigned int i = 0; i < p->gradients; i++){
		xavaBailCondition(!validate_color(p->gradient_colors[i]),
			"The gradient color %d is invalid!\n"
			"It can only be a HTML color of the form '#xxxxxx'", i+1);
	}

	// actually parse colors
	p->col = parse_color(p->color, DEF_FG_COL);    // default cyan if invalid
	p->bgcol = parse_color(p->bcolor, DEF_BG_COL); // default black if invalid

	xavaBailCondition((p->foreground_opacity > 1.0 || p->foreground_opacity < 0.0),
			"'foreground_opacity' must be in range of 0.0 to 1.0");
	xavaBailCondition((p->background_opacity > 1.0 || p->background_opacity < 0.0),
			"'background_opacity' must be in range of 0.0 to 1.0");

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
	xavaBailCondition(!foundAlignment, "Alignment '%s' is invalid!\n",
			p->winA);

	// validate: shadow
	xavaBailCondition(sscanf(p->shadow_color, "#%x", &p->shdw_col) != 1,
			"Shadow color should be a HTML color in the form '#xxxxxx'");
}

void load_config(char *configPath, struct XAVA_HANDLE *hand) {
	struct config_params *p = &hand->conf;

	// config: creating path to default config file
	if (configPath[0] == '\0') {
		char *found;
		bool success = xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG, 
			#if defined(__WIN32__)||defined(__APPLE__)
				"config.cfg",
			#elif defined(__unix__)
				"config",
			#endif
			&found);

		configPath = found;
		xavaBailCondition(!success, "Failed to create default config file!");
	} else { //opening specified file
		bool success = xavaFindAndCheckFile(XAVA_FILE_TYPE_CUSTOM_READ,
				configPath, &configPath);
		xavaBailCondition(success == false, "Specified config file '%s' does not exist!",
									configPath);
	}

	// config: parse ini
	ini = iniparser_load(configPath);

	// config: input
	inputMethod = (char *)iniparser_getstring(ini, "input:method", XAVA_DEFAULT_INPUT);
	p->inputsize = (int)exp2((float)iniparser_getint(ini, "input:size", 12));

	// config: output
	outputMethod = (char *)iniparser_getstring(ini, "output:method", XAVA_DEFAULT_OUTPUT);
	channels =  (char *)iniparser_getstring(ini, "output:channels", "mono");

	p->color = (char *)iniparser_getstring(ini, "color:foreground", "default");
	p->bcolor = (char *)iniparser_getstring(ini, "color:background", "default");
	p->foreground_opacity = iniparser_getdouble(ini, "color:foreground_opacity", 1.0);
	p->background_opacity = iniparser_getdouble(ini, "color:background_opacity", 0.0);

	p->gradients = iniparser_getint(ini, "color:gradient_count", 0);
	if(p->gradients) {
		xavaBailCondition(p->gradients < 2,
				"At least two colors must be given as gradient!\n");
		xavaBailCondition(p->gradients > 8,
				"Maximum 8 colors can be specified as gradient!\n");

		p->gradient_colors = (char **)malloc(sizeof(char*) * p->gradients);
		for(int i = 0;i < p->gradients;i++){
			char ini_config[33];
			sprintf(ini_config, "color:gradient_color_%d", (i + 1));
			p->gradient_colors[i] = (char *)iniparser_getstring(ini, ini_config, NULL);
			xavaBailCondition(!p->gradient_colors[i],
					"'gradient_color_%d' is not specified!\n", i+1);
		}
	}

	// config: default
	p->fixedbars = iniparser_getint(ini, "general:bars", 0);
	p->bw = iniparser_getint(ini, "general:bar_width", 13);
	p->bs = iniparser_getint(ini, "general:bar_spacing", 5);
	p->framerate = iniparser_getint(ini, "general:framerate", 60);
	p->vsync = iniparser_getboolean(ini, "general:vsync", 1);

	// config: window
	p->w = iniparser_getint(ini, "window:width", 1180);
	p->h = iniparser_getint(ini, "window:height", 300);

	//p->monitor_num = iniparser_getint(ini, "window:monitor", 0);

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

	// config: filter
	filterMethod = (char *)iniparser_getstring(ini, "filter:name", XAVA_DEFAULT_FILTER);
	p->fftsize = (int)exp2((float)iniparser_getint(ini, "filter:fft_size", 14));

	validate_config(hand, ini);

	#if defined(__linux__)||defined(__WIN32__)
		// spawn a thread which will check if the file had been changed in any way
		// to inform the main process that it needs to reload
		watchFile(configPath);
	#endif
}

void clean_config() {
	// apparently fucking iniparser has a double free somewhere and it can't work... great
	//iniparser_freedict(ini);
}
