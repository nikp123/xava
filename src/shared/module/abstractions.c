#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../module.h"
#include "internal.h"

XAVAMODULE *load_output_module(char *name) {
	char *new_name = calloc(strlen(name)+strlen("out_") + 1, sizeof(char));
	sprintf(new_name, "out_%s", name);
	XAVAMODULE *module = load_module(new_name);
	free(new_name);
	return module;
}

XAVAMODULE *load_input_module(char *name) {
	char *new_name = calloc(strlen(name)+strlen("in_") + 1, sizeof(char));
	sprintf(new_name, "in_%s", name);
	XAVAMODULE *module = load_module(new_name);
	free(new_name);
	return module;
}

XAVAMODULE *load_filter_module(char *name) {
	char *new_name = calloc(strlen(name)+strlen("filter_") + 1, sizeof(char));
	sprintf(new_name, "filter_%s", name);
	XAVAMODULE *module = load_module(new_name);
	free(new_name);
	return module;
}

