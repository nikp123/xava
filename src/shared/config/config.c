#include <stdio.h>
#include <string.h>

#include <iniparser.h>

#include "shared.h"
#include "shared/config.h"

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

// This shouldn't exist, but yet here we are
EXP_FUNC bool xavaConfigGetBool  (xava_config_source config, const char *section, const char *key,
        bool default_value) {
    char ini_key[64]; // i cannot be fucked to do this shit anymore
    snprintf(ini_key, 64, "%s:%s", section, key);
    return iniparser_getboolean(config->ini, ini_key, default_value);
}

EXP_FUNC int xavaConfigGetInt(xava_config_source config, const char *section, const char *key,
        int default_value) {
    char ini_key[64]; // i cannot be fucked to do this shit anymore
    snprintf(ini_key, 64, "%s:%s", section, key);
    return iniparser_getint(config->ini, ini_key, default_value);
}

EXP_FUNC double xavaConfigGetDouble(xava_config_source config, const char *section, const char *key,
        double default_value) {
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
    for(int i=0; i<size; i++) {
        translated_section_keys[i] = &returned_section_keys[i][skip];
    }

    free(returned_section_keys);

    return translated_section_keys;
}

