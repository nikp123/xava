#ifndef __XAVA_SHARED_CONFIG_H
#define __XAVA_SHARED_CONFIG_H
typedef struct xava_config* XAVACONFIG;

extern XAVACONFIG xavaConfigOpen(const char *filename);
extern void       xavaConfigClose(XAVACONFIG config);

extern bool   xavaConfigGetBool     (XAVACONFIG config, const char *section, const char *key, bool default_value);
extern int    xavaConfigGetInt      (XAVACONFIG config, const char *section, const char *key, int default_value);
extern double xavaConfigGetDouble   (XAVACONFIG config, const char *section, const char *key, double default_value);
extern char*  xavaConfigGetString   (XAVACONFIG config, const char *section, const char *key, const char* default_value);
extern int    xavaConfigGetKeyNumber(XAVACONFIG config, const char *section);
extern char** xavaConfigGetKeys     (XAVACONFIG config, const char *section);
#endif

