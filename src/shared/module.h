//#ifndef __XAVA_SHARED_LOG_H
//#define __XAVA_SHARED_LOG_H
#include <stdbool.h>

// don't leak internal structure as the user of this library
// isn't supposed to mess with it
typedef struct xavamodule XAVAMODULE;

char *get_module_error(XAVAMODULE *module);
bool is_module_valid(XAVAMODULE *module);
void *get_symbol_address(XAVAMODULE *module, char *symbol);
XAVAMODULE *load_input_module(char *name);
XAVAMODULE *load_output_module(char *name);
XAVAMODULE *load_filter_module(char *name);
void destroy_module(XAVAMODULE *module);

//#endif

