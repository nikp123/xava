#include "module.h"

#ifdef __unix__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>

char *LIBRARY_EXTENSION = ".so";

void destroy_module(XAVAMODULE *module) {
	dlclose(module->moduleHandle);
	module->moduleHandle = 0;
	free(module->name);
}

XAVAMODULE *load_module(char *name) {
	// Security check
	for(int i=0; i<strlen(name); i++) {
		// Disallow directory injections
		if(name[i] == '/') return NULL;
	}

	// Typically /usr/local/lib/xava/
	size_t new_size = strlen(name) + sizeof(PREFIX"/lib/xava/")
		+ strlen(LIBRARY_EXTENSION);

	// allocate enough for the system module size instead
	// because it's bigger, duh...
	char *new_name = calloc(new_size, sizeof(char));

	// regardless, try the local module first, as it has priority
	// over normal ones
	sprintf(new_name, "./%s%s", name, LIBRARY_EXTENSION);

	// check if the thing even exists
	FILE *fp = fopen(new_name, "r");
	if(fp == NULL) {
		sprintf(new_name, PREFIX"/lib/xava/%s%s", name,
			LIBRARY_EXTENSION);

		// lower the name, because users
		for(int i=strlen(PREFIX"/lib/xava/"); i<strlen(new_name); i++)
			new_name[i] = tolower(new_name[i]);
	} else fclose(fp);

	XAVAMODULE *module = (XAVAMODULE*) malloc(sizeof(XAVAMODULE));
	module->moduleHandle = dlopen(new_name, RTLD_NOW);
	module->name = new_name;

	#ifdef DEBUG
		printf("Module loaded '%s' loaded at %p\n",
			module->name, module->moduleHandle);
	#endif

	return module;
}

XAVAMODULE *load_output_module(char *name) {
	char *new_name = calloc(strlen(name)+strlen("out_"), sizeof(char));
	sprintf(new_name, "out_%s", name);
	XAVAMODULE *module = load_module(new_name);
	free(new_name);
	return module;
}

XAVAMODULE *load_input_module(char *name) {
	char *new_name = calloc(strlen(name)+strlen("in_"), sizeof(char));
	sprintf(new_name, "in_%s", name);
	XAVAMODULE *module = load_module(new_name);
	free(new_name);
	return module;
}

void print_module_error() {
	fprintf(stderr, "%s\n", dlerror());
}

void *get_symbol_address(XAVAMODULE *module, char *symbol) {
	void *address = dlsym(module->moduleHandle, symbol);

	if(address == NULL) {
		fprintf(stderr, "Failed to find symbol '%s' in file %s\n",
			symbol, module->name);
		print_module_error();
		exit(EXIT_FAILURE);
		// the program would crash with an NULL address error anyway
	}

	#ifdef DEBUG
		printf("Symbol '%s' of '%s' found at: %p\n",
			symbol, module->name, address);
	#endif

	return address;
}

bool is_module_valid(XAVAMODULE *module) {
	if(module->moduleHandle) 
		return 1;
	else
		return 0;
}
#endif

