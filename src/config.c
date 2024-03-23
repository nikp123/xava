#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <math.h>
#include <limits.h>

#include "output/shared/graphical.h"
#include "config.h"
#include "shared.h"
#include "shared/log.h"
#include "shared/util/array.h"

static const char *colorStrings[8] = {"black", "red", "green", "yellow",
                                        "blue", "magenta", "cyan", "white"};

XAVA_CONFIG_OPTION(char*, inputMethod);
XAVA_CONFIG_OPTION(char*, outputMethod);
XAVA_CONFIG_OPTION(char*, filterMethod);
XAVA_CONFIG_OPTION(char*, channels);

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

void validate_config(XAVA *hand, xava_config_source config) {
    XAVA_CONFIG *p = &hand->conf;
    UNUSED(config);

    // validate: input method
    hand->audio.module = xava_module_input_load(inputMethod);
    xavaBailCondition(!xava_module_valid(hand->audio.module),
            "Input method '%s' at '%s' could not load.\nReason: %s",
            inputMethod, xava_module_path_get(hand->audio.module),
            xava_module_error_get(hand->audio.module));

    // validate: output method
    hand->output.module = xava_module_output_load(outputMethod);
    xavaBailCondition(!xava_module_valid(hand->output.module),
            "Output method '%s' at '%s' could not load.\nReason: %s",
            outputMethod, xava_module_path_get(hand->output.module),
            xava_module_error_get(hand->output.module));

    // validate: filter method
    hand->filter.module = xava_module_filter_load(filterMethod);
    xavaBailCondition(!xava_module_valid(hand->filter.module),
            "Filter method '%s' could not load.\nReason: %s",
            filterMethod, xava_module_path_get(hand->filter.module),
            xava_module_error_get(hand->filter.module));

    // validate: output channels
    int stereo = -1;
    if (strcmp(channels, "mono") == 0)   stereo = 0;
    if (strcmp(channels, "stereo") == 0) stereo = 1;
    xavaBailCondition(stereo == -1, "Output channels '%s' is not supported,"
            " supported channels are", " 'mono' and 'stereo'", channels);

    // validate: input
    p->stereo = stereo ? 1 : 0; // makes the C compilers happy
    p->stereo_is_set_from_file = channels_is_set_from_file;
    xavaBailCondition(!p->samplerate, "samplerate CANNOT BE 0!");
    xavaBailCondition(p->samplelatency > p->inputsize, "Sample latency cannot be larger than the audio buffer itself!");
    xavaWarnCondition(p->samplelatency*p->framerate > p->samplerate, "Sample latency might be too large, expect audio lags!");
    xavaWarnCondition(p->samplelatency < 32, "Sample latency might be too low, high CPU usage is MOST LIKELY!");
    xavaBailCondition(p->samplelatency == 0, "Sample latency CANNOT BE 0!");
    xavaBailCondition((p->fixedbars>0)&&(p->fixedbars%2)&&p->stereo,
            "Cannot have stereo AND an odd number of bars!");

    // validate: bars
    p->autobars = !p->fixedbars_is_set_from_file;
    if (p->bw < 1) p->bw = 1;

    // validate: framerate
    xavaBailCondition(p->framerate < 1, "Framerate cannot be below 1!");

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
    for(unsigned int i = 0; i < arr_count(p->gradients); i++){
        xavaBailCondition(!validate_color(p->gradients[i]),
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

    // MUST be set by the output module instead as it is QUITE memory unsafe
    p->flag.ignoreWindowSize = false;

    xavaBailCondition(!foundAlignment, "Alignment '%s' is invalid!\n",
            p->winA);
}

char *load_config(char *configPath, XAVA *hand) {
    XAVA_CONFIG *p = &hand->conf;

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
    XAVA_CONFIG_GET_STRING(hand->default_config.config, "input", "method", XAVA_DEFAULT_INPUT, inputMethod);
    XAVA_CONFIG_GET_U32(hand->default_config.config, "input", "size", 12, p->inputsize);
    // correct the value to whatever the engine actually needs
    p->inputsize = (int)exp2((double)p->inputsize);
    XAVA_CONFIG_GET_U32(hand->default_config.config, "input", "rate", 44100, p->samplerate);
    XAVA_CONFIG_GET_U32(hand->default_config.config, "input", "latency", 128, p->samplelatency);

    // config: output
    XAVA_CONFIG_GET_STRING(hand->default_config.config, "output", "method", XAVA_DEFAULT_OUTPUT, outputMethod);
    XAVA_CONFIG_GET_STRING(hand->default_config.config, "output", "channels", "mono", channels);

    XAVA_CONFIG_GET_STRING(hand->default_config.config, "color", "foreground", "default", p->color);
    XAVA_CONFIG_GET_STRING(hand->default_config.config, "color", "background", "default", p->bcolor);
    XAVA_CONFIG_GET_F64(hand->default_config.config, "color", "foreground_opacity", 1.0, p->foreground_opacity);
    XAVA_CONFIG_GET_F64(hand->default_config.config, "color", "background_opacity", 0.0, p->background_opacity);

    XAVA_CONFIG_OPTION(u32, gradient_count);
    XAVA_CONFIG_GET_U32(hand->default_config.config, "color", "gradient_count", 0, gradient_count);
    p->gradients_is_set_from_file = gradient_count_is_set_from_file;
    arr_init_n(p->gradients, gradient_count);
    arr_count(p->gradients) = gradient_count;
    if(arr_count(p->gradients) > 0) {
        xavaBailCondition(arr_count(p->gradients) < 2,
                "At least two colors must be given as gradient!\n");
        xavaBailCondition(arr_count(p->gradients) > 8,
                "Maximum 8 colors can be specified as gradient!\n");

        for(uint32_t i = 0; i < arr_count(p->gradients); i++) {
            XAVA_CONFIG_OPTION(char*, name);
            char ini_config[33];
            sprintf(ini_config, "gradient_color_%d", (i+1));
            XAVA_CONFIG_GET_STRING(hand->default_config.config, "color", ini_config, NULL, name);

            xavaBailCondition(name_is_set_from_file == false,
                    "'%s' wasn't set properly. Check your config!", ini_config);
            p->gradients[i] = name;
        }
    }

    // config: default
    XAVA_CONFIG_GET_I32(hand->default_config.config, "general", "bars", 0, p->fixedbars);
    XAVA_CONFIG_GET_U32(hand->default_config.config, "general", "bar_width", 13, p->bw);
    XAVA_CONFIG_GET_U32(hand->default_config.config, "general", "bar_spacing", 5, p->bs);
    XAVA_CONFIG_GET_I32(hand->default_config.config, "general", "framerate", 60, p->framerate);
    XAVA_CONFIG_GET_I32(hand->default_config.config, "general", "vsync", 1, p->vsync); // ideally is bool, but it's not

    // config: window
    XAVA_CONFIG_GET_U32(hand->default_config.config, "window", "width", 1180, p->w);
    XAVA_CONFIG_GET_U32(hand->default_config.config, "window", "height", 300, p->h);

    XAVA_CONFIG_GET_STRING(hand->default_config.config, "window", "alignment", "none", p->winA);
    XAVA_CONFIG_GET_I32(hand->default_config.config, "window", "x_padding", 0, p->x);
    XAVA_CONFIG_GET_I32(hand->default_config.config, "window", "y_padding", 0, p->y);
    XAVA_CONFIG_GET_BOOL(hand->default_config.config, "window", "fullscreen",   false, p->flag.fullscreen  );
    XAVA_CONFIG_GET_BOOL(hand->default_config.config, "window", "transparency", true,  p->flag.transparency);
    XAVA_CONFIG_GET_BOOL(hand->default_config.config, "window", "border",       false, p->flag.border      );
    XAVA_CONFIG_GET_BOOL(hand->default_config.config, "window", "keep_below",   true,  p->flag.beneath     );
    XAVA_CONFIG_GET_BOOL(hand->default_config.config, "window", "interactable", true,  p->flag.interact    );
    XAVA_CONFIG_GET_BOOL(hand->default_config.config, "window", "taskbar_icon", true,  p->flag.taskbar     );
    XAVA_CONFIG_GET_BOOL(hand->default_config.config, "window", "hold_size",    false, p->flag.holdSize    );

    // config: filter
    XAVA_CONFIG_GET_STRING(hand->default_config.config, "filter", "name", XAVA_DEFAULT_FILTER, filterMethod);
    XAVA_CONFIG_GET_U32(hand->default_config.config, "filter", "fft_size", 14, p->fftsize);
    p->fftsize = (int)exp2((double)p->fftsize);

    validate_config(hand, hand->default_config.config);

    return configPath;
}

