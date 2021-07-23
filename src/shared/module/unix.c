#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>

#include "../../shared.h"

typedef struct xavamodule {
	char *name;
	void **moduleHandle;
} XAVAMODULE;

char *LIBRARY_EXTENSION = ".so";

EXP_FUNC void destroy_module(XAVAMODULE *module) {
	dlclose(module->moduleHandle);
	module->moduleHandle = 0;
	free(module->name);
	free(module);
}

EXP_FUNC XAVAMODULE *load_module(char *name) {
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

	xavaLog("Module loaded '%s' loaded at %p", 
		module->name, module->moduleHandle);

	return module;
}

EXP_FUNC char *get_module_error(XAVAMODULE *module) {
	return dlerror();
}

EXP_FUNC void *get_symbol_address(XAVAMODULE *module, char *symbol) {
	void *address = dlsym(module->moduleHandle, symbol);

	// the program would crash with an NULL pointer error anyway
	xavaBailCondition(!address, "Failed to find symbol '%s' in file '%s'",
		symbol, module->name);

	xavaLog("Symbol '%s' of '%s' found at: %p",
			symbol, module->name, address);

	return address;
}

EXP_FUNC bool is_module_valid(XAVAMODULE *module) {
	if(module->moduleHandle) 
		return 1;
	else
		return 0;
}

