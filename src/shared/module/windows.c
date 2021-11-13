#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <windows.h>

#include "../../shared.h"

typedef struct xavamodule {
    char *name;
    char *path;
    HMODULE moduleHandle;
    DWORD error;
} XAVAMODULE;

char *LIBRARY_EXTENSION = ".dll";

EXP_FUNC char *get_module_error(XAVAMODULE *module) {
    char *message;
    FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            module->error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &message,
            0, NULL);
    return message;
}

EXP_FUNC bool is_module_valid(XAVAMODULE *module) {
    if(module->moduleHandle)
        return 1;
    else
        return 0;
}

EXP_FUNC void *get_symbol_address(XAVAMODULE *module, char *symbol) {
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

EXP_FUNC XAVAMODULE *load_module(char *name) {
    // hacker prevention system 9000
    for(int i=0; i<strlen(name); i++) {
        // Disallow directory injections
        if(name[i] == '\\') return NULL;
    }

    char *new_name = malloc(strlen(name)+strlen(LIBRARY_EXTENSION)+1);
    sprintf(new_name, "%s%s", name, LIBRARY_EXTENSION);

    XAVAMODULE *module = malloc(sizeof(XAVAMODULE));
    module->name = new_name;
    module->moduleHandle = LoadLibrary(new_name);
    module->error        = GetLastError();

    if(module->moduleHandle) // if loading is done, we are successful
        return module;

    // Windows uses widechars internally
    WCHAR wpath[MAX_PATH];
    char *path = malloc(MAX_PATH);

    // Get path of where the executable is installed
    HMODULE hModule = GetModuleHandleW(NULL);
    GetModuleFileNameW(hModule, wpath, MAX_PATH);
    wcstombs(path, wpath, MAX_PATH);

    // remove program filename from the path
    for(int i=strlen(path)-1; i>0; i--) {
        if(path[i-1] == '\\') {
            path[i] = '\0';
            break;
        }
    }

    // append new filename to path
    strcat(path, new_name);

    // try again
    module->moduleHandle = LoadLibrary(path);
    module->error        = GetLastError();
    module->path         = path;

    return module; // if it failed XAVA would know (no error checks needed)
}

// because of stupidity this is not a entire path instead,
// it's supposed to take in the path without the extension
//
// the extension gets added here, just as a FYI
XAVAMODULE *load_module_from_path(char *path) {
    size_t offset;
    for(offset = strlen(path); offset > 0; offset--) {
        if(path[offset-1] == '\\')
            break;
    }

    char *new_name = &path[offset];
    char full_path[strlen(path)+strlen(LIBRARY_EXTENSION)+1];
    strcpy(full_path, path);
    strcat(full_path, LIBRARY_EXTENSION);

    XAVAMODULE *module   = (XAVAMODULE*) malloc(sizeof(XAVAMODULE));
    module->moduleHandle = LoadLibrary(full_path);
    module->error        = GetLastError();
    module->name         = new_name;
    module->path         = strdup(full_path);

    xavaLog("Module loaded '%s' loaded at %p", 
        module->name, module->moduleHandle);

    return module;
}

EXP_FUNC void destroy_module(XAVAMODULE *module) {
    FreeLibrary(module->moduleHandle);
    free(module->name);
    free(module->path);
    free(module);
}

const char *xava_module_path_get(XAVAMODULE *module) {
    // prevent NULL-pointer exception
    if(module == NULL)
        return NULL;

    return module->path;
}

const char *xava_module_extension_get(void) {
    return LIBRARY_EXTENSION;
}

