#include <stdio.h>
#include <string.h>

#include <iniparser.h>

#include "../../shared.h"

#include "../config.h"

struct xava_config {
	dictionary *ini;
	char *filename;
};

EXP_FUNC XAVACONFIG xavaConfigOpen(const char *filename) {
	XAVACONFIG config = MALLOC_SELF(config, 1);
	xavaReturnErrorCondition(!config, NULL, "Failed to allocate config");
	config->filename = strdup(filename);
	config->ini = iniparser_load(config->filename);
	xavaReturnErrorCondition(!config->ini, NULL,
			"Failed to load config file %s", filename);
	return config;
}

EXP_FUNC void xavaConfigClose(XAVACONFIG config) {
	iniparser_freedict(config->ini);
	free(config->filename);
	free(config);
}

// This shouldn't exist, but yet here we are
EXP_FUNC bool xavaConfigGetBool  (XAVACONFIG config, const char *section, const char *key,
		bool default_value) {
	char ini_key[64]; // i cannot be fucked to do this shit anymore
	snprintf(ini_key, 64, "%s:%s", section, key);
	return iniparser_getboolean(config->ini, ini_key, default_value);
}

EXP_FUNC int xavaConfigGetInt(XAVACONFIG config, const char *section, const char *key,
		int default_value) {
	char ini_key[64]; // i cannot be fucked to do this shit anymore
	snprintf(ini_key, 64, "%s:%s", section, key);
	return iniparser_getint(config->ini, ini_key, default_value);
}

EXP_FUNC double xavaConfigGetDouble(XAVACONFIG config, const char *section, const char *key,
		double default_value) {
	char ini_key[64]; // i cannot be fucked to do this shit anymore
	snprintf(ini_key, 64, "%s:%s", section, key);
	return iniparser_getdouble(config->ini, ini_key, default_value);
}

EXP_FUNC char* xavaConfigGetString(XAVACONFIG config, const char *section, const char *key,
		const char* default_value) {
	char ini_key[64]; // i cannot be fucked to do this shit anymore
	snprintf(ini_key, 64, "%s:%s", section, key);
	return (char*)iniparser_getstring(config->ini, ini_key, default_value);
}

EXP_FUNC int xavaConfigGetKeyNumber(XAVACONFIG config, const char *section) {
	return iniparser_getsecnkeys(config->ini, section);
}

EXP_FUNC char **xavaConfigGetKeys(XAVACONFIG config, const char *section) {
	char **returned_section_keys = NULL;
	iniparser_getseckeys(config->ini, section, (const char**)returned_section_keys);

	char **translated_section_keys;
	size_t size = xavaConfigGetKeyNumber(config, section);
	MALLOC_SELF(translated_section_keys, size);
	size_t skip = strlen(section)+1;
	for(int i=0; i<size; i++) {
		translated_section_keys[i] = &returned_section_keys[i][skip];
	}

	return translated_section_keys;
}

