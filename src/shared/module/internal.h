#include "../module.h"

// these functions are only meant to be called within
// the "modules" part of the library and NEVER outside
XAVAMODULE *load_module(char *name);
XAVAMODULE *load_module_from_path(char *path);

