#ifndef __XAVA_SHARED_CONFIG_H
#define __XAVA_SHARED_CONFIG_H
typedef struct xava_config* xava_config_source;

extern xava_config_source xavaConfigOpen(const char *filename);
extern void       xavaConfigClose(xava_config_source config);

extern bool   xavaConfigGetBool     (xava_config_source config, const char *section, const char *key, bool default_value);
extern int    xavaConfigGetInt      (xava_config_source config, const char *section, const char *key, int default_value);
extern double xavaConfigGetDouble   (xava_config_source config, const char *section, const char *key, double default_value);
extern char*  xavaConfigGetString   (xava_config_source config, const char *section, const char *key, const char* default_value);
extern int    xavaConfigGetKeyNumber(xava_config_source config, const char *section);
extern char** xavaConfigGetKeys     (xava_config_source config, const char *section);
#endif

