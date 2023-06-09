#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cairo.h>

#include "shared.h"

#include "output/shared/cairo/main.h"

#include "output/shared/cairo/util/feature_compat.h"
#include "output/shared/cairo/util/module.h"
#include "output/shared/cairo/util/region.h"

#include "config.h"

void *xava_cairo_module_file_load(
        xava_cairo_file_type      type,
        xava_cairo_module_handle *module,
        const char               *file_name,
        char                     *returned_path) {

    char file_path[MAX_PATH];
    void *pointer = NULL;

    xava_config_source config;

    // "cairo/module/$module_name"
    strcpy(file_path, module->prefix);
    strcat(file_path, "/");
    // add file name
    strcat(file_path, file_name);

    enum XAVA_FILE_TYPE xava_io_file_type;
    switch(type) {
        case XAVA_CAIRO_FILE_INSTALL_READ:
            xava_io_file_type = XAVA_FILE_TYPE_PACKAGE;
            break;
        default:
            xava_io_file_type = XAVA_FILE_TYPE_CONFIG;
            break;
    }

    // get REAL path
    char *actual_path;
    xavaReturnErrorCondition(
        xavaFindAndCheckFile(xava_io_file_type,
            file_path, &actual_path) == false,
        NULL,
        "Failed to find or open '%s'", file_path);

    switch(type) {
        case XAVA_CAIRO_FILE_CONFIG:
            config = xavaConfigOpen(actual_path);
            pointer = malloc(sizeof(xava_config_source));
            xavaBailCondition(pointer == NULL, "Memory error");
            ((xava_config_source*)pointer)[0] = config;
            break;
        case XAVA_CAIRO_FILE_INSTALL_READ:
        case XAVA_CAIRO_FILE_CONFIG_CUSTOM_READ:
            pointer = fopen(file_path, "rb");
            break;
        case XAVA_CAIRO_FILE_CONFIG_CUSTOM_WRITE:
            pointer = fopen(file_path, "wb");
            break;
        default:
            xavaReturnError(NULL, "Invalid file type");
    }

    if(returned_path != NULL) {
        strcpy(returned_path, actual_path);
    }
    free(actual_path);

    return pointer;
}

