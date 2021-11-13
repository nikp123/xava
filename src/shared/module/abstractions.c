#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../shared.h"
#include "internal.h"

#if defined(__WIN32__)
    #define DIR_BREAK '\\'
#else
    #define DIR_BREAK '/'
#endif

EXP_FUNC XAVAMODULE *xava_module_output_load(char *name) {
    char *new_name = calloc(strlen(name)+strlen("out_") + 1, sizeof(char));
    sprintf(new_name, "out_%s", name);
    XAVAMODULE *module = load_module(new_name);
    free(new_name);
    return module;
}

EXP_FUNC XAVAMODULE *xava_module_input_load(char *name) {
    char *new_name = calloc(strlen(name)+strlen("in_") + 1, sizeof(char));
    sprintf(new_name, "in_%s", name);
    XAVAMODULE *module = load_module(new_name);
    free(new_name);
    return module;
}

EXP_FUNC XAVAMODULE *xava_module_filter_load(char *name) {
    char *new_name = calloc(strlen(name)+strlen("filter_") + 1, sizeof(char));
    sprintf(new_name, "filter_%s", name);
    XAVAMODULE *module = load_module(new_name);
    free(new_name);
    return module;
}

EXP_FUNC XAVAMODULE *xava_module_custom_load(char *name, const char *prefix,
        const char *root_prefix) {
    char string_size = strlen(root_prefix) + 1 + strlen(prefix) + 1 +
        strlen(name) + 1;
    char path[string_size];

    // a kinda security feature
    for(size_t i=0; i<strlen(name); i++) {
        xavaBailCondition(name[i] == DIRBRK,
            "Directory injections are NOT allowed within the module name!");
    }

    xavaBailCondition(root_prefix[strlen(root_prefix)-1] != DIRBRK,
        "Bug detected: The dev SHOULD'VE included the ending bracket at the "
        "root_prefix parameter. Please report this!");

    snprintf(path, string_size-1, "%s%s_%s",
        root_prefix, prefix, name);
    return load_module_from_path(path);
}

