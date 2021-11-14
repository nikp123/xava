#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>

#include "../../shared.h"

typedef struct xavamodule {
    char *name;
    void **moduleHandle;
    char *path;
} XAVAMODULE;

char *LIBRARY_EXTENSION = ".so";

EXP_FUNC void xava_module_free(XAVAMODULE *module) {
    dlclose(module->moduleHandle);
    module->moduleHandle = 0;
    free(module->name);
    free(module->path);
    free(module);
}

EXP_FUNC XAVAMODULE *xava_module_load(char *name) {
    // Security check
    for(int i=0; i<strlen(name); i++) {
        // Disallow directory injections
        if(name[i] == '/') return NULL;
    }

    // Typically /usr/local/lib/xava/
    size_t new_size = strlen(name) + sizeof(PREFIX"/lib/xava/")
        + strlen(LIBRARY_EXTENSION);
    new_size *= sizeof(char);

    // allocate enough for the system module size instead
    // because it's bigger, duh...
    char *new_name = malloc(new_size);

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
    module->name = name;

    // don't ask, this is unexplainable C garbage at work again
    module->path = strdup(new_name);
    free(new_name);

    xavaLog("Module loaded '%s' loaded at %p", 
        module->name, module->moduleHandle);

    return module;
}

// because of stupidity this is not a entire path instead,
// it's supposed to take in the path without the extension
//
// the extension gets added here, just as a FYI
EXP_FUNC XAVAMODULE *xava_module_load_path(char *path) {
    size_t offset;
    for(offset = strlen(path); offset > 0; offset--) {
        if(path[offset-1] == '/')
            break;
    }

    char *new_name = &path[offset];
    char full_path[strlen(path)+strlen(LIBRARY_EXTENSION)+1];
    strcpy(full_path, path);
    strcat(full_path, LIBRARY_EXTENSION);

    XAVAMODULE *module = (XAVAMODULE*) malloc(sizeof(XAVAMODULE));
    module->moduleHandle = dlopen(full_path, RTLD_NOW);
    module->name = new_name;
    module->path = strdup(full_path);

    xavaLog("Module loaded '%s' loaded at %p", 
        module->name, module->moduleHandle);

    return module;
}

EXP_FUNC char *xava_module_error_get(XAVAMODULE *module) {
    return dlerror();
}

EXP_FUNC void *xava_module_symbol_address_get(XAVAMODULE *module, char *symbol) {
    void *address = dlsym(module->moduleHandle, symbol);

    // the program would crash with an NULL pointer error anyway
    xavaBailCondition(!address, "Failed to find symbol '%s' in file '%s'",
        symbol, module->name);

    xavaLog("Symbol '%s' of '%s' found at: %p",
            symbol, module->name, address);

    return address;
}

EXP_FUNC bool xava_module_valid(XAVAMODULE *module) {
    if(module->moduleHandle) 
        return 1;
    else
        return 0;
}

EXP_FUNC const char *xava_module_path_get(XAVAMODULE *module) {
    // prevent NULL-pointer exception
    if(module == NULL)
        return NULL;

    return module->path;
}

EXP_FUNC const char *xava_module_extension_get(void) {
    return LIBRARY_EXTENSION;
}

EXP_FUNC void xava_module_generate_filename(char *name,
        const char *prefix, char *result) {
    sprintf(result, "%s_%s%s", prefix, name, LIBRARY_EXTENSION);
    return;
}
