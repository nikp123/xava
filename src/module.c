#include "module.h"
#include "log.h"

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

	xavaLog("Module loaded '%s' loaded at %p", 
		module->name, module->moduleHandle);

	return module;
}

char *get_module_error(XAVAMODULE *module) {
	return dlerror();
}

void *get_symbol_address(XAVAMODULE *module, char *symbol) {
	void *address = dlsym(module->moduleHandle, symbol);

	// the program would crash with an NULL pointer error anyway
	xavaBailCondition(!address, "Failed to find symbol '%s' in file '%s'",
		symbol, module->name);

	xavaLog("Symbol '%s' of '%s' found at: %p",
			symbol, module->name, address);

	return address;
}

bool is_module_valid(XAVAMODULE *module) {
	if(module->moduleHandle) 
		return 1;
	else
		return 0;
}
#endif

#ifdef __WIN32__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

char *LIBRARY_EXTENSION = ".dll";

char *get_module_error(XAVAMODULE *module) {
	return "";
}

bool is_module_valid(XAVAMODULE *module) {
	if(module->moduleHandle)
		return 1;
	else
		return 0;
}

void *get_symbol_address(XAVAMODULE *module, char *symbol) {
	void *addr = GetProcAddress(module->moduleHandle, symbol);

	if(addr == NULL) {
		int error = GetLastError();
		char *message;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &message,
			0, NULL );

		xavaBail("Finding symbol '%s' in '%s' failed! Code %x\n"
				"Message: %s", symbol, module->name, error, message);
	}

	return addr;
}

XAVAMODULE *load_module(char *name) {
	char *new_name = malloc(strlen(name)+strlen(LIBRARY_EXTENSION));
	sprintf(new_name, "%s%s", name, LIBRARY_EXTENSION);
	XAVAMODULE *module = malloc(sizeof(XAVAMODULE));
	module->name = new_name;
	module->moduleHandle = LoadLibrary(new_name);
	return module;
}

void destroy_module(XAVAMODULE *module) {
	FreeLibrary(module->moduleHandle);
	free(module->name);
	free(module);
}
#endif

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

