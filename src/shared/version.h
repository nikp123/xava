#ifndef __XAVA_SHARED_VERSION_H
#define __XAVA_SHARED_VERSION_H
#include <stdbool.h>

typedef enum XAVA_VERSION_COMPATIBILITY {
    XAVA_VERSIONS_COMPATIBLE,
    XAVA_VERSIONS_INCOMPATIBLE,
    XAVA_VERSIONS_UNKNOWN,
} XAVA_VERSION_COMPATIBILITY;

typedef struct xava_version {
    int major;
    int minor;
    int tweak;
    int patch;
} xava_version;

xava_version               xava_version_host_get(void);
bool                       xava_version_less(xava_version host, xava_version target);
bool                       xava_version_greater(xava_version host, xava_version target);
bool                       xava_version_equal(xava_version host, xava_version target);
bool                       xava_version_breaking_check(xava_version target);
XAVA_VERSION_COMPATIBILITY xava_version_verify(xava_version target);
#endif

