#ifndef __XAVA_SHARED_MODULE_H
#define __XAVA_SHARED_MODULE_H
#include <stdbool.h>

// don't leak internal structure as the user of this library
// isn't supposed to mess with it
typedef struct xavamodule XAVAMODULE;

extern char *xava_module_error_get(XAVAMODULE *module);
extern bool xava_module_valid(XAVAMODULE *module);
extern void *xava_module_symbol_address_get(XAVAMODULE *module, char *symbol);
extern XAVAMODULE *xava_module_input_load(char *name);
extern XAVAMODULE *xava_module_output_load(char *name);
extern XAVAMODULE *xava_module_filter_load(char *name);
extern XAVAMODULE *xava_module_path_load(char *path);
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
extern const char *xava_module_path_get(XAVAMODULE *module);

// get the file extension of a module on the current system
extern const char *xava_module_extension_get(void);

// return module generated filename into result
extern void xava_module_generate_filename(char *name,
        const char *prefix, char *result);


#endif

