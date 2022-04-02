#ifndef __XAVA_SHARED_CONFIG_H
#define __XAVA_SHARED_CONFIG_H

#include "shared/util/types.h"

// config entry macro (cursed)
#define XAVA_CONFIG_OPTION(T, name) \
   T name; bool name##_is_set_from_file

// config line macro
#define XAVA_CONFIG_GET_STRING(config, section, key, default_value, return_value) \
    __internal_xavaConfigGetString(config, section, key, default_value, &return_value, &return_value##_is_set_from_file)
#define XAVA_CONFIG_GET_BOOL(config, section, key, default_value, return_value) \
    __internal_xavaConfigGetBool(config, section, key, default_value, &return_value, &return_value##_is_set_from_file)
#define XAVA_CONFIG_GET_I32(config, section, key, default_value, return_value) \
    __internal_xavaConfigGetI32(config, section, key, default_value, &return_value, &return_value##_is_set_from_file)
#define XAVA_CONFIG_GET_U32(config, section, key, default_value, return_value) \
    __internal_xavaConfigGetU32(config, section, key, default_value, &return_value, &return_value##_is_set_from_file)
#define XAVA_CONFIG_GET_F64(config, section, key, default_value, return_value) \
    __internal_xavaConfigGetF64(config, section, key, default_value, &return_value, &return_value##_is_set_from_file)

typedef struct xava_config* xava_config_source;

extern void __internal_xavaConfigGetString(
        xava_config_source config,
        const char* section,
        const char* key,
        const char* default_value,
        char **value,
        bool *value_is_set);

extern void __internal_xavaConfigGetBool(
        xava_config_source config,
        const char* section,
        const char* key,
        bool default_value,
        bool *value,
        bool *value_is_set);

extern void __internal_xavaConfigGetI32(
        xava_config_source config,
        const char* section,
        const char* key,
        i32 default_value,
        i32 *value,
        bool *value_is_set);

extern void __internal_xavaConfigGetU32(
        xava_config_source config,
        const char* section,
        const char* key,
        u32 default_value,
        u32 *value,
        bool *value_is_set);

extern void __internal_xavaConfigGetF64(
        xava_config_source config,
        const char* section,
        const char* key,
        f64 default_value,
        f64 *value,
        bool *value_is_set);


extern bool  xavaConfigGetBool  (xava_config_source config, const char *section, const char *key, bool default_value);
extern i32   xavaConfigGetI32   (xava_config_source config, const char *section, const char *key, i32 default_value);
extern f64   xavaConfigGetF64   (xava_config_source config, const char *section, const char *key, f64 default_value);
extern char* xavaConfigGetString(xava_config_source config, const char *section, const char *key, const char* default_value);

extern xava_config_source xavaConfigOpen (const char *filename);
extern void               xavaConfigClose(xava_config_source config);

extern int    xavaConfigGetKeyNumber(xava_config_source config, const char *section);
extern char** xavaConfigGetKeys     (xava_config_source config, const char *section);
#endif

