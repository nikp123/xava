#ifndef __XAVA_OUTPUT_CAIRO_MODULE_UTIL_CONFIG
#define __XAVA_OUTPUT_CAIRO_MODULE_UTIL_CONFIG

#include "../../util/module.h"

typedef enum xava_cairo_file_type {
    XAVA_CAIRO_FILE_CONFIG,               // for loading xava_config_source-es
    XAVA_CAIRO_FILE_CONFIG_CUSTOM_READ,   // for loading any file found in the config dir
    XAVA_CAIRO_FILE_CONFIG_CUSTOM_WRITE,  // the same as above
    XAVA_CAIRO_FILE_INSTALL_READ,         // for loading any files off of the install
    XAVA_CAIRO_FILE_ERROR,                // placeholder
} xava_cairo_file_type;

/**
 * A config/custom file helper for XAVA_CAIRO modules
 *
 * 1. You need to specify what type the file actually is
 * 2. A reference to the module that called it
 * 3. Local file name within your designated config folder for your module.
 * 4. If non-NULL, will write the full output file path here
 *
 * The return pointer where the resulting FILE* or xava_config_source gets put
 *
 * On fail the function returns a NULL
**/
void *xava_cairo_module_file_load(
        xava_cairo_file_type      type,
        xava_cairo_module_handle *module,
        const char               *file_name,
        char                     *returned_path);

#endif

