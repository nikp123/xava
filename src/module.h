// Modules are handled in a struct-like fashion because
// the implementation is supposed to be portable, hence
// better add a bit of abstraction instead of complicating
// everything

#ifdef __unix__
typedef struct xavamodule {
	char *name;
	void **moduleHandle;
} XAVAMODULE;
#endif

#include <stdbool.h>

void print_module_error();
bool is_module_valid(XAVAMODULE *module);
void *get_symbol_address(XAVAMODULE *module, char *symbol);
XAVAMODULE *load_input_module(char *name);
XAVAMODULE *load_output_module(char *name);
void destroy_module(XAVAMODULE *module);

