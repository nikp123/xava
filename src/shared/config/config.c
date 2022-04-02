#include <stdio.h>
#include <string.h>

#include <iniparser.h>

#include "shared.h"
#include "shared/config/config.h"

struct xava_config {
    dictionary *ini;
    char *filename;
};

EXP_FUNC xava_config_source xavaConfigOpen(const char *filename) {
    xava_config_source config = MALLOC_SELF(config, 1);
    xavaReturnErrorCondition(!config, NULL, "Failed to allocate config");
    config->filename = strdup(filename);
    config->ini = iniparser_load(config->filename);
    xavaReturnErrorCondition(!config->ini, NULL,
            "Failed to load config file %s", filename);
    return config;
}

EXP_FUNC void xavaConfigClose(xava_config_source config) {
    iniparser_freedict(config->ini);
    free(config->filename);
    free(config);
}

EXP_FUNC void __internal_xavaConfigGetString(
        xava_config_source config,
        const char* section,
        const char* key,
        const char* default_value,
        char **value,
        bool *value_is_set) {

    char ini_key[64];
    snprintf(ini_key, 64, "%s:%s", section, key);

    const char *result = iniparser_getstring(config->ini, ini_key, NULL);

    if(result == NULL) {
        *value = (char*)default_value;
        *value_is_set = false;
        return;
    }

    *value = (char*)result;
    *value_is_set = true;
    return;
}


EXP_FUNC void __internal_xavaConfigGetBool(
        xava_config_source config,
        const char* section,
        const char* key,
        bool default_value,
        bool *value,
        bool *value_is_set) {

    char ini_key[64];
    snprintf(ini_key, 64, "%s:%s", section, key);

    bool exists = iniparser_find_entry(config->ini, ini_key);

    if(!exists) {
        *value = default_value;
        *value_is_set = false;
        return;
    }

    *value = iniparser_getboolean(config->ini, ini_key, default_value);
    *value_is_set = true;
    return;
}

EXP_FUNC void __internal_xavaConfigGetI32(
        xava_config_source config,
        const char* section,
        const char* key,
        i32 default_value,
        i32 *value,
        bool *value_is_set) {

    char ini_key[64];
    snprintf(ini_key, 64, "%s:%s", section, key);

    bool exists = iniparser_find_entry(config->ini, ini_key);

    if(!exists) {
        *value = default_value;
        *value_is_set = false;
        return;
    }

    *value = iniparser_getint(config->ini, ini_key, default_value);
    *value_is_set = true;
    return;
}

EXP_FUNC void __internal_xavaConfigGetU32(
        xava_config_source config,
        const char* section,
        const char* key,
        u32 default_value,
        u32 *value,
        bool *value_is_set) {
    i32 new_default_value = (i32)default_value;
    i32 new_value;

    __internal_xavaConfigGetI32(config, section, key,
            new_default_value, &new_value, value_is_set);

    if(*value_is_set == true) {
        *value = new_value;
    } else {
        *value = default_value;
    }
}

EXP_FUNC void __internal_xavaConfigGetF64(
        xava_config_source config,
        const char* section,
        const char* key,
        f64 default_value,
        f64 *value,
        bool *value_is_set) {

    char ini_key[64];
    snprintf(ini_key, 64, "%s:%s", section, key);

    bool exists = iniparser_find_entry(config->ini, ini_key);

    if(!exists) {
        *value = default_value;
        *value_is_set = false;
        return;
    }

    *value = iniparser_getdouble(config->ini, ini_key, default_value);
    *value_is_set = true;
    return;
}

// This shouldn't exist, but yet here we are
EXP_FUNC bool xavaConfigGetBool  (xava_config_source config, const char *section, const char *key,
        bool default_value) {
    char ini_key[64]; // i cannot be fucked to do this shit anymore
    snprintf(ini_key, 64, "%s:%s", section, key);
    return iniparser_getboolean(config->ini, ini_key, default_value);
}

EXP_FUNC i32 xavaConfigGetI32(xava_config_source config, const char *section, const char *key,
        i32 default_value) {
    char ini_key[64]; // i cannot be fucked to do this shit anymore
    snprintf(ini_key, 64, "%s:%s", section, key);
    return iniparser_getint(config->ini, ini_key, default_value);
}

EXP_FUNC f64 xavaConfigGetF64(xava_config_source config, const char *section, const char *key,
        f64 default_value) {
    char ini_key[64]; // i cannot be fucked to do this shit anymore
    snprintf(ini_key, 64, "%s:%s", section, key);
    return iniparser_getdouble(config->ini, ini_key, default_value);
}

EXP_FUNC char* xavaConfigGetString(xava_config_source config, const char *section, const char *key,
        const char* default_value) {
    char ini_key[64]; // i cannot be fucked to do this shit anymore
    snprintf(ini_key, 64, "%s:%s", section, key);
    return (char*)iniparser_getstring(config->ini, ini_key, default_value);
}

EXP_FUNC int xavaConfigGetKeyNumber(xava_config_source config, const char *section) {
    return iniparser_getsecnkeys(config->ini, section);
}

EXP_FUNC char **xavaConfigGetKeys(xava_config_source config, const char *section) {
    size_t size = xavaConfigGetKeyNumber(config, section);

    char **returned_section_keys = calloc(size, sizeof(char*));
    iniparser_getseckeys(config->ini, section, (const char**)returned_section_keys);

    char **translated_section_keys;
    MALLOC_SELF(translated_section_keys, size);
    size_t skip = strlen(section)+1;
    for(size_t i=0; i<size; i++) {
        translated_section_keys[i] = &returned_section_keys[i][skip];
    }

    free(returned_section_keys);

    return translated_section_keys;
}

