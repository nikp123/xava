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
extern void destroy_module(XAVAMODULE *module);

#endif

