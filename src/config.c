#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <math.h>
#include <limits.h>

#include "output/graphical.h"
#include "config.h"
#include "shared.h"

static const char *colorStrings[8] = {"black", "red", "green", "yellow",
                                        "blue", "magenta", "cyan", "white"};

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

void validate_config(struct XAVA_HANDLE *hand, XAVACONFIG config) {
    struct config_params *p = &hand->conf;

    // validate: input method
    p->inputModule = load_input_module(inputMethod);
    xavaBailCondition(!is_module_valid(p->inputModule),
            "Input method '%s' could not load.\nReason: %s",
            inputMethod, xava_module_error_get(p->inputModule));

    // validate: output method
    p->outputModule = load_output_module(outputMethod);
    xavaBailCondition(!is_module_valid(p->outputModule),
            "Output method '%s' could not load.\nReason: %s",
            outputMethod, xava_module_error_get(p->outputModule));

    // validate: filter method
    p->filterModule = load_filter_module(filterMethod);
    xavaBailCondition(!is_module_valid(p->filterModule),
            "Filter method '%s' could not load.\nReason: %s",
            filterMethod, xava_module_error_get(p->outputModule));

    // validate: output channels
    int stereo = -1;
    if (strcmp(channels, "mono") == 0)   stereo = 0;
    if (strcmp(channels, "stereo") == 0) stereo = 1;
    xavaBailCondition(stereo == -1, "Output channels '%s' is not supported,"
            " supported channels are", " 'mono' and 'stereo'", channels);

    // validate: input
    p->stereo = stereo ? 1 : 0; // makes the C compilers happy
    xavaBailCondition(!p->samplerate, "samplerate CANNOT BE 0!");
    xavaBailCondition(p->samplelatency > p->inputsize, "Sample latency cannot be larger than the audio buffer itself!");
    xavaWarnCondition(p->samplelatency*p->framerate > p->samplerate, "Sample latency might be too large, expect audio lags!");
    xavaWarnCondition(p->samplelatency < 32, "Sample latency might be too low, high CPU usage is MOST LIKELY!");
    xavaBailCondition(p->samplelatency == 0, "Sample latency CANNOT BE 0!");

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
}

char *load_config(char *configPath, struct XAVA_HANDLE *hand) {
    struct config_params *p = &hand->conf;

    // config: creating path to default config file
    if (configPath == NULL) {
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
    hand->default_config.config = xavaConfigOpen(configPath);
    xavaBailCondition(!hand->default_config.config, "Failed to open default XAVA config at '%s'", configPath);

    // config: input
    inputMethod = (char *)xavaConfigGetString(hand->default_config.config, "input", "method", XAVA_DEFAULT_INPUT);
    p->inputsize = (int)exp2((float)xavaConfigGetInt(hand->default_config.config, "input", "size", 12));
    p->samplerate = xavaConfigGetInt(hand->default_config.config, "input", "rate", 44100);
    p->samplelatency = xavaConfigGetInt(hand->default_config.config, "input", "latency", 128);

    // config: output
    outputMethod = (char *)xavaConfigGetString(hand->default_config.config, "output", "method", XAVA_DEFAULT_OUTPUT);
    channels =  (char *)xavaConfigGetString(hand->default_config.config, "output", "channels", "mono");

    p->color = (char *)xavaConfigGetString(hand->default_config.config, "color", "foreground", "default");
    p->bcolor = (char *)xavaConfigGetString(hand->default_config.config, "color", "background", "default");
    p->foreground_opacity = xavaConfigGetDouble(hand->default_config.config, "color", "foreground_opacity", 1.0);
    p->background_opacity = xavaConfigGetDouble(hand->default_config.config, "color", "background_opacity", 0.0);

    p->gradients = xavaConfigGetInt(hand->default_config.config, "color", "gradient_count", 0);
    if(p->gradients) {
        xavaBailCondition(p->gradients < 2,
                "At least two colors must be given as gradient!\n");
        xavaBailCondition(p->gradients > 8,
                "Maximum 8 colors can be specified as gradient!\n");

        p->gradient_colors = (char **)malloc(sizeof(char*) * p->gradients);
        for(int i = 0;i < p->gradients;i++){
            char ini_config[33];
            sprintf(ini_config, "gradient_color_%d", (i+1));
            p->gradient_colors[i] = (char *)xavaConfigGetString(hand->default_config.config, "color", ini_config, NULL);
            xavaBailCondition(!p->gradient_colors[i],
                    "'gradient_color_%d' is not specified!\n", i+1);
        }
    }

    // config: default
    p->fixedbars = xavaConfigGetInt(hand->default_config.config, "general", "bars", 0);
    p->bw = xavaConfigGetInt(hand->default_config.config, "general", "bar_width", 13);
    p->bs = xavaConfigGetInt(hand->default_config.config, "general", "bar_spacing", 5);
    p->framerate = xavaConfigGetInt(hand->default_config.config, "general", "framerate", 60);
    p->vsync = xavaConfigGetBool(hand->default_config.config, "general", "vsync", 1);

    // config: window
    p->w = xavaConfigGetInt(hand->default_config.config, "window", "width", 1180);
    p->h = xavaConfigGetInt(hand->default_config.config, "window", "height", 300);

    //p->monitor_num = xavaConfigGetInt(hand->default_config.config, "window", "monitor", 0);

    p->winA = (char *)xavaConfigGetString(hand->default_config.config, "window", "alignment", "none");
    p->wx = xavaConfigGetInt(hand->default_config.config, "window", "x_padding", 0);
    p->wy = xavaConfigGetInt(hand->default_config.config, "window", "y_padding", 0);
    p->fullF = xavaConfigGetBool(hand->default_config.config, "window", "fullscreen", 0);
    p->transF = xavaConfigGetBool(hand->default_config.config, "window", "transparency", 1);
    p->borderF = xavaConfigGetBool(hand->default_config.config, "window", "border", 0);
    p->bottomF = xavaConfigGetBool(hand->default_config.config, "window", "keep_below", 1);
    p->interactF = xavaConfigGetBool(hand->default_config.config, "window", "interactable", 1);
    p->taskbarF = xavaConfigGetBool(hand->default_config.config, "window", "taskbar_icon", 1);
    p->holdSizeF = xavaConfigGetBool(hand->default_config.config, "window", "hold_size", false);

    // config: filter
    filterMethod = (char *)xavaConfigGetString(hand->default_config.config, "filter", "name", XAVA_DEFAULT_FILTER);
    p->fftsize = (int)exp2((float)xavaConfigGetInt(hand->default_config.config, "filter", "fft_size", 14));

    validate_config(hand, hand->default_config.config);

    return configPath;
}

