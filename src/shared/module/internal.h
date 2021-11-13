#include "../module.h"

// these functions are only meant to be called within
// the "modules" part of the library and NEVER outside
XAVAMODULE *xava_module_load(char *name);
XAVAMODULE *xava_module_load_path(char *path);

