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

xava_version               xavaGetHostVersion(void);
bool                       xavaIsVersionLess(xava_version host, xava_version target);
bool                       xavaIsVersionGreater(xava_version host, xava_version target);
bool                       xavaIsVersionEqual(xava_version host, xava_version target);
bool                       xavaIsBreakingVersion(xava_version target);
XAVA_VERSION_COMPATIBILITY xavaVerifyVersion(xava_version target);
#endif

