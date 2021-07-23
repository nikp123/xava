#ifndef __XAVA_SHARED_CONFIG_H
#define __XAVA_SHARED_CONFIG_H
typedef struct xava_config* XAVACONFIG;

extern XAVACONFIG xavaConfigOpen(const char *filename);
extern void       xavaConfigClose(XAVACONFIG config);

extern bool         xavaConfigGetBool  (XAVACONFIG config, char *section, char *key, bool default_value);
extern int          xavaConfigGetInt   (XAVACONFIG config, char *section, char *key, int default_value);
extern double       xavaConfigGetDouble(XAVACONFIG config, char *section, char *key, double default_value);
extern const char*  xavaConfigGetString(XAVACONFIG config, char *section, char *key, const char* default_value);
extern int          xavaConfigGetKeyNumber(XAVACONFIG config, char *section);
extern char**       xavaConfigGetKeys(XAVACONFIG config, char *section);
#endif

