#ifndef __XAVA_SHARED_MODULE_H
#define __XAVA_SHARED_MODULE_H
#include <stdbool.h>

// don't leak internal structure as the user of this library
// isn't supposed to mess with it
typedef struct xavamodule XAVAMODULE;

extern char *get_module_error(XAVAMODULE *module);
extern bool is_module_valid(XAVAMODULE *module);
extern void *get_symbol_address(XAVAMODULE *module, char *symbol);
extern XAVAMODULE *load_input_module(char *name);
extern XAVAMODULE *load_output_module(char *name);
extern XAVAMODULE *load_filter_module(char *name);
extern void xava_module_free(XAVAMODULE *module);

/**
 * name is the user-specified module name
 * prefix is the application-specified module name prefix
 * root_prefix is the directory from where the module is
 * supposed to be loaded (and it must include the last
 * bracket)
 **/
extern XAVAMODULE *xava_module_custom_load(char *name,
        const char *prefix, const char *root_prefix);

// get the path of a loaded module
const char *xava_module_path_get(XAVAMODULE *module);

// get the file extension of a module on the current system
const char *xava_module_extension_get(void);

#endif

